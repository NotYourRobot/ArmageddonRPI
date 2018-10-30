/*
 * File: INFO.C
 * Usage: Informative commands.
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

/*
 * Revision history.  Pls. post changes to this file.
 * 12/17/1999 Added this, and the light-stuff. (Will
 * quote lines in a bit.   -Sanvean
 * 04/24/2000 Fixed a bunch of typos in wall spells.  -sv
 */

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <time.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <unistd.h>

#include "constants.h"
#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "comm.h"
#include "parser.h"
#include "pather.h"
#include "handler.h"
#include "db.h"
#include "db_file.h"
#include "skills.h"
#include "limits.h"
#include "guilds.h"
#include "clan.h"
#include "creation.h"
#include "modify.h"
#include "object_list.h"
#include "character.h"
#include "cities.h"
#include "reporting.h"
#include "special.h"
#include "wagon_save.h"
#include "watch.h"
#include "immortal.h"
#include "info.h"
#define SHOW_CRIMINAL

/* Procedures related to 'look' */
void show_tables(CHAR_DATA * ch);
void show_obj_properties(OBJ_DATA * obj, CHAR_DATA * ch, int bits);

/* create_help_data()
 *
 * Creates a new help entry and returns a pointer to it.  Users are expected
 * to manage deleting the memory when finished.
 */
HELP_DATA *
create_help_data()
{
  HELP_DATA *help_entry = NULL;

  CREATE(help_entry, HELP_DATA, 1);
  bzero(help_entry, sizeof(help_entry));
  //  reset_help_data(help_entry);

  return help_entry;
}

/* reset_help_data()
 *
 * Sets the values of the specified help entry to their default.
 */
void
reset_help_data(HELP_DATA * help_entry)
{
  // Set everything to their default value
  help_entry->topic = NULL;
  help_entry->category = NULL;
  help_entry->body = NULL;
  help_entry->syntax = NULL;
  help_entry->notes = NULL;
  help_entry->delay = NULL;
  help_entry->see_also = NULL;
  help_entry->example = NULL;
}


/* free_help_data()
 *
 * Releases the memory associated with a help_data
 */
void
free_help_data(HELP_DATA * help_entry)
{
  DESTROY(help_entry->topic);
  DESTROY(help_entry->category);
  DESTROY(help_entry->body);
  DESTROY(help_entry->syntax);
  DESTROY(help_entry->notes);
  DESTROY(help_entry->delay);
  DESTROY(help_entry->see_also);
  DESTROY(help_entry->example);
  DESTROY(help_entry);
}


// Different poison types
const char *poison_color[] = {
	"none",
	"dour grey",            /* Generic and Spell */
	"pale yellow",          /* Grishen */
	"wavering purple",      /* Skellebain (visions) */
	"cold blue",            /* Methelinoc */
	"obsidian",             /* Terradin */
	"greenish-yellow",      /* Peraine  */
	"white",                /* Heramide */
	"bright red",           /* Plague */
	"pulsing purple",       /* Skellebain (tripping) */
        "bright purple",        /* Skellebain (physical) */
        "\n"
};

const char *
indefinite_article(const char *str)
{
    if (!strncmp(str, "some ", 5))
        return "";
    if (IS_VOWEL(*str))
        return "an ";
    return "a ";
}

char *
find_ex_description_all_words(char *words, struct extra_descr_data *list, bool brackets_ok)
{
    struct extra_descr_data *i;

    for (i = list; i; i = i->next)
        if (!isnameprefix("page_", i->keyword)  /* stop finding page_ */
            &&((!brackets_ok && isallname(words, i->keyword))
               || (brackets_ok && isallnamebracketsok(words, i->keyword))))
            return (i->description);

    return (0);
}

char *
find_ex_description(char *word, struct extra_descr_data *list, bool brackets_ok)
{
    struct extra_descr_data *i;

    for (i = list; i; i = i->next)
        if (!isnameprefix("page_", i->keyword)  /* stop finding page_ */
            &&((!brackets_ok && isname(word, i->keyword))
               || (brackets_ok && isnamebracketsok(word, i->keyword))))
            return (i->description);

    return (0);
}

char *
find_page_ex_description(char *word, struct extra_descr_data *list, bool brackets_ok)
{
    struct extra_descr_data *i;

    for (i = list; i; i = i->next)
        if ((!brackets_ok && isname(word, i->keyword))
            || (brackets_ok && isnamebracketsok(word, i->keyword)))
            return (i->description);

    return (0);
}

void
show_list_of_obj_to_char( OBJ_DATA *obj, OBJ_DATA *list, CHAR_DATA *ch, char *location, int bits ) {
    char *tmpstr;
    char buffer[MAX_STRING_LENGTH];

    if (IS_SET(ch->specials.brief, BRIEF_EQUIP)) {
        sprintf(buffer, "%s %s (%s) is %s.",
         location,
         format_obj_to_char(obj,ch,1),
         bits == FIND_OBJ_INV ? "carried" 
          : bits == FIND_OBJ_ROOM ? "here"
          : bits == FIND_OBJ_EQUIP ? "used" : "unknown",
         list_obj_to_char(list, ch, 2, TRUE));
        tmpstr = strdup(buffer);
        tmpstr = format_string(tmpstr);
        cprintf(ch, "%s", tmpstr);
        free(tmpstr);
    } else {
        cprintf(ch, "%s %s (%s) :\n\r%s\n\r",
         location,
         format_obj_to_char(obj,ch,1),
         bits == FIND_OBJ_INV ? "carried" 
          : bits == FIND_OBJ_ROOM ? "here" 
          : bits == FIND_OBJ_EQUIP ? "used" : "unknown",
         list_obj_to_char(list, ch, 2, TRUE));
    }
}

int
change_ldesc(const char *new_ldesc, CHAR_DATA * ch)
{
    char desc_holder[85];

/* It's ok now, really, it is -Morg
   if (IS_NPC(ch))
   return TRUE;
 */

    if (new_ldesc == NULL)
        return FALSE;

    if (strlen(new_ldesc) + strlen(MSTR(ch, short_descr)) > 80)
        return FALSE;

    /* -- Shouldn't cause a problem as it's reset if you do anything active,
     * but if someone casts invis on you, it might mess up, so just evaluate
     * with PERS()
     *
     * strcat(desc_holder, MSTR( ch, short_descr ) );
     * strcat(desc_holder, " ");
     */

    strcpy(desc_holder, "");
    strcat(desc_holder, new_ldesc);

    if (!ispunct(desc_holder[strlen(desc_holder) - 1]))
        strcat(desc_holder, ".");

    strcat(desc_holder, "\n\r");

    /* added to stop a memory leak here --Morgenes */
    if (ch->long_descr) {
        free((char *) ch->long_descr);
    }

    ch->long_descr = strdup(desc_holder);
    return TRUE;
}


int
change_objective(const char *new_objective, CHAR_DATA * ch)
{
    char objective_holder[80];

    if (new_objective == NULL)
        return FALSE;

    if (strlen(new_objective) > 70)
        return FALSE;

    objective_holder[0] = '\0';
    strcat(objective_holder, new_objective);

    /* added to stop a memory leak here --Morgenes */
    if (ch->player.info[1]) {
        free((char *) ch->player.info[1]);
    }

    ch->player.info[1] = strdup(objective_holder);
    return TRUE;
}


int
describe_written_language(int language, CHAR_DATA * ch, char *buffer)
{
    if (language == 0) {
        strcpy(buffer, "There is nothing written on it.\n\r");
        return 0;
    } else if (!has_skill(ch, language)) {
        strcpy(buffer, "There's something written on on it in an unknown language.\n\r");
        return 0;
    } else {
        strcpy(buffer, "There's something written on it in ");
        switch (language) {
        case 151:
            strcat(buffer, "sirihish");
            break;
        case 152:
            strcat(buffer, "bendune");
            break;
        case 153:
            strcat(buffer, "allundean");
            break;
        case 154:
            strcat(buffer, "mirukkim");
            break;
        case 155:
            strcat(buffer, "kentu");
            break;
        case 156:
            strcat(buffer, "nrizkt");
            break;
        case 157:
            strcat(buffer, "untal");
            break;
        case 158:
            strcat(buffer, "cavilish");
            break;
        case 159:
            strcat(buffer, "tatlum");
            break;
        case 236:
            strcat(buffer, "anyar");
            break;
        case 238:
            strcat(buffer, "heshrak");
            break;
        case 240:
            strcat(buffer, "vloran");
            break;
        case 242:
            strcat(buffer, "domat");
            break;
        case 244:
            strcat(buffer, "ghaati");
            break;
        default:
            strcat(buffer, "an unknown language");
            break;
        }
    }
    return 1;
}

int
read_item(OBJ_DATA * object, CHAR_DATA * ch, char *buffer)
{
    if (!(object->obj_flags.type == ITEM_NOTE || object->obj_flags.type == ITEM_SCROLL)) {
        strcpy(buffer, "You can't read that.\n\r");
        return 0;
    }

    if (object->obj_flags.type == ITEM_SCROLL) {
        if (describe_written_language(object->obj_flags.value[5], ch, buffer)) {
            strcat(buffer, ", but the characters keep moving making it impossible to read.\n\r");
            return 1;
        }
        return 0;
    }

    if (describe_written_language(object->obj_flags.value[2], ch, buffer)) {
        strcat(buffer, ":\n\r");
    } else
        return 0;

    /* if the character knows it well enough to recognize... */
    if ((ch->skills[object->obj_flags.value[2]]->learned) < 40) {
        strcat(buffer, "You don't know it well enough to make anything " "out, however.\n\r\n\r");
        return 0;
    }

    /* if they get this far, then they can read it */
    return 1;
}


void
show_obj_to_char(OBJ_DATA * object, CHAR_DATA * ch, int mode)
{
    static char buffer[MAX_STRING_LENGTH];

    buffer[0] = '\0';

    bzero(buffer, MAX_STRING_LENGTH);

    if ((mode == 0) && OSTR(object, long_descr)) {
        if (object->in_room && (object->in_room->sector_type == SECT_SILT) &&
            //      (!(object->obj_flags.type == ITEM_FURNITURE &&
            //         IS_SET(object->obj_flags.value[1], FURN_SKIMMER))) &&
            (!(object->obj_flags.type == ITEM_FURNITURE)) && !IS_IMMORTAL(ch))
            return;
        if (IS_SET(object->obj_flags.extra_flags, OFL_HIDDEN))
            return;
    }

    sprintf(buffer, "%s\n\r", format_obj_to_char(object, ch, mode));
    page_string(ch->desc, buffer, 1);

}

sh_int
weapon_valid_for_sheath( OBJ_DATA *weapon, OBJ_DATA *sheath ) {
    char buf[MAX_STRING_LENGTH];

    if( !weapon || !sheath ) return FALSE;

    if( get_obj_extra_desc_value(sheath, "[SHEATH_TYPES]", buf, MAX_STRING_LENGTH) ) {
        if( isanynamebracketsok(buf, OSTR(weapon, name))) {
            return TRUE;
        }
        
    }
    return FALSE;
}


OBJ_DATA *
get_sheathed_weapon(OBJ_DATA *obj) {
    OBJ_DATA *weapon;

    if( !obj ) return NULL;

    if( obj->obj_flags.type != ITEM_CONTAINER ) return NULL;

    if( !obj->contains ) return NULL;

    for( weapon = obj->contains; weapon; weapon = weapon->next_content ) {
        if( weapon_valid_for_sheath( weapon, obj ) ) {
            return weapon;
        }
    }

    return NULL;
}

char *
format_poisoned_obj(OBJ_DATA * object, CHAR_DATA * ch, int ptype)
{
    static char buffer[MAX_STRING_LENGTH];

    /* If they have POISON skill above 70, can tell what type it is,  */
    /* otherwise if have POISON skill can tell it's poisoned at least */
    if ((ptype >= 1)
		&& (((ch->skills[SKILL_POISON] && ch->skills[SKILL_POISON]->learned > 70))
		|| affected_by_spell(ch, SPELL_DETECT_POISON))) {
	sprintf(buffer, "A faint, shiny sheen of %s overlies $p.", poison_color[ptype]);
    } else if ((ptype >= 1) && (ch->skills[SKILL_POISON])) { 
	sprintf(buffer, "A faint, shiny sheen overlies $p.");
    } else {
	return NULL;
    }
    return (buffer);
}

char *
format_obj_to_char(OBJ_DATA * object, CHAR_DATA * ch, int mode)
{
    char buf[MAX_STRING_LENGTH];
    static char buffer[MAX_STRING_LENGTH];
    char pre[MAX_STRING_LENGTH];
    char post[MAX_STRING_LENGTH];
    int temp, scount, ccount, acount;

    buffer[0] = '\0';
    pre[0] = '\0';
    post[0] = '\0';

    if (!object)
        return NULL;

    bzero(buffer, MAX_STRING_LENGTH);
    if ((mode == 0) && OSTR(object, long_descr)) {
        if (object->in_room && (object->in_room->sector_type == SECT_SILT) &&
            //      (!(object->obj_flags.type == ITEM_FURNITURE &&
            //         IS_SET(object->obj_flags.value[1], FURN_SKIMMER))) &&
            (!(object->obj_flags.type == ITEM_FURNITURE)) && !IS_IMMORTAL(ch))
            return NULL;
        if (!object->table && IS_SET(object->obj_flags.extra_flags, OFL_HIDDEN))
            return NULL;

        if (object->occupants)
            return NULL;

        if (object->table) {
            sprintf(buffer, "%s is drawn up to it.", OSTR(object, short_descr));
        } else if (object->ldesc) {
            /* if they included a %s to place the short description */
            if (strstr(object->ldesc, "%s")) {
                sprintf(buf, "%s", format_obj_to_char(object, ch, 1));
                sprintf(buffer, object->ldesc, buf);

                if (!ispunct(object->ldesc[strlen(object->ldesc) - 1]))
                    strcat(buffer, ".");

                *buffer = toupper(*buffer);
                return (buffer);
            }

            sprintf(buffer, "%s is here %s%s", 
             OSTR(object, short_descr), object->ldesc,
             !ispunct(object->ldesc[strlen(object->ldesc) - 1]) ? "." : "");
        } else {
            strcpy(buffer, OSTR(object, long_descr));
        }

        *buffer = toupper(*buffer);

    } else if (OSTR(object, short_descr)
               && ((mode == 1) || (mode == 2) || (mode == 3) || (mode == 4))) {
        strcpy(buffer, OSTR(object, short_descr));
    } else if (mode == 5) {
        if ((object->obj_flags.type == ITEM_LIGHT)) {
            strcpy(buffer, "It looks like a light.");
        } else if ((object->obj_flags.type == ITEM_DRINKCON)) {
            strcpy(buffer, "It looks like a liquid container.");
        } else {                /* ITEM_TYPE != ITEM_DRINKCON */
            strcpy(buffer, "You don't see anything unusual about it.");
        }
    }

    if (mode != 3) {
        /* rearranged order & wording to have it in the descrip of this -Morg */
        if (IS_OBJ_STAT(object, OFL_INVISIBLE)) {
            strcat(pre, "invisible ");  /* was strcat( buffer, " (invisible)" ); */
        }

        if (IS_OBJ_STAT(object, OFL_ETHEREAL)) {
            strcat(pre, "ethereal ");   /* was " (ethereal)" */
        }

        if (IS_OBJ_STAT(object, OFL_GLOW)) {
            strcat(pre, "glowing ");    /* was " (glowing)" */
        }
        if (IS_OBJ_STAT(object, OFL_HUM) && !IS_AFFECTED(ch, CHAR_AFF_DEAFNESS)) {
            strcat(pre, "humming ");    /* was " (humming)" */
        }
        if (IS_OBJ_STAT(object, OFL_MAGIC)
            && (IS_AFFECTED(ch, CHAR_AFF_DETECT_MAGICK)
                || affected_by_spell(ch, PSI_MAGICKSENSE)))
            strcat(pre, "shimmering "); /* was (aura) */
        if (IS_OBJ_STAT(object, OFL_ETHEREAL_OK)
            && (IS_AFFECTED(ch, CHAR_AFF_DETECT_MAGICK)
                || affected_by_spell(ch, PSI_MAGICKSENSE)
                || IS_AFFECTED(ch, CHAR_AFF_DETECT_ETHEREAL)))
            strcat(pre, "shadowy ");

        if (find_ex_description("[SPELL_VAMPIRIC_BLADE]", object->ex_description, TRUE))
            strcat(pre, "decaying ");

        switch (object->obj_flags.type) {
        default:
            break;

        case ITEM_LIGHT:
            {
                int remaining = object->obj_flags.value[0];

                if (object->obj_flags.value[2] == LIGHT_TYPE_MAGICK
                    && (object->obj_flags.value[0] < 0 || object->obj_flags.value[1] < 0)) {
                    break;
                }

                if (object->obj_flags.value[1] == -1 && IS_LIT(object)) {
                    remaining = 10;
                }

                if (!object->obj_flags.value[0] && IS_REFILLABLE(object)) {
                    strcat(pre, "empty ");
                } else if (!object->obj_flags.value[0]
                           && !IS_LIT(object)
                           && (IS_CAMPFIRE(object) || IS_TORCH(object))) {
                    strcat(pre, "ashen ");
                } else {
                    switch (object->obj_flags.value[2]) {
                    case LIGHT_TYPE_ANIMAL:
                        strcat(pre,
                               !IS_LIT(object) ? "" : remaining <= 1 ? "very dim " : remaining <=
                               5 ? "dim " : "shining ");
                        break;
                    case LIGHT_TYPE_FLAME:
                        strcat(pre,
                               !IS_LIT(object) ? "unlit " : remaining <=
                               1 ? "very dim " : remaining <=
                               5 ? "dim " : IS_CAMPFIRE(object) | IS_TORCH(object) ? "burning " :
                               "lit ");
                        break;
                    case LIGHT_TYPE_MAGICK:
                    case LIGHT_TYPE_MINERAL:
                    case LIGHT_TYPE_PLANT:
                        strcat(pre,
                               !IS_LIT(object) ? "" : remaining <= 1 ? "very dim " : remaining <=
                               5 ? "dim " : "glowing ");
                        break;
                    default:
                        strcat(pre,
                               !IS_LIT(object) ? "" : remaining <= 1 ? "very dim " : remaining <=
                               5 ? "dim " : "lit ");
                    }
                }

                if (IS_CAMPFIRE(object)) {
                    strcat(pre,
                           object->obj_flags.value[1] >
                           40 ? "bonfire " : object->obj_flags.value[1] >
                           30 ? "huge " : object->obj_flags.value[1] >
                           20 ? "large " : object->obj_flags.value[1] >
                           10 ? "medium-sized " : object->obj_flags.value[1] <= 4 ? "small " : "");
                }

                break;
            }

        case ITEM_ARMOR:
            /*      if (!object->ldesc) */
            {
                if ((object->obj_flags.value[0] != object->obj_flags.value[3])
                    && object->obj_flags.value[3] > 0) {
                    temp = get_condition(object->obj_flags.value[0], object->obj_flags.value[3]);
                    if ((object->obj_flags.material == MATERIAL_SKIN)
                        || (object->obj_flags.material == MATERIAL_CLOTH)
                        || (object->obj_flags.material == MATERIAL_SILK))
                        sprintf(buf, "%s ", soft_condition[temp]);
                    else
                        sprintf(buf, "%s ", hard_condition[temp]);
                    strcat(pre, buf);
                }
            }
            break;
        case ITEM_FOOD:
            /*      if (!object->ldesc) */
            {
                if (object->obj_flags.value[0]
                    && obj_default[object->nr].value[0]) {
                    int percent = PERCENTAGE(object->obj_flags.value[0],
                                             obj_default[object->nr].value[0]);

                    if (obj_default[object->nr].value[0] < object->obj_flags.value[0])
                        percent = 100;

                    strcat(pre,
                           percent > 90 ? "" : percent > 70 ? "partially eaten " : percent >
                           40 ? "half eaten " : "small portion of ");
                    if (percent <= 40) {
                        char tmp[MAX_STRING_LENGTH];
                        one_argument(OSTR(object, short_descr), tmp, sizeof(tmp));
                        strcat(pre, tmp);
                        strcat(pre, " ");
                    }
                }
            }
            break;
        case ITEM_DRINKCON:
            if ((!object->obj_flags.value[1]) && (mode != 1))
                strcat(pre, "empty ");
            break;

        case ITEM_SPICE: {
            int percent = PERCENTAGE(object->obj_flags.value[0],
                          obj_default[object->nr].value[0]);

            // smoke object
            if( object->obj_flags.value[2] == 1 ) {
                if( percent != 100 ) {
                    strcat( pre, 
                     percent > 90 ? "smoked" 
                      : percent > 60 ? "partially smoked "
                       : percent > 40 ? "half of " : "small portion of ");
                }
            } else {
                if( percent != 100 ) {
                    strcat( pre, 
                     percent > 90 ? "" 
                      : percent > 60 ? "shaved "
                       : percent > 40 ? "half of " : "small portion of ");
                }
            }

            if (percent <= 60) {
                char tmp[MAX_STRING_LENGTH];
                one_argument(OSTR(object, short_descr), tmp, sizeof(tmp));
                strcat(pre, tmp);
                strcat(pre, " ");
            }
            break;
        }

        case ITEM_CONTAINER: {
            OBJ_DATA *sheathed_weapon = NULL;
            if( (sheathed_weapon = get_sheathed_weapon(object)) != NULL ) {
               strcat(post, " with ");
               if( get_obj_extra_desc_value( sheathed_weapon, "[HILT_SDESC]", buf, MAX_STRING_LENGTH) ) {
                  strcat(post, buf);
               } else {
                  strcat(post, "a hilt");
               }
               strcat(post, " protruding from it");
            }
            break;
        }
        }

        scount = 0;

        if (IS_SET(object->obj_flags.state_flags, OST_DUSTY)
            && (scount < 1)) {
            strcat(pre, "dusty ");
            scount++;
        }
        if (IS_SET(object->obj_flags.state_flags, OST_BLOODY)
            && (scount < 1)) {
            strcat(pre, "bloodied ");
            scount++;
        }
        if (IS_SET(object->obj_flags.state_flags, OST_ASH)
            && (scount < 1)) {
            strcat(pre, "ash-covered ");
            scount++;
        }
        if (IS_SET(object->obj_flags.state_flags, OST_SWEATY)
            && (scount < 1)) {
            strcat(pre, "sweat-stained ");
            scount++;
        }
        if (IS_SET(object->obj_flags.state_flags, OST_TORN)
            && (scount < 1)
            && (!IS_SET(object->obj_flags.state_flags, OST_TATTERED))) {
            strcat(pre, "torn ");
            scount++;
        }
        if (IS_SET(object->obj_flags.state_flags, OST_TATTERED)
            && (scount < 1)) {
            strcat(pre, "tattered ");
            scount++;
        }
        if (IS_SET(object->obj_flags.state_flags, OST_STAINED)
            && (scount < 1) && (!IS_SET(object->obj_flags.state_flags, OST_SWEATY))) {
            strcat(pre, "stained ");
            scount++;
        }
        if (IS_SET(object->obj_flags.state_flags, OST_MUDCAKED)
            && (scount < 1)) {
            strcat(pre, "mud-caked ");
            scount++;
        }
        if (IS_SET(object->obj_flags.state_flags, OST_OLD)
            && (scount < 1)) {
            strcat(pre, "old ");
            scount++;
        }
        if (IS_SET(object->obj_flags.state_flags, OST_REPAIRED)
            && (scount < 1)) {
            strcat(pre, "patched ");
            scount++;
        }
        if (IS_SET(object->obj_flags.state_flags, OST_SEWER)
            && (scount < 1)) {
            strcat(pre, "smelly ");
            scount++;
        }
        if (IS_SET(object->obj_flags.state_flags, OST_LACED)
            && (scount < 1)) {
            strcat(pre, "lace-edged ");
            scount++;
        }
        if (IS_SET(object->obj_flags.state_flags, OST_EMBROIDERED)
            && (scount < 1)) {
            strcat(pre, "embroidered ");
            scount++;
        }
        if (IS_SET(object->obj_flags.state_flags, OST_FRINGED)
            && (scount < 1)) {
            strcat(pre, "fringed ");
            scount++;
        }
        if (IS_SET(object->obj_flags.state_flags, OST_BURNED)
            && (scount < 1)) {
            switch (object->obj_flags.material) {
            case MATERIAL_METAL:
            case MATERIAL_STONE:
            case MATERIAL_CHITIN:
            case MATERIAL_OBSIDIAN:
            case MATERIAL_GEM:
            case MATERIAL_GLASS:
            case MATERIAL_BONE:
            case MATERIAL_CERAMIC:
            case MATERIAL_HORN:
            case MATERIAL_GRAFT:
            case MATERIAL_IVORY:
            case MATERIAL_DUSKHORN:
            case MATERIAL_TORTOISESHELL:
                strcat(pre, "blackened ");
                break;
            default:
                strcat(pre, "burned ");
                break;
            }
            scount++;
        }


        ccount = 0;


        if (IS_SET(object->obj_flags.color_flags, OCF_RED)
            && (ccount < 2)) {
            strcat(pre, "red");
            ccount++;
        }
        if (IS_SET(object->obj_flags.color_flags, OCF_BLUE)
            && (ccount < 2)) {
            if (ccount > 0)
                strcat(pre, " and blue");
            else
                strcat(pre, "blue");
            ccount++;
        }
        if (IS_SET(object->obj_flags.color_flags, OCF_YELLOW)
            && (ccount < 2)) {
            if (ccount > 0)
                strcat(pre, " and yellow");
            else
                strcat(pre, "yellow");
            ccount++;
        }
        if (IS_SET(object->obj_flags.color_flags, OCF_GREEN)
            && (ccount < 2)) {
            if (ccount > 0)
                strcat(pre, " and green");
            else
                strcat(pre, "green");
            ccount++;
        }
        if (IS_SET(object->obj_flags.color_flags, OCF_PURPLE)
            && (ccount < 2)) {
            if (ccount > 0)
                strcat(pre, " and purple");
            else
                strcat(pre, "purple");
            ccount++;
        }
        if (IS_SET(object->obj_flags.color_flags, OCF_ORANGE)
            && (ccount < 2)) {
            if (ccount > 0)
                strcat(pre, " and orange");
            else
                strcat(pre, "orange");
            ccount++;
        }
        if (IS_SET(object->obj_flags.color_flags, OCF_BLACK)
            && (ccount < 2)) {
            if (ccount > 0)
                strcat(pre, " and black");
            else
                strcat(pre, "black");
            ccount++;
        }
        if (IS_SET(object->obj_flags.color_flags, OCF_WHITE)
            && (ccount < 2)) {
            if (ccount > 0)
                strcat(pre, " and white");
            else
                strcat(pre, "white");
            ccount++;
        }
        if (IS_SET(object->obj_flags.color_flags, OCF_BROWN)
            && (ccount < 2)) {
            if (ccount > 0)
                strcat(pre, " and brown");
            else
                strcat(pre, "brown");
            ccount++;
        }
        if (IS_SET(object->obj_flags.color_flags, OCF_GREY)
            && (ccount < 2)) {
            if (ccount > 0)
                strcat(pre, " and grey");
            else
                strcat(pre, "grey");
            ccount++;
        }
        if (IS_SET(object->obj_flags.color_flags, OCF_SILVERY)
            && (ccount < 2)) {
            if (ccount > 0)
                strcat(pre, " and silvery");
            else
                strcat(pre, "silvery");
            ccount++;
        }

        acount = 0;

        if (ccount > 0) {
            if (IS_SET(object->obj_flags.adverbial_flags, OAF_BATIKED)
                && (acount < 1)) {
                acount++;
                strcat(pre, " batiked ");
            }
            if (IS_SET(object->obj_flags.adverbial_flags, OAF_STRIPED)
                && (acount < 1)) {
                acount++;
                strcat(pre, "-striped ");
            }
            if (IS_SET(object->obj_flags.adverbial_flags, OAF_DYED)
                && (acount < 1)) {
                acount++;
                strcat(pre, "-dyed ");
            }
            if (IS_SET(object->obj_flags.adverbial_flags, OAF_PATTERNED)
                && (acount < 1)) {
                acount++;
                strcat(pre, " patterned ");
            }
            if (IS_SET(object->obj_flags.adverbial_flags, OAF_CHECKERED)
                && (acount < 1)) {
                acount++;
                strcat(pre, " checkered ");
            }
        }

        if (acount == 0) {
            if (ccount == 1)
                strcat(pre, " ");
            else if (ccount == 2)
                strcat(pre, ", ");
        }

        // pre and maybe post
        if (pre[0] != '\0') {
            sprintf(buffer, "%s%s%s%s%s", indefinite_article(pre), pre,
             mode == 0 && post[0] == '\0' ? smash_article(buffer) : smash_article(OSTR(object, short_descr)), 
             post[0] != '\0' ? post : "",
             mode == 0 && post[0] != '\0' ? " is here." : "");
        } 
        // pre only
        else if( post[0] != '\0' ) {
            sprintf(buffer, "%s%s%s", OSTR(object, short_descr), post,
             mode == 0 ? " is here." : "");
        }

        if (mode == 0) {
            *buffer = toupper(*buffer);
        }

        /* append the title */
        if (mode == 2 && object->obj_flags.type == ITEM_NOTE) {
            char *title;

            /* if they can read the language */
            /* and if it has a title */
            if (has_skill(ch, object->obj_flags.value[2])
                && ch->skills[object->obj_flags.value[2]]->learned >= 40
                && (title =
                    find_ex_description("[BOOK_TITLE]", object->ex_description, TRUE)) != NULL) {
                sprintf(pre, ", \"%s\"", title);
                sprintf(buffer, "%s%s", buffer, pre);
            }
        }
    }

    return (buffer);
}

/*
 * New version of pluralize, more verbose
 */
char *
pluralize(char *argument)
{
    static char buf[MAX_STRING_LENGTH];
    char *v, *v2, *v3, *v4, *v5;
    char xbuf[MAX_STRING_LENGTH];

    sprintf(buf, "%s", smash_article(argument));

    if ((v = strstr(buf, "some "))) {
        char repl[] = "sets of some ";
        memmove(buf + strlen(repl), v+5, strlen(v+5) + 1);
        strncpy(buf, repl, sizeof(repl) - 1);
        return buf;
    }

    v = strstr(buf, " of ");
    v2 = strstr( buf, " set with ");
    v3 = strstr(buf, " with ");
    v4 = strstr(buf, " made of ");
    v5 = strstr(buf, " marked with ");

    if(v4) {
       v = v4;
    }
    if ((v2 && !v) || (v2 && v && strlen(v2) > strlen(v))) {
        v = v2;
    }
    else if ((v5 && !v) || (v5 && v && strlen(v5) > strlen(v))) {
        v = v5;
    }
    else if ((v3 && !v) || (v3 && v && strlen(v3) > strlen(v))) {
        v = v3;
    }

    xbuf[0] = '\0';
    if (v != NULL) {
        sprintf(xbuf, "%s", v);
        buf[strlen(buf) - strlen(v)] = '\0';
    }

    if ((v = strstr(buf, "tooth")) && v[5] == 0) {
        v[1] = v[2] = 'e';
    } else if ((v = strstr(buf, "shelves")) && v[7] == 0) {
        return buf;
    } else if (!strcmp(buf, "goose")) {
        buf[1] = buf[2] = 'e';
    } else if((v = strstr( buf, "cuff")) && v[4] == 0) {
        strcat(buf, "s");
    }else if (LOWER(buf[strlen(buf) - 1]) == 'f') {
        if (LOWER(buf[strlen(buf) - 2]) == 'f') {
            buf[strlen(buf) - 2] = 'v';
            buf[strlen(buf) - 1] = 'e';
            strcat(buf, "s");
        } else {
            buf[strlen(buf) - 1] = 'v';
            strcat(buf, "es");
        }
    } else if (LOWER(buf[strlen(buf) - 2]) == 'f'
      && LOWER(buf[strlen(buf) - 1]) == 'e') {
        buf[strlen(buf) - 2] = 'v';
        strcat(buf, "s");
    }

    /* 'th' should end in 's' not 'es'
     * if a 'th' word comes up that should be pluralized with 'es'
     * this should be extended to make sure 'length' and 'cloth' are handled
     else if (LOWER (buf[strlen (buf) - 1]) == 'h'
     && LOWER (buf[strlen (buf) - 2]) == 't')
     {
     strcat (buf, "es");
     }
     */

    else if (LOWER(buf[strlen(buf) - 1]) == 'h' && LOWER(buf[strlen(buf) - 2]) == 's') {
        strcat(buf, "es");
    } else if (LOWER(buf[strlen(buf) - 1]) == 'h' && LOWER(buf[strlen(buf) - 2]) == 'c') {
        strcat(buf, "es");
    } else if (LOWER(buf[strlen(buf) - 1]) == 's' && LOWER(buf[strlen(buf) - 2]) == 'e'
               && LOWER(buf[strlen(buf) - 3]) == 'v') {
        strcat(buf, "");
    } else if (LOWER(buf[strlen(buf) - 1]) == 'x') {
        strcat(buf, "es");
    } else if (LOWER(buf[strlen(buf) - 1]) == 's') {
        if (LOWER(buf[strlen(buf) - 2]) == 'u' && !IS_VOWEL(LOWER(buf[strlen(buf) - 3]))) {
            buf[strlen(buf) - 2] = 'i';
            buf[strlen(buf) - 1] = '\0';
        } else
            strcat(buf, "es");
    } else if (LOWER(buf[strlen(buf) - 1]) == 'y' && !IS_VOWEL(LOWER(buf[strlen(buf) - 2]))) {
        buf[strlen(buf) - 1] = 'i';
        strcat(buf, "es");
    } else
        strcat(buf, "s");

    strcat(buf, xbuf);
    return buf;
}


void
show_obj_to_char_new(OBJ_DATA * object, CHAR_DATA * ch, int mode)
{
    char buf[256], buffer[MAX_STRING_LENGTH];
    int temp;
    bool found;

    gamelog("show_obj_to_char_new");
    bzero(buffer, MAX_STRING_LENGTH);
    if ((mode == 0) && OSTR(object, long_descr)) {
        if (object->in_room && (object->in_room->sector_type == SECT_SILT) &&
            //      (!(object->obj_flags.type == ITEM_FURNITURE &&
            //         IS_SET(object->obj_flags.value[1], FURN_SKIMMER))) &&
            (!(object->obj_flags.type == ITEM_FURNITURE)) && !IS_IMMORTAL(ch))
            return;
        if (IS_SET(object->obj_flags.extra_flags, OFL_HIDDEN))
            return;

        //
        //      if (find_ex_description ("[UNIQUE_LDESC]", object->ex_description, TRUE))
        //        {
        //          gamelog ("Found unique_ldesc");
        //          strcpy (buffer, find_ex_description("[UNIQUE_LDESC]", object->ex_description, TRUE));
        //        }
        //      else
        //

        strcpy(buffer, OSTR(object, long_descr));

        /* CAP(buffer); */
        *buffer = toupper(*buffer);
    } else if (OSTR(object, short_descr)
               && ((mode == 1) || (mode == 2) || (mode == 3) || (mode == 4)))
        //
        //      if (find_ex_description ("[UNIQUE_SDESC]", object->ex_description, TRUE))
        //              {
        //          gamelog ("Found unique_sdesc");
        //          strcpy (buffer, find_ex_description("[UNIQUE_SDESC]", object->ex_description, TRUE));
        //        }
        //      else
        //
        strcpy(buffer, OSTR(object, short_descr));
    else if (mode == 5) {
        if ((object->obj_flags.type == ITEM_LIGHT)) {
            strcpy(buffer, "It looks like a light source.");
        } else if ((object->obj_flags.type == ITEM_DRINKCON)) {
            strcpy(buffer, "It looks like a liquid container.");
        } else {                /* ITEM_TYPE != ITEM_DRINKCON */
            strcpy(buffer, "You don't see anything unusual about it.");
        }
    }
    if (mode != 3) {
        found = FALSE;
        if (object->obj_flags.type == ITEM_ARMOR) {
            temp = get_condition(object->obj_flags.value[0], object->obj_flags.value[3]);
            if ((object->obj_flags.material == MATERIAL_SKIN)
                || (object->obj_flags.material == MATERIAL_CLOTH)
                || (object->obj_flags.material == MATERIAL_SILK))
                sprintf(buf, " (%s)", soft_condition[temp]);
            else
                sprintf(buf, " (%s)", hard_condition[temp]);
            strcat(buffer, buf);
        }
        if (IS_OBJ_STAT(object, OFL_INVISIBLE)) {
            strcat(buffer, " (invisible)");
            found = TRUE;
        }
        if (IS_OBJ_STAT(object, OFL_ETHEREAL)) {
            strcat(buffer, " (ethereal)");
            found = TRUE;
        }
    }
    strcat(buffer, "\n\r");
    page_string(ch->desc, buffer, 1);
}

void
old_list_obj_to_char(OBJ_DATA * list, CHAR_DATA * ch, int mode, bool show)
{
    OBJ_DATA *i;
    bool found;

    found = FALSE;
    for (i = list; i; i = i->next_content) {
        if (CAN_SEE_OBJ(ch, i)) {
            show_obj_to_char(i, ch, mode);
            found = TRUE;
        }
    }
    if ((!found) && (show))
        send_to_char("Nothing\n\r", ch);
}

int
get_bulk_penalty(OBJ_DATA *obj, CHAR_DATA *ch) {
    int wt = GET_OBJ_WEIGHT(obj);
    int size = get_char_size(ch);
    char buf[MAX_STRING_LENGTH];

    if (wt > size * 4.5)
        return 100;
    if (wt > size * 3.5 || IS_CORPSE(obj))
        return 75;
    if (wt > size * 2.5 )
        return 50;
    if (wt > size * 1.5
     || get_obj_extra_desc_value(obj, "[BULKY]", buf, MAX_STRING_LENGTH) )
        return 25;
    return 0;
}

int 
get_char_bulk_penalty(CHAR_DATA *ch) {
   int penalty = 0;
   OBJ_DATA *obj;

   for( obj = ch->carrying; obj; obj = obj->next_content ) {
       penalty = MAX(penalty, get_bulk_penalty(obj, ch));
   }

   return penalty;
}

bool
is_bulky_for_ch(OBJ_DATA * obj, CHAR_DATA *ch, int bulk_distance)
{
    char buf[MAX_STRING_LENGTH];

    if( bulk_distance == -1 ) return FALSE;

    if( IS_CORPSE(obj) ) return TRUE;

    if( get_obj_extra_desc_value(obj, "[BULKY]", buf, MAX_STRING_LENGTH) ) 
        return TRUE;

    if (GET_OBJ_WEIGHT(obj) > get_char_size(ch) * (bulk_distance + 1.5))
        return TRUE;
    return FALSE;
}

char *
list_bulky_obj_to_char(OBJ_DATA * list, CHAR_DATA * ch, int mode, bool show, int bulk_distance )
{
    char buf[MAX_STRING_LENGTH];
    static char final[MAX_STRING_LENGTH];
    char **prgpstrShow;
    int *prgnShow;
    char *pstrShow;
    OBJ_DATA **lastobj;
    OBJ_DATA *obj;
    int nShow;
    int iShow;
    int count;
    bool fCombine;

    /*
     * Alloc space for output lines.
     */
    count = 0;
    for (obj = list; obj != NULL; obj = obj->next_content)
        count++;

    /* if count is 0, then these malloc(0), so adding 1 to count,    */
    /*  so there isnt some accident crappage going on, particularly  */
    /*  on the frees at the end of the function.                     */
    /* -Tene  5/17/98                                                */

    prgpstrShow = malloc((count + 1) * sizeof(char *));
    prgnShow = malloc((count + 1) * sizeof(int));
    lastobj = malloc((count + 1) * sizeof(OBJ_DATA *));
    nShow = 0;

    final[0] = '\0';
    /*
     * Format the list of objects.
     */
    for (obj = list; obj != NULL; obj = obj->next_content) {
        if (CAN_SEE_OBJ(ch, obj)) {
            if ((mode == 3 || mode == 4) 
             && !is_bulky_for_ch(obj, obj->carried_by, bulk_distance))
                continue;

            pstrShow = format_obj_to_char(obj, ch, mode);

            if (!pstrShow)
                continue;

            if (strstr(pstrShow, "(no description)"))
                continue;

            /* if( obj->occupants )
             * continue; */

            fCombine = FALSE;

            /*
             * Look for duplicates, case sensitive.
             * Matches tend to be near end so run loop backwords.
             */
            for (iShow = nShow - 1; iShow >= 0; iShow--) {
                if (!strcmp(prgpstrShow[iShow], pstrShow)
                    || lastobj[iShow] == obj) {
                    prgnShow[iShow]++;
                    lastobj[iShow] = obj;
                    fCombine = TRUE;
                    break;
                }
            }

            /*
             * Couldn't combine, or didn't want to.
             */
            if (!fCombine) {
                prgpstrShow[nShow] = strdup(pstrShow);
                prgnShow[nShow] = 1;
                lastobj[nShow] = obj;
                nShow++;
            }
        }
    }

    /*
     * Output the formatted list.
     */
    for (iShow = 0; iShow < nShow; iShow++) {
        if (iShow > 0) {
            if (mode == 2 || mode == 1) {
                /* this is for the aber version, paragraphs  */
                if (IS_SET(ch->specials.brief, BRIEF_EQUIP)) {
                    if (iShow > 0 && iShow < nShow - 1)
                        strcat(final, ", ");
                    else if (iShow == nShow - 1)
                        strcat(final, ", and ");
                } else {
                    strcat(final, "\n\r");
                }
            } else if (mode != 3) {
                strcat(final, "\n\r");
            }
        }

        if (mode == 3) {
            sprintf(buf, "- %s is carrying ", HSSH(lastobj[iShow]->carried_by));
            strcat(final, buf);
        }

        if (prgnShow[iShow] > 1) {
            if (mode == 2 || mode == 3 || mode == 4) {
                sprintf(buf, "%s ", numberize(prgnShow[iShow]));
            } else {
                if (lastobj[iShow]->table) {
                    sprintf(buf, "%s %s are drawn up to it.", numberize(prgnShow[iShow]),
                            pluralize(format_obj_to_char(lastobj[iShow], ch, 2)));
                } else {
                    if (lastobj[iShow]->ldesc && strstr(lastobj[iShow]->ldesc, "%s")) {
                        char buffer[MAX_STRING_LENGTH];

                        sprintf(buffer, "%s %s", numberize(prgnShow[iShow]),
                                pluralize(format_obj_to_char(lastobj[iShow], ch, 2)));
                        sprintf(buf, lastobj[iShow]->ldesc, buffer);

                        if (!ispunct(lastobj[iShow]->ldesc[strlen(lastobj[iShow]->ldesc) - 1]))
                            strcat(buf, ".");
                    } else {
                        sprintf(buf, "%s %s are here%s%s%s", numberize(prgnShow[iShow]),
                                pluralize(format_obj_to_char(lastobj[iShow], ch, 2)),
                                lastobj[iShow]->ldesc ? " " : "",
                                lastobj[iShow]->ldesc ? lastobj[iShow]->ldesc : "",
                                lastobj[iShow]->ldesc
                                && ispunct(lastobj[iShow]->
                                           ldesc[strlen(lastobj[iShow]->ldesc) - 1]) ? "" : ".");
                    }
                }
                buf[0] = UPPER(buf[0]);
            }

            if (mode == 4 || mode == 3 || mode == 2 || mode == 1) {
                string_safe_cat(final, buf, MAX_STRING_LENGTH);
            } else {
                send_to_char(buf, ch);
            }
        }

        if (mode == 4 || mode == 3 || mode == 2 || mode == 1) {
            strcat(final, prgnShow[iShow] > 1 ? pluralize(prgpstrShow[iShow]) : prgpstrShow[iShow]);
            if (mode == 3) {
                strcat(final, ".\n\r");
            }
        } else {
            if (prgnShow[iShow] <= 1) {
                send_to_char(prgpstrShow[iShow], ch);
            }
            send_to_char("\n\r", ch);
        }

        free(prgpstrShow[iShow]);
        lastobj[iShow] = NULL;
    }

    if (show && nShow == 0) {
        string_safe_cat(final, "nothing", MAX_STRING_LENGTH);

        if (mode == 4)
            string_safe_cat(final, " obvious", MAX_STRING_LENGTH);
    }

    /*
     * Clean up.
     */
    free(prgpstrShow);
    free(prgnShow);
    free(lastobj);

    if (mode == 4 || mode == 3 || mode == 2 || mode == 1) {
        return (final);
    }
    return (NULL);
}

char *
list_obj_to_char(OBJ_DATA * list, CHAR_DATA * ch, int mode, bool show) {
   return list_bulky_obj_to_char( list, ch, mode, show, -1 );
}


/*
 * Return either the string representing the number, or the number as a string
 */
char *
numberize(int n)
{
    static char buf[MAX_STRING_LENGTH];

    if (n == 1)
        sprintf(buf, "one");
    else if (n == 2)
        sprintf(buf, "a couple of");
    else if (n >= 3 && n <= 5)
        sprintf(buf, "a few");
    else if (n >= 6 && n <= 9)
        sprintf(buf, "some");
    else if (n >= 10 && n <= 15)
        sprintf(buf, "several");
    else
        sprintf(buf, "many");

    return buf;
}




void
show_eco_to_galith(CHAR_DATA * ch, CHAR_DATA * galith)
{
    int warmth = ch->specials.eco / 10;
    char buf[256];
    char msg[256];

    switch (warmth) {
    case (-10):
        sprintf(buf, "completely frozen");
        break;
    case (-9):
        sprintf(buf, "nearly frozen");
        break;
    case (-8):
        sprintf(buf, "frigid");
        break;
    case (-7):
        sprintf(buf, "very cold");
        break;
    case (-6):
        sprintf(buf, "cold");
        break;
    case (-5):
        sprintf(buf, "very chilly");
        break;
    case (-4):
        sprintf(buf, "chilly");
        break;
    case (-3):
        sprintf(buf, "a little chilly");
        break;
    case (-2):
        sprintf(buf, "cool");
        break;
    case (-1):
    case (-0):
    case (1):
        sprintf(buf, "neutral");
        break;
    case (2):
        sprintf(buf, "lukewarm");
        break;
    case (3):
        sprintf(buf, "a little warm");
        break;
    case (4):
        sprintf(buf, "warm");
        break;
    case (5):
        sprintf(buf, "very warm");
        break;
    case (6):
        sprintf(buf, "hot");
        break;
    case (7):
        sprintf(buf, "very hot");
        break;
    case (8):
        sprintf(buf, "seering");
        break;
    case (9):
    case (10):
        sprintf(buf, "infernal");
        break;
    default:
        sprintf(buf, "*error %d- please report to an imm*", warmth);
        break;
    }

    sprintf(msg, "The balance of %s is %s.\n", PERS(galith, ch), buf);
    send_to_char(msg, galith);

    return;
}

static int sorted_eq[MAX_WEAR] = {

    /*  6,  20,  21, 3,  4,  */
    /*  5,  10,  14, 15, 9, */
    /* 16, 17,  19, 1,  2, */
    /*  0,  11,  18, 12, 13, */
    /*  7,  22,  8,  23, 24, */
    /*  25 */
    WEAR_HEAD,
    WEAR_ABOUT_HEAD,
    WEAR_IN_HAIR,
    WEAR_FACE,
    WEAR_LEFT_EAR,
    WEAR_RIGHT_EAR,
    WEAR_NECK,
    WEAR_ABOUT_THROAT,
    WEAR_ON_BACK,
    WEAR_BACK,
    WEAR_BODY,
    WEAR_OVER_SHOULDER_R,
    WEAR_SHOULDER_R,
    WEAR_OVER_SHOULDER_L,
    WEAR_SHOULDER_L,
    WEAR_ARMS,
    WEAR_WRIST_R,
    WEAR_WRIST_L,
    WEAR_HANDS,
    EP,
    ES,
    ETWO,
    WEAR_FOREARMS,
    WEAR_FINGER_R,
    WEAR_FINGER_L,
    WEAR_FINGER_R2,
    WEAR_FINGER_L2,
    WEAR_FINGER_R3,
    WEAR_FINGER_L3,
    WEAR_FINGER_R4,
    WEAR_FINGER_L4,
    WEAR_FINGER_R5,
    WEAR_FINGER_L5,
    WEAR_BELT,
    WEAR_ON_BELT_1,
    WEAR_ON_BELT_2,
    WEAR_ABOUT,
    WEAR_WAIST,
    WEAR_LEGS,
    WEAR_ANKLE,
    WEAR_ANKLE_L,
    WEAR_FEET,


};


static int show_tribe_affiliation[] = { 7, 11, 24, 25, 0 };

void show_equipment(CHAR_DATA *ch, CHAR_DATA *tch);

int get_char_distance(CHAR_DATA *ch, CHAR_DATA *vict ) {
    if( !ch || !vict || !ch->in_room || !vict->in_room ) return -1;
    if( ch->in_room == vict->in_room ) return 0;
    return generic_astar(ch->in_room, vict->in_room, 3, 30.0f, 0, 0, 0, 0, 0, 0, 0, 0);
}

void
show_char_to_char(CHAR_DATA * i, CHAR_DATA * ch, int mode)
{
    char buffer[MAX_STRING_LENGTH];
    char show_mess[256];
    char buf[MAX_STRING_LENGTH];
    char ball_buf[MAX_STRING_LENGTH];
    char bufy[MAX_STRING_LENGTH];
    char *temp_desc;
    char *tmp_string;
    char *det_mag_msg = 0;
    char *det_invis_msg = 0;
    char *det_eth_msg = 0;
    int l, j, found, percent, temp, counter, powerlevel, chance;
    int disp = 0;
    OBJ_DATA *tmp;
    char *tmp_desc;
    CHAR_DATA *rider;
    CHAR_DATA *subduer;
    struct affected_type *af;

    int ct;

    if (mode == 0) {
        if (i->specials.riding)
            return;

        for (rider = i->in_room->people; rider; rider = rider->next_in_room)
            if (rider->specials.riding == i)
                break;
        if (ch == rider)
            return;

        if (rider) {
            if (IS_AFFECTED(i, CHAR_AFF_INVISIBLE) && is_char_ethereal(i))
                strcat(buffer, "<*> ");
            else {
                if (IS_AFFECTED(i, CHAR_AFF_INVISIBLE))
                    strcpy(buffer, "* ");
                else
                    *buffer = '\0';
                if (is_char_ethereal(i))
                    strcat(buffer, "<> ");
            }

#define AZROEN_INVIS_RIDER 1
#ifdef AZROEN_INVIS_RIDER
            if (CAN_SEE(ch, rider)) {
                strcat(buffer, "$N stands here, carrying ");
                if (IS_AFFECTED(rider, CHAR_AFF_INVISIBLE) && is_char_ethereal(rider))
                    strcat(buffer, "<*> ");
                else {
                    if (IS_AFFECTED(rider, CHAR_AFF_INVISIBLE))
                        strcat(buffer, "* ");
                    if (is_char_ethereal(rider))
                        strcat(buffer, "<> ");
                }
                sprintf(buf, "%s on $S back.", PERS(ch, rider));
            }
#else
            if (CAN_SEE(ch, rider)) {
                sprintf(buf, "$N stands here, carrying %s on $S back.", PERS(ch, rider));
            }
#endif
            else if (i->long_descr) {
                sprintf(buf, "%s %s", PERS(ch, i), i->long_descr);
            } else
                sprintf(buf, "%s stands here.", PERS(ch, i));

            strcat(buffer, buf);
            /*  CAP(buffer); */
            *buffer = toupper(*buffer);
            act(buffer, FALSE, ch, 0, i, TO_CHAR);
            return;
        }

        if (!CAN_SEE(ch, i))
            return;

        for (subduer = i->in_room->people; subduer; subduer = subduer->next_in_room)
            if ((subduer != ch) && (subduer->specials.subduing == i))
                break;

        /* if they're not being subdued,
         * and subduing someone
         * that ch can see,
         * and is in the same room,
         * skip them, they'll be handled when the subdued person is shown
         */
        if (!subduer && i->specials.subduing && CAN_SEE(ch, i->specials.subduing)
            && i->specials.subduing->in_room == i->in_room)
            return;


        if (subduer || 
            i->specials.subduing) {
          if (IS_AFFECTED(i, CHAR_AFF_INVISIBLE) && is_char_ethereal(i)) {
            strcat(buffer, "<*> ");
          } else {
            if (IS_AFFECTED(i, CHAR_AFF_INVISIBLE)) {
              strcpy(buffer, "* ");
            } else {
              *buffer = '\0';
            }
            
            if (is_char_ethereal(i)) {
              strcat(buffer, "<> ");
            }
          }
          strcat(buffer, PERS(ch, i));
          
          switch (GET_POS(i)) {
          case POSITION_STUNNED:
            strcat(buffer, " is lying on the ground stunned, ");
            break;
          case POSITION_MORTALLYW:
            strcat(buffer, " is lying on the ground, mortally wounded, ");
            break;
          case POSITION_DEAD:
            strcat(buffer, " is lying here, dead,");
            break;
          case POSITION_STANDING:
            temp = is_flying_char(i);
            if (temp >= FLY_LEVITATE) {
              if (temp == FLY_LEVITATE)
                strcat(buffer, " is floating here, ");
              else
                strcat(buffer, " is flying here, ");
            } else {
              if (affected_by_spell(i, SPELL_ANIMATE_DEAD))
                strcat(buffer, " slowly staggers around, ");
                    else {
                      strcat(buffer, " is standing ");
                        OBJ_DATA *furn = i->on_obj;

                        if (furn && !furn->table) {
                            if (IS_SET(furn->obj_flags.value[1], FURN_SKIMMER)
                             || IS_SET(furn->obj_flags.value[1], FURN_WAGON)) {
                                sprintf(buf, "on %s", 
                                 format_obj_to_char(furn, ch, 1));
                            } else {
                                sprintf(buf, "at %s", 
                                 format_obj_to_char(furn, ch, 1));
                            }
                            strcat(buffer, buf);
                        } else if (furn && furn->table) {
                            sprintf(buf, "at %s", 
                             format_obj_to_char(furn->table, ch, 1));
                            strcat(buffer, buf);
                        } else
                            strcat(buffer, "here");
                    }
                }
                break;
            case POSITION_SITTING:
                strcat(buffer, " is sitting ");
                break;
            case POSITION_RESTING:
                strcat(buffer, " is here resting, ");
                break;
            case POSITION_SLEEPING:
                strcat(buffer, " is sleeping here, ");
                break;
            case POSITION_FIGHTING:
                if (i->specials.fighting) {
                    if( trying_to_disengage(i)) {
                        strcat(buffer, " is here, defending against ");
                    } else {
                        strcat(buffer, " is here, fighting ");
                    }
                    if (i->specials.fighting == ch)
                        strcat(buffer, "you");
                    else
                        strcat(buffer, PERS(ch, i->specials.fighting));
                    strcat(buffer, " ");
                } else
                    strcat(buffer, " stands here, ");
                break;
            default:
                strcat(buffer, " is floating here, ");
                break;
            }                   /* End switch */

            if (i->specials.subduing) {
                sprintf(buf, " holding %s", PERS(ch, i->specials.subduing));
                disp = 1;
            }

            if (subduer && subduer != ch) {
                if (disp) {
                    strcat(buffer, buf);
                    strcat(buffer, ", and ");
                }
                strcpy(buf, " held by $N");
                disp = 1;
            }

            if (subduer && subduer == ch) {
                if (disp) {
                    strcat(buffer, buf);
                    strcat(buffer, ", and ");
                }
                strcpy(buf, " held by you");
                disp = 1;
            }

            strcat(buffer, buf);
            strcat(buffer, ".");
            *buffer = toupper(*buffer);
            act(buffer, FALSE, ch, 0, subduer, TO_CHAR);
            return;
        }

        /*  new_ldesc begin  */
        if ((!IS_NPC(i) && !i->long_descr)
            /* default for pcs w/out ldesc */
            /* masks, hoods, etc on npc's */
            || (IS_NPC(i) && strcmp(PERS(ch, i), MSTR(i, short_descr)))
            /* !ldesc and sdesc and sdesc != default */
            || (IS_NPC(i) && !i->long_descr && i->short_descr
                && strcmp(i->short_descr, npc_default[i->nr].short_descr))
            /* peraine poison */
            || is_paralyzed(i)
            /* lifting something */
            || i->lifting
            /* on something */
            || i->on_obj
            /* npc not in default position */
            || (IS_NPC(i) && !i->long_descr && (GET_POS(i) != i->specials.default_pos))) {
            strcpy(buffer, "");
            if (IS_AFFECTED(i, CHAR_AFF_INVISIBLE) && is_char_ethereal(i))
                strcat(buffer, "<*> ");
            else {
                if (IS_AFFECTED(i, CHAR_AFF_INVISIBLE))
                    strcat(buffer, "* ");
                if (is_char_ethereal(i))
                    strcat(buffer, "<> ");
            }
            strcat(buffer, PERS(ch, i));

            if (IS_CREATING(i))
                strcat(buffer, " [CREATING]");

            if (IS_AFFECTED(i, CHAR_AFF_HIDE))
                strcat(buffer, " is hiding here");
            else
                switch (GET_POS(i)) {
                case POSITION_STUNNED:
                    strcat(buffer, " lies on the ground stunned, unable to move.");
                    break;
                case POSITION_MORTALLYW:
                    strcat(buffer, " lies here mortally wounded, on the verge of death.");
                    break;
                case POSITION_DEAD:
                    strcat(buffer, " is lying here, dead.");
                    break;
                case POSITION_STANDING:
                    temp = is_flying_char(i);
                    if (temp >= FLY_LEVITATE) {
                        if (temp == FLY_LEVITATE)
                            strcat(buffer, " is floating here");
                        else
                            strcat(buffer, " is flying here");
                    } else {
                        if (affected_by_spell(i, SPELL_ANIMATE_DEAD))
                            strcat(buffer, " slowly staggers around");
                        else {
                            strcat(buffer, " is standing ");
                            OBJ_DATA *furn = i->on_obj;

                            if (furn && !furn->table) {
                                if (IS_SET(furn->obj_flags.value[1], FURN_SKIMMER)
                                 || IS_SET(furn->obj_flags.value[1], FURN_WAGON)) {
                                    sprintf(buf, "on %s", 
                                     format_obj_to_char(furn, ch, 1));
                                } else {
                                    sprintf(buf, "at %s", 
                                     format_obj_to_char(furn, ch, 1));
                                }
                                strcat(buffer, buf);
                            } else if (furn && furn->table) {
                                sprintf(buf, "at %s", 
                                 format_obj_to_char(furn->table, ch, 1));
                                strcat(buffer, buf);
                            } else
                                strcat(buffer, "here");
                        }
                    }
                    break;
                case POSITION_SITTING:
                    strcat(buffer, " is sitting ");
                    if (i->on_obj && !i->on_obj->table) {
                        sprintf(buf, "on %s", format_obj_to_char(i->on_obj, ch, 1));
                        strcat(buffer, buf);
                    } else if (i->on_obj && i->on_obj->table) {
                        sprintf(buf, "at %s", format_obj_to_char(i->on_obj->table, ch, 1));
                        strcat(buffer, buf);
                    } else
                        strcat(buffer, "here");
                    break;
                case POSITION_RESTING:
                    strcat(buffer, " is reclining ");
                    if (i->on_obj && !i->on_obj->table) {
                        sprintf(buf, "on %s", format_obj_to_char(i->on_obj, ch, 1));
                        strcat(buffer, buf);
                    } else if (i->on_obj && i->on_obj->table) {
                        sprintf(buf, "at %s", format_obj_to_char(i->on_obj->table, ch, 1));
                        strcat(buffer, buf);
                    } else
                        strcat(buffer, "here");
                    break;
                case POSITION_SLEEPING:
                    strcat(buffer, " is sleeping ");
                    if (i->on_obj && !i->on_obj->table) {
                        sprintf(buf, "on %s", format_obj_to_char(i->on_obj, ch, 1));
                        strcat(buffer, buf);
                    } else if (i->on_obj && i->on_obj->table) {
                        sprintf(buf, "at %s", format_obj_to_char(i->on_obj->table, ch, 1));
                        strcat(buffer, buf);
                    } else
                        strcat(buffer, "here");
                    break;
                case POSITION_FIGHTING:
                    if (i->specials.fighting) {
                        if( trying_to_disengage(i)) {
                            strcat(buffer, " is here, defending against ");
                        } else {
                            strcat(buffer, " is here, fighting ");
                        }

                        if (i->specials.fighting == ch)
                            strcat(buffer, "you");
                        else
                            strcat(buffer, PERS(ch, i->specials.fighting));
                        strcat(buffer, ".");
                    } else
                        strcat(buffer, " is standing here.");
                    break;
                default:
                    strcat(buffer, " is floating here.");
                    break;
                }
            if ((GET_POS(i) == POSITION_FIGHTING) || (GET_POS(i) <= POSITION_STUNNED)) {
                strcat(buffer, "\n\r");
                /* CAP(buffer); */
                *buffer = toupper(*buffer);
                send_to_char(buffer, ch);
            } else {
                if (is_paralyzed(i))
                    strcat(buffer, ", rigid and unmoving");

                if (i->lifting) {
                    sprintf(buf, ", lifting %s", format_obj_to_char(i->lifting, ch, 1));
                    strcat(buffer, buf);
                }

                if (GET_MAX_HIT(i) > 0)
                    percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
                else
                    percent = -1;       /* How could MAX_HIT be < 1?? */
                if (percent <= 90) {    /* Add wounded in their ldescs */
                    if (percent <= 10)
                        strcat(buffer, ", looking near-death");
                    else {
                        if (percent <= 40)
                            strcat(buffer, ", bleeding profusely");
                        else {
                            if (percent <= 70)
                                strcat(buffer, ", bleeding heavily");
                            else
                                strcat(buffer, ", bleeding lightly");
                        }       /* End else .4 */
                    }           /* End else .1 */
                } /* End wounded ldesc */
                else {
                    if (GET_MAX_MOVE(i) > 0)
                        percent = (100 * GET_MOVE(i)) / GET_MAX_MOVE(i);
                    else
                        percent = -1;   /* How could MAX_MOVE be < 1?? */
                    if (percent <= 80 && ch->in_room && i->in_room && (ch->in_room == i->in_room)) {    /* Add tired in ldesc */
                        if (percent <= 10)
                            strcat(buffer, ", looking exhausted");
                        else if (percent <= 30)
                            strcat(buffer, ", appearing very tired");
                        else if (percent <= 60)
                            strcat(buffer, ", looking tired");
                        else
                            strcat(buffer, ", looking a bit winded");
                    }           /* End tired ldesc */
                }
                strcat(buffer, ".");
                strcat(buffer, "\n\r");
                /* CAP(buffer); */
                *buffer = toupper(*buffer);
                send_to_char(buffer, ch);
            }                   /* end not position fighting or < stunned */
        } else {
            if (IS_AFFECTED(i, CHAR_AFF_INVISIBLE) && is_char_ethereal(i))
                strcpy(buffer, "<*> ");
            else {
                if (IS_AFFECTED(i, CHAR_AFF_INVISIBLE))
                    strcpy(buffer, "* ");
                else
                    *buffer = '\0';

                if (is_char_ethereal(i))
                    strcat(buffer, "<> ");
            }

            if (!IS_NPC(i)
                || (IS_NPC(i) && i->long_descr && strncmp(MSTR(i, long_descr), "A ", 2) /* NPC Creation */
                    &&strncmp(MSTR(i, long_descr), "An ", 3)
                    && strncmp(MSTR(i, long_descr), "The ", 4))) {
                strcat(buffer, PERS(ch, i));
                strcat(buffer, " ");
                strcat(buffer, MSTR(i, long_descr));
            } else {
                strcat(buffer, MSTR(i, long_descr));
            }


            /*      strcat(buffer, "\n\r"); */
            /* CAP(buffer); */
            *buffer = toupper(*buffer);
            send_to_char(buffer, ch);
        }
        /* End new_ldesc */

        if (IS_AFFECTED(i, CHAR_AFF_SANCTUARY))
            act("- $e glows with a bright light!", FALSE, i, 0, ch, TO_VICT);

        if (affected_by_spell(i, SPELL_LEVITATE))
            act("- $s feet hover above the ground.", FALSE, i, 0, ch, TO_VICT);

        if (affected_by_spell(i, SPELL_FLY))
            act("- $s feet hover above the ground.", FALSE, i, 0, ch, TO_VICT);

        if (affected_by_spell(i, SPELL_WIND_ARMOR))
            act("- $s body is surrounded by a dusty whirlwind.", FALSE, i, 0, ch, TO_VICT);

        if (affected_by_spell(i, SPELL_FIRE_ARMOR))
            act("- $s body is surrounded by flames.", FALSE, i, 0, ch, TO_VICT);

        if (affected_by_spell(i, SPELL_IMMOLATE))
            act("- $e is writhing in agony as orange flames immolate $s body!", FALSE, i, 0, ch,
                TO_VICT);

        if (affected_by_spell(i, SPELL_ENERGY_SHIELD))
            act("- $s form is surrounded by faint sparkles of light.", FALSE, i, 0, ch, TO_VICT);

        if (affected_by_spell(i, SPELL_SHADOW_ARMOR))
            act("- $s body is partly obscured by shadows.", FALSE, i, 0, ch, TO_VICT);

        if (!is_merchant(i) && !IS_IMMORTAL(i)) {
            cprintf(ch, "%s", list_bulky_obj_to_char(i->carrying, ch, 3, FALSE, get_char_distance(ch, i)));
        }

        if (i->equipment[WEAR_ABOUT_HEAD]) {
            if (char_can_see_obj(ch, i->equipment[WEAR_ABOUT_HEAD])) {
                sprintf(ball_buf, "- %s floats about $s head.",
                        OSTR(i->equipment[WEAR_ABOUT_HEAD], short_descr));
                act(ball_buf, FALSE, i, 0, ch, TO_VICT);
            }
        }

	// Humming gear check
	if (!IS_IMMORTAL(i)) {
            const char * hum_level[] = {
		    "", "faint", "moderate", "loud", "blaring"};
            // count and figure out how loud it is
	    int level = 0;
	    for (int j = 0; j < MAX_WEAR ; j++) {
		if (i->equipment[j] && IS_OBJ_STAT(i->equipment[j], OFL_HUM)) {
		    level += 2;
                    if (is_covered(ch, i, j))  // -1 for covered
			    level--;
		}
	    }
	    OBJ_DATA * tmp_obj = NULL;
            for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
		if (tmp_obj && IS_OBJ_STAT(tmp_obj, OFL_HUM))
		    level += 2;
	    }
	    int humlev = MAX(0, MIN(4, level / 5));
            if (humlev && ch->in_room && i->in_room && (ch->in_room == i->in_room)) {    // Only > 0 gets shown
                char magick_buf[MAX_STRING_LENGTH];
	    	sprintf(magick_buf, "- $e emits a %s humming sound.", hum_level[humlev]);
		act(magick_buf, FALSE, i, 0, ch, TO_VICT);
	    }
	}
    } else if (mode == 1) {
        tmp = i->equipment[WEAR_HEAD];

        det_mag_msg = find_ex_description("[DET_MAG_DESC]", i->ex_description, TRUE);

        det_eth_msg = find_ex_description("[DET_ETH_DESC]", i->ex_description, TRUE);

        det_invis_msg = find_ex_description("[DET_INVIS_DESC]", i->ex_description, TRUE);

        if (tmp && tmp->obj_flags.type == ITEM_MASK) {
            tmp_desc = find_ex_description("[mask_desc]", tmp->ex_description, TRUE);

            if (tmp_desc) {
                cprintf(ch, "%s", tmp_desc);
            } else if (MSTR(i, description))
                cprintf(ch, "%s", MSTR(i, description));
        }

        /* NESSALIN_DESC_CHANGE_8_24_2002 */
        // Check for edesc on character

        else if ((det_mag_msg)
                 && ((affected_by_spell(ch, SPELL_DETECT_MAGICK))
                     || IS_AFFECTED(ch, CHAR_AFF_DETECT_MAGICK)
                     || (affected_by_spell(ch, PSI_MAGICKSENSE) && can_use_psionics(i))))
            cprintf(ch, "%s", det_mag_msg);

        /* NESSALIN_DESC_CHANGE_11_15_2003 */
        // Check for ethereal edesc on character

        else if ((det_eth_msg)
                 && ((affected_by_spell(ch, SPELL_DETECT_ETHEREAL))
                     || IS_AFFECTED(ch, CHAR_AFF_DETECT_ETHEREAL)))
            cprintf(ch, "%s", det_eth_msg);
        else if ((det_invis_msg)
                 && ((affected_by_spell(ch, SPELL_DETECT_INVISIBLE))
                     || IS_AFFECTED(ch, CHAR_AFF_DETECT_INVISIBLE)))
            cprintf(ch, "%s", det_invis_msg);
        else if (MSTR(i, description))
            cprintf(ch, "%s", MSTR(i, description));
        else
            act("You see nothing special about $m.", FALSE, i, 0, ch, TO_VICT);

        if (i->temp_description)
            cprintf(ch, "%s", i->temp_description);

        /* display the eco if the looker is a galith */
        if (GET_RACE(ch) == RACE_GALITH)
            show_eco_to_galith(i, ch);

        /* Show a character to another */

        if (GET_MAX_HIT(i) > 0)
            percent = (100 * GET_HIT(i)) / GET_MAX_HIT(i);
        else
            percent = -1;       /* How could MAX_HIT be < 1?? */

        strcpy(buffer, capitalize(PERS(ch, i)));

        if (percent >= 100)
            strcat(buffer, " is in excellent condition.\n\r");
        else if (percent >= 90)
            strcat(buffer, " looks relatively fit.\n\r");
        else if (percent >= 70)
            strcat(buffer, " is in moderate condition.\n\r");
        else if (percent >= 50)
            strcat(buffer, " does not look well.\n\r");
        else if (percent >= 30)
            strcat(buffer, " is in poor condition.\n\r");
        else if (percent >= 15)
            strcat(buffer, " is in terrible condition.\n\r");
        else if (percent >= 0)
            strcat(buffer, " looks near death.\n\r");
        else
            strcat(buffer, " is in critical condition.\n\r");

        send_to_char(buffer, ch);

#if defined(AGING_SHOW)
        player_age = age(ch);
        g = age_class(get_virtual_age(player_age.year));
        g = MAX(0, g);
        g = MIN(5, g);
        sprintf(buf, " which by your race and appearance is %s.\n\r", age_names[g]);

        player_age = age(ch);
        percent = age_class(get_virtual_age(player_age.year));
        percent = MAX(0, percent);
        percent = MIN(5, percent);
        strcpy(buffer, PERS(ch, i));
        sprintf(buf, " appears %s for %s race.\n\r", age_names[percent], strcat(buffer, buf));

#endif
        if (affected_by_spell(i, SPELL_STONE_SKIN))
            act("$S skin has a stonelike quality.", FALSE, ch, 0, i, TO_CHAR);

        if (affected_by_spell(i, SPELL_HERO_SWORD))
            act("$E does not seem to breath or blink.", FALSE, ch, 0, i, TO_CHAR);

        if (affected_by_spell(i, SPELL_FLOURESCENT_FOOTSTEPS))
            act("$S feet glow with a yellowish light.", FALSE, ch, 0, i, TO_CHAR);

        if (affected_by_spell(i, SPELL_ENERGY_SHIELD))
            act("$S form is surrounded by faint sparkles of light.", FALSE, ch, 0, i, TO_CHAR);

        if (affected_by_spell(i, SPELL_WIND_ARMOR))
            act("A raging whirlwind of dust surrounds $S form, blowing strongly.", FALSE, ch, 0, i, TO_CHAR);

        if (affected_by_spell(i, SPELL_FIRE_ARMOR))
            act("Crackling flames surround $S form, dancing brightly.", FALSE, ch, 0, i, TO_CHAR);

        if (affected_by_spell(i, SPELL_IMMOLATE))
            act("$E writhes in agony as orange flames immolate $S body.", FALSE, ch, 0, i, TO_CHAR);

        if (affected_by_spell(i, SPELL_SHADOW_ARMOR))
            act("Writhing shadows surround $S body, darkening $S features.", FALSE, ch, 0, i,
                TO_CHAR);

        if (TRUE == get_char_extra_desc_value(i, "[SEND_SHADOW_SDESC]", bufy, MAX_STRING_LENGTH)) {
            if (IS_AFFECTED(ch, CHAR_AFF_DETECT_ETHEREAL)) {
                sprintf(buf, "%s is surrounded by the faint image of %s.", capitalize(HSSH(i)),
                        bufy);
                act(buf, FALSE, ch, 0, i, TO_CHAR);
            }
        }

        if (IS_AFFECTED(ch, CHAR_AFF_DETECT_MAGICK)
            || (affected_by_spell(ch, PSI_MAGICKSENSE) && can_use_psionics(i))) {

            /* EYES */
            int color = 0;
            struct affected_type *af;

            if (affected_by_spell(i, SPELL_INFRAVISION))
                color += 1;
            if (affected_by_spell(i, SPELL_DETECT_INVISIBLE))
                color += 2;
            if (affected_by_spell(i, SPELL_DETECT_MAGICK))
                color += 4;
            if (affected_by_spell(i, SPELL_IDENTIFY))
                color += 8;
            if (affected_by_spell(i, SPELL_DETECT_POISON))
                color += 16;
            switch (color) {
            case 1: // red
                act("$N's eyes glow dimly red.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 2: // blue
                act("$N's eyes glow dimly blue.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 3: // red & blue
                act("$N's eyes glow dimly purple.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 4: // yellow
                act("$N's eyes glow dimly yellow.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 5: // red and yellow
                act("$N's eyes glow dimly orange.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 6: // blue and yellow
                act("$N's eyes glow dimly green.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 7: // red, blue and yellow
                act("$N's eyes glow dimly light purple.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 8: // shimmer
                act("$N's pupils have a silver shimmer to them.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 9: // red & shimmer
                act("$N's shimmering eyes glow dimly red.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 10: // blue & shimmer
                act("$N's shimmering eyes glow dimly blue.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 11: // red, blue & shimmer
                act("$N's shimmering eyes glow dimly purple.", FALSE, ch, 0, i, TO_CHAR);
                break;
	    case 12: // yellow & shimmer
                act("$N's shimmering eyes glow dimly yellow.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 13: // red, yellow & shimmer
                act("$N's shimmering eyes glow dimly orange.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 14: // blue, yellow & shimmer
                act("$N's shimmering eyes glow dimly green.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 15: // red, blue, yellow & shimmer
                act("$N's shimmering eyes glow dimly pale purple.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 16: // brown
                act("$N's eyes glow dimly pale brown.", FALSE, ch, 0, i, TO_CHAR);
                break;
	    case 17: // red & brown
                act("$N's eyes glow dimly pale burgundy.", FALSE, ch, 0, i, TO_CHAR);
		break;
	    case 18: // blue & brown
                act("$N's eyes glow dimly pale taupe.", FALSE, ch, 0, i, TO_CHAR);
		break;
	    case 19: // red, blue & brown
                act("$N's eyes glow dimly pale sepia.", FALSE, ch, 0, i, TO_CHAR);
		break;
	    case 20: // yellow & brown
                act("$N's eyes glow dimly pale citrine.", FALSE, ch, 0, i, TO_CHAR);
		break;
	    case 21: // red, yellow & brown
                act("$N's eyes glow dimly pale tawny.", FALSE, ch, 0, i, TO_CHAR);
		break;
	    case 22: // blue, yellow & brown
                act("$N's eyes glow dimly pale sienna.", FALSE, ch, 0, i, TO_CHAR);
		break;
	    case 23: // red, yellow, blue & brown
                act("$N's eyes glow dimly pale ochre.", FALSE, ch, 0, i, TO_CHAR);
		break;
	    case 24: // brown & shimmer
                act("$N's shimmering eyes glow dimly pale brown.", FALSE, ch, 0, i, TO_CHAR);
		break;
	    case 25: // red, brown & shimmer
                act("$N's shimmering eyes glow dimly pale burgundy.", FALSE, ch, 0, i, TO_CHAR);
		break;
	    case 26: // blue, brown & shimmer
                act("$N's shimmering eyes glow dimly pale taupe.", FALSE, ch, 0, i, TO_CHAR);
		break;
	    case 27: // red, blue, brown & shimmer
                act("$N's shimmering eyes glow dimly pale sepia.", FALSE, ch, 0, i, TO_CHAR);
		break;
	    case 28: // yellow, brown & shimmer
                act("$N's shimmering eyes glow dimly pale citrine.", FALSE, ch, 0, i, TO_CHAR);
		break;
	    case 29: // red, yellow, brown & shimmer
                act("$N's shimmering eyes glow dimly pale tawny.", FALSE, ch, 0, i, TO_CHAR);
		break;
	    case 30: // blue, yellow, brown & shimmer
                act("$N's shimmering eyes glow dimly pale sienna.", FALSE, ch, 0, i, TO_CHAR);
		break;
	    case 31: // red, yellow, blue, brown & shimmer
                act("$N's shimmering eyes glow dimly pale ochre.", FALSE, ch, 0, i, TO_CHAR);
		break;
            default:
                break;
            }
            if (affected_by_spell(i, SPELL_BLIND))
                act("$N's eyes are covered with black spots.", FALSE, ch, 0, i, TO_CHAR);
            if (affected_by_spell(i, SPELL_DETECT_ETHEREAL))
                act("$N's eyes have black circles around them.", FALSE, ch, 0, i, TO_CHAR);

            /* HEAD */
            color = 0;
            if (affected_by_spell(i, SPELL_FURY))
                color += 1;     /* red */
            if (affected_by_spell(i, SPELL_CALM))
                color += 2;     /* blue */
            if (affected_by_spell(i, SPELL_PSI_SUPPRESSION))
                color += 4;     /* yellow */
            switch (color) {
            case 1:
                act("$N's head glows dimly red.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 2:
                act("$N's head glows dimly blue.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 3:
                act("$N's head glows dimly purple.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 4:
                act("$N's head glows dimly yellow.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 5:
                act("$N's head glows dimly orange.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 6:
                act("$N's head glows dimly green.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 7:
                act("$N's head glows dimly light purple.", FALSE, ch, 0, i, TO_CHAR);
                break;
            default:
                break;
            }

            if (affected_by_spell(i, SPELL_RECITE)) {
                cprintf(ch, "The ghostly image of %s clouds %s appearance.\n\r",
                        MSTR(i->specials.contact, short_descr), HSHR(i));
            }

            if (affected_by_spell(i, SPELL_DEAFNESS))
                act("A black haze hovers over $N's ears.", FALSE, ch, 0, i, TO_CHAR);
            if (affected_by_spell(i, SPELL_FEEBLEMIND))
                act("$N's head has a gray circle around it.", FALSE, ch, 0, i, TO_CHAR);
            if (affected_by_spell(i, SPELL_FEAR))
                act("$N's head has a yellow streak running down the back.", FALSE, ch, 0, i,
                    TO_CHAR);
            if (affected_by_spell(i, SPELL_CHARM_PERSON))
                act("$N's head has a pinkish hue near the top.", FALSE, ch, 0, i, TO_CHAR);

            if ((af = affected_by_spell(i, SPELL_SLEEP)) != NULL && af->magick_type != MAGICK_NONE)
                act("$N's head has a bright red glow near the base.", FALSE, ch, 0, i, TO_CHAR);
            if (affected_by_spell(i, SPELL_INSOMNIA))
                act("$N's head has violently active white swirls around the top.", FALSE, ch, 0, i,
                    TO_CHAR);

            /* ARMS */
            color = 0;
            if (affected_by_spell(i, SPELL_STRENGTH))
                color += 1;     /*green lines */
            if (affected_by_spell(i, SPELL_WEAKEN))
                color += 2;     /*pale green lines */
            switch (color) {
            case 1:
                act("$N's arms glow with green lines.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 2:
                act("$N's arms glow with pale green lines.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 3:
                act("$N's arms glow with interlacing green, and pale green lines.", FALSE, ch, 0, i,
                    TO_CHAR);
                break;
            default:
                break;
            }

            /* LEGS */
            color = 0;
            if (affected_by_spell(i, SPELL_GODSPEED))
                color += 1;     /*green lines */
            if (affected_by_spell(i, SPELL_SLOW))
                color += 2;     /*pale green lines */
            switch (color) {
            case 1:
                act("$N's legs glow with green lines.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 2:
                act("$N's legs glow with pale green lines.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 3:
                act("$N's legs glow with interlacing green, and pale green lines.", FALSE, ch, 0, i,
                    TO_CHAR);
                break;
            default:
                break;
            }

            /* MOUTH */
            color = 0;
            if (affected_by_spell(i, SPELL_TONGUES))
                color += 1;     /*green lines */
            if (affected_by_spell(i, SPELL_SILENCE))
                color += 2;     /*pale green lines */
            if (affected_by_spell(i, SPELL_FIREBREATHER))
                color += 4;     /*red */
            switch (color) {
            case 1:
                act("$N's mouth glows with a silvery aura.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 2:
                act("$N's mouth is hidden behind a black square.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 3:
                act("$N's mouth is hidden behind a black square, from behind which comes a silvery aura.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 4:
                act("$N's mouth is lined with tiny glowing red flames.", FALSE, ch, 0, i, TO_CHAR);
                break;
            case 5:
                act("$N's mouth is lined with tiny red flames, overlying a glaze of silver.", FALSE,
                    ch, 0, i, TO_CHAR);
                break;
            case 6:
                act("$N's mouth is lined with flames, surrounding a black square.", FALSE, ch, 0, i,
                    TO_CHAR);
                break;
            case 7:
                act("$N's mouth is lined with flames, surrounding a black and silver square.",
                    FALSE, ch, 0, i, TO_CHAR);
                break;
            default:
                break;
            }

            if (affected_by_spell(i, SPELL_BREATHE_WATER))
                act("$N's mouth has a faint, glowing green tint around the lips.", FALSE, ch, 0, i,
                    TO_CHAR);


            /* BODY */
            if (affected_by_spell(i, SPELL_FEATHER_FALL))
                act("$N's body is surrounded by a light blue aura.", FALSE, ch, 0, i, TO_CHAR);
            if (affected_by_spell(i, SPELL_LEVITATE))
                act("$N's body is surrounded by a blue aura.", FALSE, ch, 0, i, TO_CHAR);
            if (affected_by_spell(i, SPELL_FLY))
                act("$N's body is surrounded by a bright blue aura.", FALSE, ch, 0, i, TO_CHAR);
            if (affected_by_spell(i, SPELL_WIND_ARMOR))
                act("$N's body is surrounded by a violent, swirling blue aura.", FALSE, ch, 0, i,
                    TO_CHAR);
            if (affected_by_spell(i, SPELL_PARALYZE))
                act("$N's body has several grey bands around it.", FALSE, ch, 0, i, TO_CHAR);
            if (affected_by_spell(i, SPELL_CURSE))
                act("A reddish brown hue envelops $N's body.", FALSE, ch, 0, i, TO_CHAR);
            if (affected_by_spell(i, SPELL_SANCTUARY))
                act("$N is surrounded by a white light.", FALSE, ch, 0, i, TO_CHAR);
            if (affected_by_spell(i, SPELL_REGENERATE))
                act("Faint green lines overlie $N's form.", FALSE, ch, 0, i, TO_CHAR);
            if (affected_by_spell(i, SPELL_QUICKENING))
                act("Faint sparks play at intervals over $N's form.", FALSE, ch, 0, i, TO_CHAR);
            if (affected_by_spell(i, SPELL_ETHEREAL))
                act("$N is almost transparent!", FALSE, ch, 0, i, TO_CHAR);
            if (affected_by_spell(i, SPELL_POISON))
                act("$N's body is surrounded by a sickly green aura.", FALSE, ch, 0, i, TO_CHAR);
            if (affected_by_spell(i, SPELL_ARMOR))
                act("$N's body is covered with a pulsing yellow aura.", FALSE, ch, 0, i, TO_CHAR);
            if (affected_by_spell(i, SPELL_INVULNERABILITY))
                act("$N's body glows with a shiny, silver covering.", FALSE, ch, 0, i, TO_CHAR);
            if (isnamebracketsok("[sand_statue]", MSTR(i, name))) {
                tmp_string =
                    find_ex_description("[SAND_STATUE_POWERLEVEL]", i->ex_description, TRUE);
                if (tmp_string)
                    powerlevel = atoi(tmp_string);
                else
                    powerlevel = 1;
                chance = 75 - (powerlevel * 10);
                if (chance >= number(1, 100))
                    act("$N is made entirely of a life-like sand!", FALSE, ch, 0, i, TO_CHAR);
            }
        }

        if (IS_AFFECTED(ch, CHAR_AFF_DETECT_POISON)) {
	    int ptype;

	    ptype = is_poisoned_char(i);
	    if (ptype) {
		    sprintf(buffer, "$N's body is surrounded by a sickly %s aura.", poison_color[ptype]);
		    act(buffer, FALSE, ch, 0, i, TO_CHAR);
	    }
	}

        /* Check known member for small tribal groups - Turgon */
        for (counter = 0; show_tribe_affiliation[counter] != 0; counter++) {
            if ((IS_TRIBE(ch, show_tribe_affiliation[counter])
                 && IS_TRIBE(i, show_tribe_affiliation[counter]))) {
                char sztemp[256];
                char real_name_buf[MAX_STRING_LENGTH];
                int clan = show_tribe_affiliation[counter];

                int rank = get_clan_rank(i->clan, clan);
                char *rname = get_clan_rank_name(clan, rank);

                sprintf(sztemp, "$e is %s%sof the %s", rname ? rname : "", rname ? " " : "",
                        clan_table[clan].name);

                if (!IS_NPC(i)
                    || (IS_NPC(i)
                        && (IS_TRIBE_LEADER(i, clan)
                            || IS_SET(i->specials.act, CFL_UNIQUE)))) {
                    sprintf(buffer, ", known as %s.", REAL_NAME(i));
                    strcat(sztemp, buffer);
                } else
                    strcat(sztemp, ".");
                act(sztemp, FALSE, i, 0, ch, TO_VICT);
            }
        }

        /* A good place to show templars / militia whether the subject is a
         * known felon. */
#ifdef SHOW_CRIMINAL
        /* A templar in one of the city clans */
        if (GET_TRIBE(ch)
            && ((GET_GUILD(ch) == GUILD_TEMPLAR) || (GET_GUILD(ch) == GUILD_LIRATHU_TEMPLAR)
                || (GET_GUILD(ch) == GUILD_JIHAE_TEMPLAR))) {
            for (ct = 0; ct < MAX_CITIES; ct++)
                if (IS_TRIBE(ch, city[ct].tribe))
                    break;

            /* A valid city id to use */
            if (ct < MAX_CITIES) {
                /* Only works when we look at them within city limits
                 * - Knowledge stems from the power of the King */
                if (room_in_city(ch->in_room) == ct) {
                    /* Now, is the subject a criminal here? */
                    if (IS_SET(i->specials.act, city[ct].c_flag)) {
                        strcpy(buf, "");
                        for (af = i->affected; af; af = af->next) {
                            if (af->type == TYPE_CRIMINAL && af->modifier == city[ct].c_flag) {
                                if (af->power > CRIME_UNKNOWN && af->power <= MAX_CRIMES) {
                                    sprintf(buf, " for %s", Crime[af->power]);
                                    break;
                                }
                            }
                        }
                        sprintf(buffer, "$n is wanted in %s%s.", city[ct].name, buf);
                        act(buffer, FALSE, i, 0, ch, TO_VICT);
                    }
                }
            }
        }
#endif

        /* See who is leading a mount */
        if (IS_SET(i->specials.act, CFL_MOUNT) && i->master) {
            if (i->master == ch)
                act("$n is hitched to you.", FALSE, i, 0, ch, TO_VICT);
            else {
                if (char_can_see_char(ch, i->master)) {
                    sprintf(buffer, "$n is being led by %s.", PERS(ch, i->master));
                    act(buffer, FALSE, i, 0, ch, TO_VICT);
                } else
                    act("$n is being hitched to someone.", FALSE, i, 0, ch, TO_VICT);
            }
        }

        /* Put detect_magick shit here */

        if( IS_SET( ch->specials.brief, BRIEF_EQUIP ) ) {
            show_equipment(i, ch);
        } else {
            found = FALSE;
            for (j = 0; j < MAX_WEAR; j++) {
                if (i->equipment[j]) {
                    if (CAN_SEE_OBJ(ch, i->equipment[j]) && !is_covered(ch, i, j)) {
                        found = TRUE;
                    }
                }
                for (l = 0; l < MAX_WEAR_EDESC; l++) {
                    if (j == wear_edesc[l].location_that_covers) {
                        temp_desc =
                            find_ex_description(wear_edesc[l].edesc_name, i->ex_description, TRUE);
                        if ((temp_desc) && (!is_desc_covered(ch, i, j)))
                            found = TRUE;
                    }
                }
            }

            if (found) {
                send_to_char("\n\r", ch);
                act("$n is using:", FALSE, i, 0, ch, TO_VICT);
                for (j = 0; j < MAX_WEAR; j++) {
                    if (i->equipment[sorted_eq[j]]) {
                        if (CAN_SEE_OBJ(ch, i->equipment[sorted_eq[j]])
                            && !is_covered(ch, i, sorted_eq[j])) {
                            send_to_char(where[sorted_eq[j]], ch);
                            show_obj_to_char(i->equipment[sorted_eq[j]], ch, 1);
                        }
                    }
                    for (l = 0; l < MAX_WEAR_EDESC; l++) {
                        if (sorted_eq[j] == wear_edesc[l].location_that_covers) {
                            temp_desc =
                            find_ex_description(wear_edesc[l].edesc_name, i->ex_description, TRUE);
                                if ((temp_desc) && (!(i->equipment[sorted_eq[j]]))
                                && (!is_desc_covered(ch, i, sorted_eq[j]))) {
                                sprintf(show_mess, "<%s>", wear_edesc[l].name);
                                cprintf(ch, "%-25s%s\n\r", show_mess, temp_desc);
                            }
                        }
                    }
                }
            }
        }

        if (!is_merchant(i) && i != ch && !IS_IMMORTAL(i)) {
            if( IS_SET( ch->specials.brief, BRIEF_EQUIP ) ) {
                char *tmpstr;

                sprintf(buf, "\n\r%s is carrying %s", capitalize(HSSH(i)),
                        list_bulky_obj_to_char(i->carrying, ch, 4, TRUE,
                        get_char_distance(ch, i) ));
                tmpstr = format_string(strdup(buf));
                cprintf(ch, "%s", tmpstr);
                free(tmpstr);
            } else {
                cprintf(ch, "\n\r%s is carrying:\n\r%s\n\r", capitalize(HSSH(i)),
                        list_obj_to_char(i->carrying, ch, 4, TRUE));
            }
        }

        /* if they're actively watching, give watch chance to avoid */
        if (is_char_actively_watching_char(i, ch)) {
            if (!watch_success(i, ch, NULL, 0, SKILL_NONE)) {
                cprintf(ch, "You notice %s is watching you.\n\r", PERS(ch, i));
            }
        }
    } else if (mode == 2) {     /* Lists inventory */
        sprintf(buf, "%s is carrying:\n\r%s\n\r", PERS(ch, i),
                list_bulky_obj_to_char(i->carrying, ch, 1, TRUE, 
                 get_char_distance(ch, i)));
        send_to_char(buf, ch);
    }
}

void
show_magick_level_to_char(CHAR_DATA * ch)
{
    CHAR_DATA *i;
    OBJ_DATA *j;
    const char * magick_level[] = {
	    "trace", "weak", "moderate", "strong",
	    "exceptional", "overwhelming" };
    int level = 0;

    if(!ch || !ch->in_room)
	return;

    for (i = ch->in_room->people; i; i = i->next_in_room)
        if (ch != i) {
	}

    for (j = ch->in_room->contents; j; j = j->next_content)
    {
    }

    if (level) {
	int lev = level % (sizeof(magick_level) / sizeof(magick_level[0]));
        cprintf(ch, "You detect %s %s amount of magickal energy in the area.",
	    index("aeiouyAEIOUY", *magick_level[lev]) ? "an" : "a", magick_level[lev]);
    }
}


void
list_char_to_char(CHAR_DATA * list, CHAR_DATA * ch, int mode)
{
    CHAR_DATA *i;

    for (i = list; i; i = i->next_in_room)
        if (ch != i) {
            switch (CAN_SEE(ch, i)) {
            case 1:            /* sees i clearly */
            case 8:
                show_char_to_char(i, ch, 0);
                break;
            case 2:            /* sees i as hidden */
                cprintf(ch, "*A %sstrange shadow is here*\n\r", height_weight_descr(ch, i));
                break;
            case 3:            /* sees i as invis */
                cprintf(ch, "*There is a %sstrange blur in the air*\n\r", height_weight_descr(ch, i));
                break;
            case 4:            /* sees i as ethereal */
                cprintf(ch, "*A %sghostly outline is here*\n\r", height_weight_descr(ch, i));
                break;
            case 5:            /* sees i as heat (infra) */
                cprintf(ch, "*A %sred shape is here*\n\r", height_weight_descr(ch, i));
                break;
            case 6:            /* weather is too bad to see */
                cprintf(ch, "*A %sfaint shape is here*\n\r", height_weight_descr(ch, i));
                break;
            case 7:            /* sees i as shimmering (magick) */
                cprintf(ch, "*A %sshimmering shape is here*\n\r", height_weight_descr(ch, i));
                break;
            default:
                break;
            }
        }
}

int
char_can_peek_at_chars_object(CHAR_DATA * ch, CHAR_DATA * vict, OBJ_DATA * obj)
{
    int skill;
    int roll;

    if (IS_IMMORTAL(ch))
        return TRUE;

    /* can always peek when subdued or unconscious */
    if (!AWAKE(vict) || IS_AFFECTED(vict, CHAR_AFF_SUBDUED)
        || is_paralyzed(vict))
        return TRUE;

    if (!has_skill(ch, SKILL_PEEK))
        skill = 5;
    else
        skill = ch->skills[SKILL_PEEK]->learned;

    /* if it's a worn_by_item, small chance  */
    if (obj->equipped_by)
        skill /= 2;

    roll = number(1, 100);

    if (skill >= roll)
        return TRUE;
    else
        return FALSE;
}

int
check_peek_biff(CHAR_DATA * ch, CHAR_DATA * i) {
    int biffed;
    int roll;
    int skill = has_skill(ch, SKILL_PEEK) ? ch->skills[SKILL_PEEK]->learned : 5;
    char buf[MAX_STRING_LENGTH];
    CHAR_DATA *rch;

    /* if the victim is watching the character, hefty penalty */
    if (is_char_actively_watching_char(i, ch)) {
        skill -= get_watched_penalty(i);
    }

    /* added a populated bonus */
    if (IS_SET(ch->in_room->room_flags, RFL_POPULATED)) {
        skill += 30;
    }

    /* was a 10% chance of failing -Morg */
    /* biffed = number( 1, 100 ) < 10; */

    /* now it's skill based, like every other skill in the game -Morg */
    roll = number(1, 101);
    biffed = roll > skill;

    if (IS_IMMORTAL(ch))
        biffed = FALSE;

    if (ch == i)
        biffed = FALSE;

    if (i != NULL && !AWAKE(i))
        biffed = FALSE;

    if (!CAN_SEE(i, ch))
        biffed = FALSE;

    if (biffed) {
        /* second skill check to see if they were caught */
        roll = number(1, 101);

        /* They fail, but can the recover? */
        if (roll > skill) {
            /* no, complete failure */
            act("$n looks you over carefully.", FALSE, ch, 0, i, TO_VICT);
            act("As you study $N's possessions, $E notices you looking.", FALSE, ch, 0, i, TO_CHAR);

            /* let others in the room have a chance to have seen it */
            for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
                /* use roll - skill as a gauge of how bad they messed up */
                if (rch != ch && rch != i && watch_success(rch, ch, i, roll - skill, SKILL_PEEK)) {
                    sprintf(buf, "%s", PERS(rch, ch));
                    cprintf(rch, "You notice %s look %s over carefully.\n\r", buf, PERS(rch, i));
                    if (IS_AFFECTED(ch, CHAR_AFF_HIDE)) {
                        REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_HIDE);
                    }
                }
            }
            sprintf(buf, "%s biffed a peek at %s.\n\r", GET_NAME(ch), GET_NAME(i));
            send_to_monitors(ch, i, buf, MONITOR_ROGUE);
        } else {
            /* managed to recover */
            act("You can't manage to get a good peek inside $S pack.", FALSE, ch, 0, i, TO_CHAR);

            /* let others in the room have a chance to have seen it */
            for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
                /* use roll - skill as a gauge of how bad they messed up */
                if (rch != ch && rch != i && watch_success(rch, ch, i, roll - skill, SKILL_PEEK)) {
                    sprintf(buf, "%s", PERS(rch, ch));
                    cprintf(rch, "You notice %s looking %s over carefully.\n\r", buf, PERS(rch, i));
                    if (IS_AFFECTED(ch, CHAR_AFF_HIDE)) {
                        REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_HIDE);
                    }
                }
            }
            sprintf(buf, "%s recovered a failed peek at %s.\n\r", GET_NAME(ch), GET_NAME(i));
            send_to_monitors(ch, i, buf, MONITOR_ROGUE);
        }
        gain_skill(ch, SKILL_PEEK, 1);
    }
    // didn't biff, still give a chance for watchers
    else {
        for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
            /* use roll - skill as a gauge of how big of a success */
            if (rch != ch && rch != i && watch_success(rch, ch, i, roll - skill, SKILL_PEEK)) {
                sprintf(buf, "%s", PERS(rch, ch));
                cprintf(rch, "You notice %s looking %s over carefully.\n\r", buf, PERS(rch, i));
                if (IS_AFFECTED(ch, CHAR_AFF_HIDE)) {
                    REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_HIDE);
                }
            }
        }

        if (!IS_IMMORTAL(ch)) {
            sprintf(buf, "%s peeks at %s.\n\r", GET_NAME(ch), GET_NAME(i));
            send_to_monitors(ch, i, buf, MONITOR_ROGUE);
        }
    }
    return biffed;
}

void
cmd_peek(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int found = FALSE, j;
    char buffer[2000];
    OBJ_DATA *tmp_obj = NULL;
    CHAR_DATA *i = NULL;

    char pos_item[MAX_STRING_LENGTH];
    int eq_pos, temp, tempcolor, vis;
    bool peek_into = FALSE;

    argument = one_argument(argument, buffer, sizeof(buffer));

    temp = strlen(buffer);
    if (temp > 2) {
        if (buffer[temp - 2] == '\'' && buffer[temp - 1] == 's') {
            peek_into = TRUE;
            buffer[temp - 2] = '\0';
        } else if (buffer[temp - 1] == '\'' && buffer[temp - 2] == 's') {
            peek_into = TRUE;
            buffer[temp - 1] = '\0';
        }
    }

    if( !IS_IMMORTAL(ch) && IS_DARK(ch->in_room)) {
        cprintf(ch, "You can't see a thing; its dark out!\n\r");
        return;
    }

    // take into account room visibility
    if (!IS_IMMORTAL(ch) && ((vis = visibility(ch)) < 0)) {
        cprintf(ch, "You can't see a thing; sand swirls about you!\n\r");
        return;
    }

    i = get_char_room_vis(ch, buffer);

    if (!i) {
        send_to_char("No one here by that name.\n\r", ch);
        return;
    }

    if (i == ch) {
        send_to_char("Why are you peeking at yourself?\n\r", ch);
        return;
    }

    if (AWAKE(i) && !is_paralyzed(i) && !IS_AFFECTED(i, CHAR_AFF_SUBDUED)
        && !has_skill(ch, SKILL_PEEK)) {
        send_to_char("You do not see anything abnormal.\n\r", ch);
        return;
    }

    /* possesions of another character */
    if (peek_into) {
        argument = one_argument(argument, pos_item, sizeof(pos_item));

        tmp_obj = get_object_in_equip_vis(ch, pos_item, i->equipment, &eq_pos);

        if (!tmp_obj) {
            if ((tmp_obj = get_obj_in_list_vis(ch, pos_item, i->carrying)) == NULL)  {
                act("$E isn't wearing or carrying anything like that.",
                 FALSE, ch, 0, i, TO_CHAR);
                return;
            } else {
               eq_pos = -1;
            }
        }

        if (check_peek_biff(ch, i))
            return;

        if (IS_IMMORTAL(ch) || eq_pos == -1 || !is_covered(ch, i, eq_pos)) {
            if (char_can_peek_at_chars_object(ch, i, tmp_obj)) {
                if (GET_ITEM_TYPE(tmp_obj) == ITEM_DRINKCON) {
                    if (tmp_obj->obj_flags.value[1] <= 0)
                        act("It is empty.", FALSE, ch, 0, 0, TO_CHAR);
                    else {
                        temp =
                            (((tmp_obj->obj_flags.value[1]) * 3) / (tmp_obj->obj_flags.value[0]));
                        temp = MAX(0, MIN(3, temp));
                        tempcolor = tmp_obj->obj_flags.value[2];
                        tempcolor = MAX(0, MIN((MAX_LIQ - 1), tempcolor));

                        sprintf(buffer, "It's %sfull of %s%s liquid.\n\r", fullness[temp],
                                indefinite_article(color_liquid[tempcolor]), color_liquid[tempcolor]);

                        send_to_char(buffer, ch);
                    }
                } else if (GET_ITEM_TYPE(tmp_obj) == ITEM_CONTAINER) {
                    if (is_closed_container(ch, tmp_obj) && !IS_IMMORTAL(ch)) {
                        send_to_char("It is closed.\n\r", ch);
                        return;
                    }

                    if (IS_IMMORTAL(ch)) {
                        act("You use your x-ray vision and peek into $p.", FALSE, ch, tmp_obj, NULL,
                            TO_CHAR);
                    }

                    /* Show it to them */
                    show_list_of_obj_to_char(tmp_obj, tmp_obj->contains, ch, "In", FIND_OBJ_EQUIP );
                } else
                    send_to_char("That is not a container.\n\r", ch);
            } /* Couldn't get a good peek */
            else {
                act("You find it difficult to peek inside $p.", FALSE, ch, tmp_obj, NULL, TO_CHAR);
            }
        } else {                /* It was covered up */

            act("It would be too difficult to peek inside $p.", FALSE, ch, tmp_obj, NULL, TO_CHAR);
            return;
        }
    } else {                    /* Not the possession, just the person */
        if (check_peek_biff(ch, i))
            return;

        act("$n is using:", FALSE, i, 0, ch, TO_VICT);
        found = FALSE;
        for (j = 0; j < MAX_WEAR; j++) {
            if (i->equipment[sorted_eq[j]]) {
                if (CAN_SEE_OBJ(ch, i->equipment[sorted_eq[j]])
                    && (!is_covered(ch, i, sorted_eq[j])
                        || (is_covered(ch, i, sorted_eq[j])
                            && sorted_eq[j] != WEAR_HANDS       // no glove peaking
                            && char_can_peek_at_chars_object(ch, i, i->equipment[sorted_eq[j]])))) {
                    found = TRUE;
                    send_to_char(where[sorted_eq[j]], ch);
                    show_obj_to_char(i->equipment[sorted_eq[j]], ch, 1);
                }
            }
        }

        if (!found)
            send_to_char("Nothing.\n\r", ch);

        found = FALSE;
        act("Peering into $S pack, you see the following items:", FALSE, ch, 0, i, TO_CHAR);

        // if they have coins
        if( GET_OBSIDIAN(i) > 0 ) {
            // create a temporary money object to show
            if (!(tmp_obj = create_money(GET_OBSIDIAN(i), COIN_ALLANAK))) {
                gamelog("cmd_peek: Unable to create money object.");
            } else if (char_can_peek_at_chars_object(ch, i, tmp_obj)) {
                show_obj_to_char(tmp_obj, ch, 1);
                extract_obj(tmp_obj);
                found = TRUE;
            }
        }

        for (tmp_obj = i->carrying; tmp_obj; tmp_obj = tmp_obj->next_content) {
            if (CAN_SEE_OBJ(ch, tmp_obj) 
             && char_can_peek_at_chars_object(ch, i, tmp_obj)) {
                show_obj_to_char(tmp_obj, ch, 1);
                found = TRUE;
            }
        }
        if (!found)
            send_to_char("Nothing.\n\r", ch);
    }
}


void
show_light_properties(OBJ_DATA * obj, CHAR_DATA * ch)
{
    char color[MAX_STRING_LENGTH];

    if (!IS_LIT(obj)) {
        switch (obj->obj_flags.value[2]) {
        case LIGHT_TYPE_FLAME: /* Flame */
            cprintf(ch, "It is not currently lit.\n\r");
            break;
        default:
            break;
        }

        if (IS_REFILLABLE(obj) && obj->obj_flags.value[1]) {
            if (!obj->obj_flags.value[0]) {
                cprintf(ch, "It is empty.\n\r");
            } else {
                int temp = obj->obj_flags.value[0] * 3 / obj->obj_flags.value[1];
                temp = MAX(0, MIN(3, temp));
                cprintf(ch, "It's %sfull.\n\r", fullness[temp]);
            }
        }
        return;
    }
    if (obj->obj_flags.value[4] > 0)
        sprintf(color, "%s", light_color[obj->obj_flags.value[4]].name);

    /* Wheeeeeeee, here's the light code that WORKS.  -Sanvean */
    switch (obj->obj_flags.value[2]) {
    case LIGHT_TYPE_FLAME:     /* Flame */
        if (obj->obj_flags.value[4] > 0)
            strcat(color, " ");
        if (obj->obj_flags.value[0] > 40) {
            cprintf(ch, "%s's %sflame burns strong and bright.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), color);
        } else if ((obj->obj_flags.value[0] < 40)
                   && (obj->obj_flags.value[0] > 5)) {
            cprintf(ch, "%s's %sflames burn strong and bright.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), color);
        } else if ((obj->obj_flags.value[0] < 6)
                   && (obj->obj_flags.value[0] > 1)) {
            cprintf(ch, "%s's %sflame flickers feebly. \n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), color);
        } else {
            cprintf(ch, "%s's %sflame flickers weakly, about to go out.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), color);
        }
        break;
    case LIGHT_TYPE_MAGICK:    /* magick */
        if (obj->obj_flags.value[0] <= 1)       /* 0 duration */
            cprintf(ch, "%s glows dimly%s%s, its energies ebbing with each moment.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), (*color != '\0' ? " " : ""), color);
        else if (obj->obj_flags.value[0] < 6)   /* 2-5 duration */
            cprintf(ch, "%s glows dimly%s%s.\n\r", capitalize(format_obj_to_char(obj, ch, 1)),
                    (*color != '\0' ? " " : ""), color);
        else if (obj->obj_flags.value[0] < 40)  /* 6-39 duration */
            cprintf(ch, "%s provides a steady%s%s light.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), (*color != '\0' ? " " : ""), color);
        else                    /* 40+ duration */
            cprintf(ch, "%s provides a steady, strong%s%s light.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), (*color != '\0' ? " " : ""), color);
        break;
    case LIGHT_TYPE_PLANT:     /* plant */
        if (obj->obj_flags.value[0] > 25) {
            cprintf(ch, "%s glows with a strong, steady %s light.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), color);
        } else if ((obj->obj_flags.value[0] < 25)
                   && (obj->obj_flags.value[0] > 3)) {
            cprintf(ch, "%s provides a steady %s light.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), color);
        } else if ((obj->obj_flags.value[0] < 4)
                   && (obj->obj_flags.value[0] > 1)) {
            cprintf(ch, "%s glows dimly%s%s.\n\r", capitalize(format_obj_to_char(obj, ch, 1)),
                    (*color == '\0') ? "" : " ", color);
        } else {
            cprintf(ch, "%s glows dimly, its %s light fading with each moment.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), color);
        }
        break;
    case LIGHT_TYPE_ANIMAL:    /* animal */
        if (obj->obj_flags.value[0] > 20) {
            cprintf(ch, "%s shines with a strong, steady %s light.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), color);
        } else if ((obj->obj_flags.value[0] < 20)
                   && (obj->obj_flags.value[0] > 5)) {
            cprintf(ch, "%s shines steadily, producing %s light.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), color);
        } else if ((obj->obj_flags.value[0] < 6)
                   && (obj->obj_flags.value[0] > 1)) {
            cprintf(ch, "%s wavers dimly%s%s.\n\r", capitalize(format_obj_to_char(obj, ch, 1)),
                    (*color == '\0') ? "" : " ", color);
        } else {
            cprintf(ch, "%s's %s light wavers dimly, fading with each moment.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), color);
        }
        break;
    case LIGHT_TYPE_MINERAL:   /* mineral */
        if (obj->obj_flags.value[0] > 30) {
            cprintf(ch, "%s glows with a steady, bright %s luminescence.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), color);
        } else if ((obj->obj_flags.value[0] < 31)
                   && (obj->obj_flags.value[0] > 5)) {
            cprintf(ch, "%s glows with a steady %s luminescence.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), color);
        } else if ((obj->obj_flags.value[0] < 6)
                   && (obj->obj_flags.value[0] > 1)) {
            cprintf(ch, "%s's %s light glows faint but steady.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), color);
        } else {
            cprintf(ch, "%s's %s light is faint, ebbing slowly.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), color);
        }
        break;
    default:
        if (obj->obj_flags.value[0] > 30) {
            cprintf(ch, "%s shines with a steady, bright %s light.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), color);
        } else if ((obj->obj_flags.value[0] < 40)
                   && (obj->obj_flags.value[0] > 5)) {
            cprintf(ch, "%s shines with a steady %s light.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), color);
        } else if ((obj->obj_flags.value[0] < 6)
                   && (obj->obj_flags.value[0] > 1)) {
            cprintf(ch, "%s's %s light shines faint but steady.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), color);
        } else {
            cprintf(ch, "%s's %s light is faint, ebbing slowly.\n\r",
                    capitalize(format_obj_to_char(obj, ch, 1)), color);
        }
        break;
    }                           /* End of the switch */

/* commenting out till i can mess with format -sanvean
  sprintf(buf, "(%s) \n\r",
   format_obj_to_char( obj, ch, 1 ),
   bits == FIND_OBJ_INV ? "being carried" :
   bits == FIND_OBJ_ROOM ? "lying here" :
   bits == FIND_OBJ_EQUIP ? "being used" : "unknown");
  send_to_char(buf, ch);*/
}


/* Added 8/18/00 in order to be able to assess the condition of objects */
/* Currently it is only for assessing wagons, to determine damage -- Az */
/* Tinkered with it to expand it somewhat.  Added a case statement for  */
/* tool items so people can better tell what they might be used for, and*/
/* also made it so when you assess any item, it tells you where it might*/
/* be worn (handy for WORN, ARMOR, TREASURE, SKIN, HERB, etc.  -Sanvean */
/* Also added cases for components, herbs and weapons.  Note: I've put  */
/* put the case statements in alphabetical order, to make them easier to*/
/* find/manipulate, if we try to keep that going, it may make life      */
/* easier.  -San, 8/26/00                                               */
/* Adding TREASURE case.  -San 4/21/01                                  */
/* Added LIGHT case. -Morg 4/27/02                                      */
void
assess_object(CHAR_DATA * ch, OBJ_DATA * obj, int verbosity)
{
    char buf[MAX_STRING_LENGTH];
    int dam, maxdam, temp;
    int perc, obj_cap;
    struct harness_type *temp_beast;
    char *poison_msg = NULL;

    if (CAN_WEAR(obj, ITEM_WEAR_BELT)) {
        act("$p can be worn as a belt.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_FINGER)) {
        act("$p can be worn on your finger.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_NECK)) {
        act("$p can be worn around the neck.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_BODY)) {
        act("$p can be worn on the torso.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_HEAD)) {
        act("$p can be worn on the head.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_LEGS)) {
        act("$p can be worn on one's legs.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_FEET)) {
        act("$p can be worn on one's feet.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_HANDS)) {
        act("$p can be worn on one's hands.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_ARMS)) {
        act("$p can be worn on one's arms.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_ABOUT)) {
        act("$p can be worn about one's body.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_WAIST)) {
        act("$p can be worn about one's waist.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_WRIST)) {
        act("$p can be worn on the wrist.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_ON_BELT)) {
        act("$p can be hooked onto a belt.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_BACK)) {
        act("$p can be worn across one's back.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_IN_HAIR)) {
        act("$p can be worn in one's hair.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_FACE)) {
        act("$p can be worn on the face.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_ANKLE)) {
        act("$p can be worn around the ankle.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if ((CAN_WEAR(obj, ITEM_WEAR_RIGHT_EAR)) || (CAN_WEAR(obj, ITEM_WEAR_LEFT_EAR))) {
        act("$p can be worn in one's ear.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_FOREARMS)) {
        act("$p can be worn on one's forearms.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_ABOUT_THROAT)) {
        act("$p can be worn about one's throat.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_SHOULDER)) {
        act("$p can be worn on the shoulder.", FALSE, ch, obj, 0, TO_CHAR);
    }
    if (CAN_WEAR(obj, ITEM_WEAR_OVER_SHOULDER)) {
        act("$p can be slung over the shoulder.", FALSE, ch, obj, 0, TO_CHAR);
    }

    switch (obj->obj_flags.type) {
    case ITEM_BOW:
        /* can you pull it back? */
        if ((!obj->obj_flags.value[2]) && (obj->obj_flags.value[1] > GET_STR(ch))) {
            sprintf(buf, "You test %s's pull and decide it is too strong " "for you.\n\r",
                    format_obj_to_char(obj, ch, 1));
            send_to_char(buf, ch);
        } else if ((!obj->obj_flags.value[2])
                   && (obj->obj_flags.value[1] + 1 < GET_STR(ch))) {
            sprintf(buf, "You test %s's pull and decide it is a little " "weak for you.\n\r",
                    format_obj_to_char(obj, ch, 1));
            send_to_char(buf, ch);
        } else if ((!obj->obj_flags.value[2])) {
            sprintf(buf, "You test %s's pull and decide it is just about " "right for you.\n\r",
                    format_obj_to_char(obj, ch, 1));
            send_to_char(buf, ch);
        }
        break;
    case ITEM_ARMOR:
        if (size_match(ch, obj)) {
            act("$p looks like it will fit you.", FALSE, ch, obj, 0, TO_CHAR);
        } else
            act("$p looks like it will require tailoring to fit you.", FALSE, ch, obj, 0, TO_CHAR);
        break;
    case ITEM_COMPONENT:
        if ((affected_by_spell(ch, SPELL_IDENTIFY))
            && ch->skills[SKILL_COMPONENT_CRAFTING]) {
            if (obj->obj_flags.value[0] == 0) {
                act("$p appears allied to the sphere of pret.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 1) {
                act("$p appears allied to the sphere of mutur.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 2) {
                act("$p appears allied to the sphere of dreth.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 3) {
                act("$p appears allied to the sphere of psiak.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 4) {
                act("$p appears allied to the sphere of iluth.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 5) {
                act("$p appears allied to the sphere of divan.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 6) {
                act("$p appears allied to the sphere of threl.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 7) {
                act("$p appears allied to the sphere of morz.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 8) {
                act("$p appears allied to the sphere of viqrol.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 9) {
                act("$p appears allied to the sphere of nathro.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 10) {
                act("$p appears allied to the sphere of locror.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 11) {
                act("$p appears allied to the sphere of wilith.", FALSE, ch, obj, 0, TO_CHAR);
            }
        } else {
            act("$p appears unremarkable to your eyes.", FALSE, ch, obj, 0, TO_CHAR);
        }
        break;
    case ITEM_HERB:
        if (ch->skills[SKILL_BREW]) {
            if (obj->obj_flags.value[2] == 0) {
                act("$p has a faint reddish tint.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[2] == 1) {
                act("$p has a faint bluish tint.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[2] == 2) {
                act("$p has a faint green tint.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[2] == 3) {
                act("$p has a faint dark tint.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[2] == 4) {
                act("$p has a faint purplish tint.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[2] == 5) {
                act("$p has a faint yellow tint.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[2] == 6) {
                act("$p has a pale tint.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[2] == 7) {
                act("$p has a faint greyish tint.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[2] == 8) {
                act("$p has a faint rosy tint.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[2] == 9) {
                act("$p has a faint orange tint.", FALSE, ch, obj, 0, TO_CHAR);
            } else {
                act("$p appears unremarkable to your eyes.", FALSE, ch, obj, 0, TO_CHAR);
            }
        }
        break;
    case ITEM_LIGHT:
        show_light_properties(obj, ch);
        break;
    case ITEM_NOTE:
/* Note: this is mainly for the spice smokers, so they can see how  */
/* many more smokes there is in something.  Nonetheless, I've tried */
/* to word it so even assessing scrolls won't be too funky.  -San   */
        if (obj->obj_flags.value[0] >= 10) {
            act("$p appears to have a great many pages in it.", FALSE, ch, obj, 0, TO_CHAR);
            return;
        } else if (obj->obj_flags.value[0] >= 5) {
            act("$p appears to have a lot of pages in it.", FALSE, ch, obj, 0, TO_CHAR);
            return;
        } else if (obj->obj_flags.value[0] >= 2) {
            act("$p appears to have a few pages in it.", FALSE, ch, obj, 0, TO_CHAR);
            return;
        } else if (obj->obj_flags.value[0] == 1) {
            act("$p appears to have a single page.", FALSE, ch, obj, 0, TO_CHAR);
            return;
        }
    case ITEM_STAFF:
    case ITEM_WAND:
        if ((affected_by_spell(ch, SPELL_DETECT_MAGICK)) && ch->skills[SKILL_COMPONENT_CRAFTING]) {
            act("You sense a magickal presence in $p.", FALSE, ch, obj, 0, TO_CHAR);
        } else {
            act("$p appears unremarkable to your eyes.", FALSE, ch, obj, 0, TO_CHAR);
        }
        break;
    case ITEM_TOOL:
        {
            if (obj->obj_flags.value[0] == 1) {
                act("$p might be used in climbing.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 2) {
                act("$p might be used to trap another object.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 3) {
                act("$p might be used to remove a trap.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 4) {
                act("$p might be used to create a snare.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 5) {
                act("$p might be used as a bandage.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 6) {
                act("$p might be useful in picking locks.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 7) {
                act("$p might be useful in sharpening a weapon.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 8) {
                act("$p might be useful in mending soft armors.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 9) {
                act("$p might be useful in mending hard armors.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 10) {
                act("$p is clearly a tent.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 11) {
                act("$p seems to be a fuse.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 12) {
                act("$p looks useful for chiseling stone.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 13) {
                act("$p looks useful for hammering things.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 14) {
                act("$p looks useful for sawing wood or bone.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 15) {
                act("$p looks useful for sewing cloth.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 16) {
                act("$p looks useful for piercing holes in leather.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 17) {
                act("$p looks useful for smoothing or polishing.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 18) {
                act("$p might be useful in sharpening a weapon.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 19) {
                act("$p might be used for very detailed work.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 20) {
                act("$p might be used for painting things.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 21) {
                act("$p might be used in leatherworking or tanning.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 22) {
                act("$p might be used in fletchery.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 23) {
                act("$p might be used for drilling holes.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 24) {
                act("$p might be used for fabricworking.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 25) {
                act("$p appears to be an ordinary household implement.", FALSE, ch, obj, 0,
                    TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 26) {
                act("$p might be used for scenting a room or person.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 27) {
                act("$p appears to be used for woodcarving.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 28) {
                act("$p appears to be used in building.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 29) {
                act("$p appears to be used in woodworking or similar work.", FALSE, ch, obj, 0,
                    TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 30) {
                act("$p appears to be some form of incense.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 31) {
                act("$p appears to be used in cleaning.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 32) {
                act("$p appears to be some sort of spice paraphernalia.", FALSE, ch, obj, 0,
                    TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 33) {
                act("$p looks as though it would be useful for cooking.", FALSE, ch, obj, 0,
                    TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 34) {
                act("$p looks as though it is used in brewing.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == TOOL_KILN) {
              act ("$p is a kiln, used for ceramics.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == TOOL_LOOM_SMALL) {
              act ("$p is a small loom, used for weaving small cloth items.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == TOOL_LOOM_LARGE) {
              act ("$p is a large loom, used for weaving large cloth items.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[0] == 0) {
                act("$p appears unremarkable to your eyes.", FALSE, ch, obj, 0, TO_CHAR);
            }

            switch (obj->obj_flags.value[1]) { // Quality
            case 0:
              send_to_char("The tool appears completely non-functional.\n\r", ch);
              break;
            case 1:
              send_to_char("The tool appears to be of barely functional quality.\n\r", ch);
              break;
            case 2:
              send_to_char("The tool appears to be of terrible quality.\n\r", ch);
              break;
            case 3:
              send_to_char("The tool appears to be of poor quality.\n\r", ch);
              break;            
            case 4:
              send_to_char("The tool appears to be of below average quality.\n\r", ch);
              break;
            case 5:
              send_to_char("The tool appears to be of average quality.\n\r", ch);
              break;
            case 6:
              send_to_char("The tool appears to be of above average quality.\n\r", ch);
              break;
            case 7:
              send_to_char("The tool appears to be of decent quality.\n\r", ch);
              break;
            case 8:
              send_to_char("The tool appears to be of good quality.\n\r", ch);
              break;
            case 9:
              send_to_char("The tool appears to be of an excellent quality.\n\r", ch);
              break;
            case 10:
              send_to_char("The tool appears to be of a pristine quality.\n\r", ch);
              break;
            default:
              send_to_char("This tool looks to be in servicable condition.\n\r", ch);
              break;
            }
        }
        break;
    case ITEM_TREASURE:
        if (obj->obj_flags.value[0] == 2) {
            act("$p appears remarkable for its artistic qualities.", FALSE, ch, obj, 0, TO_CHAR);
        } else if (obj->obj_flags.value[0] == 3) {
            act("$p appears to be a wind instrument.", FALSE, ch, obj, 0, TO_CHAR);
        } else if (obj->obj_flags.value[0] == 4) {
            act("$p appears to be a stringed instrument.", FALSE, ch, obj, 0, TO_CHAR);
        } else if (obj->obj_flags.value[0] == 5) {
            act("$p appears to be a percussion instrument.", FALSE, ch, obj, 0, TO_CHAR);
        } else {
            act("$p appears unremarkable to your eyes.", FALSE, ch, obj, 0, TO_CHAR);
        }
        break;
    case ITEM_WAGON:
        if (obj->obj_flags.value[1] == 0) {
            act("$p appears ordinary to your eyes.", FALSE, ch, obj, 0, TO_CHAR);
        } else {
            dam = obj->obj_flags.value[4];
            maxdam = MAX(1, calc_wagon_max_damage(obj));
            perc = ((dam * 10) / maxdam);
            if (!perc) {
                act("$p appears to be in perfect condition.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (perc && (perc < 2)) {
                act("$p appears to be in good shape.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if ((perc >= 2) && (perc < 4)) {
                act("$p appears to have some damage on it.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if ((perc >= 4) && (perc < 6)) {
                act("$p has a lot of damage on it.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if ((perc >= 6) && (perc < 8)) {
                act("$p is heavily damaged.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if ((perc >= 8) && (perc < 10)) {
                act("$p has extensive damage on it, and looks undriveable.", FALSE, ch, obj, 0,
                    TO_CHAR);
            }
            if (perc >= 10) {
                act("$p is completely totalled.", FALSE, ch, obj, 0, TO_CHAR);
            }
            perc = 0;
            for (temp_beast = obj->beasts; temp_beast; temp_beast = temp_beast->next)
                if (temp_beast->beast)
                    perc++;

            if ((obj->obj_flags.value[1] - perc) > 0) {
                if (obj->beasts)
                    sprintf(buf, "There is room for %d more beasts to be harnessed to" " $p.",
                            (obj->obj_flags.value[1] - perc));
                else
                    sprintf(buf, "There is room for %d beasts to be harnessed to $p.",
                            (obj->obj_flags.value[1] - perc));
                act(buf, FALSE, ch, obj, 0, TO_CHAR);
            } else {
                if (obj->beasts)
                    strcpy(buf, "There is no more room for beasts to be harnessed" " to $p.");
                else
                    strcpy(buf, "There is no room for beasts to be harnessed to $p.");
                act(buf, FALSE, ch, obj, 0, TO_CHAR);
            }
        }
        break;
    case ITEM_WEAPON:
        {
            /*
             * Weight 
             */
            if (GET_OBJ_WEIGHT(obj) > GET_STR(ch))
                send_to_char("You test its weight and decide that it is too heavy"
                             " for you to use.\n\r", ch);
            else
                send_to_char("You test its weight and decide that you could use it.\n\r", ch);

            /*
             * One-handed, Two-handed 
             */
            if (IS_SET(obj->obj_flags.extra_flags, OFL_NO_ES))
                send_to_char("This is a two-handed weapon.\n\r", ch);

            /*
             * Useable for skinning 
             */
            if (((obj->obj_flags.value[3] == (TYPE_CHOP - 300))
                 || (obj->obj_flags.value[3] == (TYPE_STAB - 300)))
                && !IS_SET(obj->obj_flags.extra_flags, OFL_NO_ES) && CAN_WEAR(obj, ITEM_EP))
                send_to_char("You could use this for skinning.\n\r", ch);

            /*
             * sheathable, where 
             */
            if (IS_SET(obj->obj_flags.extra_flags, OFL_NO_SHEATHE))
                send_to_char("You wouldn't be able to sheath it.\n\r", ch);
            else {
               if (IS_SET(obj->obj_flags.extra_flags, OFL_NO_ES))
                send_to_char("It would be too large to sheathe on a belt.\n\r", ch);
               if (!CAN_SHEATH_BACK(obj))
                send_to_char("It would be too small to wear on your back.\n\r", ch);
            }

            if ((isname("nakgear1", OSTR(obj, name))) && (ch->skills[LANG_SOUTH_ACCENT])) {
                act("$p looks like a militia weapon.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if ((isname("tulukgear1", OSTR(obj, name))) && (ch->skills[LANG_NORTH_ACCENT])) {
                act("$p looks like a militia weapon.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if ((obj->obj_flags.value[3] == 1) && (ch->skills[PROF_BLUDGEONING] || ch->skills[SKILL_CLUBMAKING])) {
                act("$p seems to be a bludgeoning weapon.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if ((obj->obj_flags.value[3] == 2) && (ch->skills[PROF_PIERCING] || ch->skills[SKILL_KNIFE_MAKING] || ch->skills[SKILL_SWORDMAKING])) {
                act("$p seems to be a piercing weapon.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (((obj->obj_flags.value[3] == 4) || (obj->obj_flags.value[3] == 3))
                && !IS_SET(obj->obj_flags.extra_flags, OFL_NO_ES) && CAN_WEAR(obj, ITEM_EP)) {
                act("$p would work well for skinning.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if (obj->obj_flags.value[3] == 3) {
                if (ch->skills[SKILL_BACKSTAB] || ch->skills[SKILL_KNIFE_MAKING]) {
                    act("$p seems to be a stabbing weapon.", FALSE, ch, obj, 0, TO_CHAR);
                } else if (ch->skills[PROF_PIERCING]) {
                    act("$p seems to be a piercing weapon.", FALSE, ch, obj, 0, TO_CHAR);
                }
            }
            if ((obj->obj_flags.value[3] == 4) && (ch->skills[PROF_CHOPPING] || ch->skills[SKILL_AXEMAKING])) {
                act("$p seems to be a chopping weapon.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if ((obj->obj_flags.value[3] == 5) && (ch->skills[PROF_SLASHING] || ch->skills[SKILL_SWORDMAKING] || ch->skills[SKILL_KNIFE_MAKING])) {
                act("$p seems to be a slashing weapon.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if ((obj->obj_flags.value[3] == 13) && (ch->skills[PROF_PIKE] || ch->skills[SKILL_SPEARMAKING])) {
                act("$p seems to be a pike weapon.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if ((obj->obj_flags.value[3] == 14) && (ch->skills[PROF_TRIDENT] || ch->skills[SKILL_SPEARMAKING])) {
                act("$p seems to be a trident weapon.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if ((obj->obj_flags.value[3] == 15) && (ch->skills[PROF_POLEARM] || ch->skills[SKILL_SPEARMAKING])) {
                act("$p seems to be a polearm weapon.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if ((obj->obj_flags.value[3] == 16) && (ch->skills[PROF_KNIFE] || ch->skills[SKILL_KNIFE_MAKING])) {
                act("$p seems to be a knife weapon.", FALSE, ch, obj, 0, TO_CHAR);
            }
            if ((obj->obj_flags.value[3] == 17) && (ch->skills[PROF_RAZOR] || ch->skills[SKILL_KNIFE_MAKING])) {
                act("$p seems to be a razor weapon.", FALSE, ch, obj, 0, TO_CHAR);
            }

	    // If it's poisoned, the poison_msg will not be NULL
	    poison_msg = format_poisoned_obj(obj, ch, obj->obj_flags.value[5]);
	    if (poison_msg)
		    act(poison_msg, FALSE, ch, obj, 0, TO_CHAR);
        }
        break;
    case ITEM_WORN:
        if (size_match(ch, obj)) {
            act("$p looks like it will fit you.", FALSE, ch, obj, 0, TO_CHAR);
        } else
            act("$p looks like it will require tailoring to fit you.", FALSE, ch, obj, 0, TO_CHAR);
        break;
    case ITEM_CONTAINER:
        /*
         * Hooded? 
         */
        if (IS_SET(obj->obj_flags.value[1], CONT_HOODED))
            send_to_char("It has a hood which can be raised or lowered.\n\r", ch);

        /*
         * Closeable? 
         */
        if (IS_SET(obj->obj_flags.value[1], CONT_CLOSEABLE))
            send_to_char("It can be opened and closed.\n\r", ch);

        /*
         * Lockable? 
         */
        if (obj->obj_flags.value[2] > 0)
            send_to_char("It can be locked and unlocked.\n\r", ch);

        /*
         * Approxmate Size 
         */
        obj_cap = obj->obj_flags.value[0];
        if (number(0, 1))
            obj_cap += number(1, 3);
        else
            obj_cap -= number(1, 3);
        obj_cap = MAX(1, obj_cap);
        sprintf(buf, "It could probably hold around %d stones.\n\r", obj_cap);
        send_to_char(buf, ch);

        break;
    case ITEM_SPICE: {
        int percent = PERCENTAGE(obj->obj_flags.value[0],
                       obj_default[obj->nr].value[0]);
    
        if ((obj->obj_flags.value[3] >= 1)
		&& (ch->skills[SKILL_POISON] || affected_by_spell(ch, SPELL_DETECT_POISON))) {
		poison_msg = format_poisoned_obj(obj, ch, obj->obj_flags.value[3]);
		if (poison_msg) 
		    act(poison_msg, FALSE, ch, obj, 0, TO_CHAR);
	}
        break;

        // not at max
        if( percent != 100 ) {
            // don't do it for smokes
            if( obj->obj_flags.value[2] == 1 ) {
               // 81-99% smoked
               if( percent > 80 ) {
                  cprintf(ch,"It has been partially smoked.\n\r");
               }
               // 61-80% - 3/4 full
               else if( percent > 60 ) {
                  cprintf(ch,"About a quarter of it has been smoked.\n\r");
               }
               // 41 - 60% - half
               else if( percent > 40 ) {
                  cprintf(ch,"About half of it is left.\n\r");
               } 
               // 16 - 40$ - less than a quarter left
               else if( percent > 15 ) {
                  cprintf(ch,"About a quarter of it is left.\n\r");
               }
               // 0 - 15%
               else {
                  cprintf(ch,"Only a small bit is left.\n\r");
               }
            } else {
               // 81-99% shaved
               if( percent > 80 ) {
                  cprintf(ch,"It has had some spice shaved off of it.\n\r");
               }
               // 61-80% - 3/4 full
               else if( percent > 60 ) {
                  cprintf(ch,"About a quarter of it has been shaved off.\n\r");
               }
               // 41 - 60% - half
               else if( percent > 40 ) {
                  cprintf(ch,"About half of it is left.\n\r");
               } 
               // 16 - 40$ - less than a quarter left
               else if( percent > 15 ) {
                  cprintf(ch,"About a quarter of it is left.\n\r");
               }
               // 0 - 15%
               else {
                  cprintf(ch,"Only a small bit is left.\n\r");
               }
            }
        }
        else { 
            act("You don't notice anything unusual about $p.", FALSE, ch, obj, 0, TO_CHAR);
        }
        break;
    }
    case ITEM_DRINKCON:
        if ((obj->obj_flags.value[5] >= 1)
		&& (ch->skills[SKILL_POISON] || affected_by_spell(ch, SPELL_DETECT_POISON))) {
		poison_msg = format_poisoned_obj(obj, ch, obj->obj_flags.value[5]);
		if (poison_msg)
		    act(poison_msg, FALSE, ch, obj, 0, TO_CHAR);
	}
        break;
    case ITEM_FOOD:
        if ((obj->obj_flags.value[3] >= 1)
		&& (ch->skills[SKILL_POISON] || affected_by_spell(ch, SPELL_DETECT_POISON))) {
		poison_msg = format_poisoned_obj(obj, ch, obj->obj_flags.value[3]);
		if (poison_msg)
		    act(poison_msg, FALSE, ch, obj, 0, TO_CHAR);
	}
        break;
    default:
        act("You don't notice anything unusual about $p.", FALSE, ch, obj, 0, TO_CHAR);
        break;
    }

    if( obj->carried_by == ch || obj->equipped_by == ch ) {
        char buf2[MAX_STRING_LENGTH];

        sprintf(buf, "It is %s", encumb_class(get_obj_encumberance(obj, ch)));

        switch (obj->obj_flags.type) {
        case ITEM_CONTAINER:
            if (!(obj->contains)) {
                strcat(buf, ", and empty.\n\r");
            } else {
                temp =
                    GET_OBJ_WEIGHT(obj) * 3 /
                    (obj->obj_flags.value[0] ? obj->obj_flags.value[0] : 1);
                temp = MAX(0, MIN(3, temp));
    
                sprintf(buf2, ", and %sfull.\n\r", fullness[temp]);
                strcat(buf, buf2);
            }
    
            break;
        case ITEM_DRINKCON:
            if (obj->obj_flags.value[1] <= 0) {
                strcat(buf, ", and empty.\n\r");
            } else {
                temp = obj->obj_flags.value[1] * 3 / obj->obj_flags.value[0];
                temp = MAX(0, MIN(3, temp));
    
                sprintf(buf2, ", and %sfull.\n\r", fullness[temp]);
                strcat(buf, buf2);
            }
            break;
        default:
            strcat(buf, ".\n\r");
            break;
        }
        send_to_char(buf, ch);
    } else {
        switch (obj->obj_flags.type) {
        case ITEM_CONTAINER:
            if (!(obj->contains)) {
                cprintf(ch, "It is empty.\n\r");
            } else {
                temp =
                    GET_OBJ_WEIGHT(obj) * 3 /
                    (obj->obj_flags.value[0] ? obj->obj_flags.value[0] : 1);
                temp = MAX(0, MIN(3, temp));
    
                cprintf(ch, "It is %sfull.\n\r", fullness[temp]);
            }
    
            break;
        case ITEM_DRINKCON:
            if (obj->obj_flags.value[1] <= 0) {
                cprintf(ch, "It is empty.\n\r");
            } else {
                temp = obj->obj_flags.value[1] * 3 / obj->obj_flags.value[0];
                temp = MAX(0, MIN(3, temp));
    
                cprintf(ch, "It is %sfull.\n\r", fullness[temp]);
            }
            break;
        default:
            break;
        }
    }

    return;
}                               /* End assess_object */

int
get_apparent_race(CHAR_DATA *ch) {
   int race = GET_RACE(ch);
   char buf[MAX_STRING_LENGTH];

   if (race == RACE_HALF_ELF) {
       if (get_char_extra_desc_value(ch, "[HALF_ELF_APP]", buf, sizeof(buf))) {
           race = atoi(buf);
       }
   } 

   return race;
}

void
cmd_assess(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buffer[50];
    char buf1[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    CHAR_DATA *tar_ch;
    CHAR_DATA *rch;
    OBJ_DATA *tar_obj;
    OBJ_DATA *shoes, *tshoes;
    struct time_info_data player_age, target_age;
    int ch_int, tar_int, percent, vis, age_diff = 0;
    int age_int, verbose = 0;
    int ch_apparent_race;
    int tch_apparent_race;
    char *age_names[] = {
        "young",
        "in adulthood",
        "mature",
        "middle-aged",
        "old",
        "ancient"
    };

    argument = one_argument(argument, buffer, sizeof(buffer));

    if (!*buffer) {
        send_to_char("Syntax: Assess (-v) <target>\n\r", ch);
        return;
    }

    if (STRINGS_SAME(buffer, "-v")) {
        verbose = 1;
        argument = one_argument(argument, buffer, sizeof(buffer));
    }

    if (!*buffer) {
        send_to_char("Assess whose condition?\n\r", ch);
        return;
    }

    tar_ch = get_char_room_vis(ch, buffer);
    if ((!tar_ch) || (!CAN_SEE(ch, tar_ch))) {
        if ((tar_obj = get_obj_room_vis(ch, buffer))) {
            assess_object(ch, tar_obj, verbose);
            return;
        }
        send_to_char("No one here by that name.\n\r", ch);
        return;
    }

    if ((vis = visibility(ch)) < 0) {
        send_to_char("No one here by that name.\n\r", ch);
        return;
    }

    ch_apparent_race = get_apparent_race(ch);
    tch_apparent_race = get_apparent_race(tar_ch);

    /* allow others to see this */
    if (tar_ch != ch) {
        for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
            if (rch != ch && watch_success(rch, ch, tar_ch, 0, SKILL_NONE)) {
                if (rch == tar_ch) {
                    cprintf(rch, "You notice %s glance your way.\n\r", PERS(rch, ch));
                } else {
                    strcpy( buf1, PERS(rch, ch));
                    cprintf(rch, "You notice %s glancing at %s.\n\r", buf1,
                            PERS(rch, tar_ch));
                }
            }
        }
    }

    /* if they're actively watching, give watch chance to avoid */
    if (is_char_actively_watching_char(tar_ch, ch)) {
        if (!watch_success(tar_ch, ch, NULL, 0, SKILL_NONE)) {
            cprintf(ch, "You notice %s is watching you.\n\r", PERS(ch, tar_ch));
        }
    }

    /* Get age, height, and weight stats */
    /*  But only if verbose is requested */
    if (verbose) {

        /* Show age of target relative to you. */
        target_age = age(tar_ch);

        if (ch_apparent_race == tch_apparent_race) {

            player_age = age(ch);
            age_diff = (player_age.year - target_age.year);

            sprintf(buf2, "%s",
                    ((GET_SEX(tar_ch) ==
                      SEX_MALE) ? "He" : ((GET_SEX(tar_ch) == SEX_FEMALE) ? "She" : "It")));

            if (age_diff == 0)
                strcat(buf2, " is about the same age as you.\n\r");
            if ((age_diff > 0) && (age_diff < 4))
                strcat(buf2, " is slightly younger than you.\n\r");
            if ((age_diff >= 4) && (age_diff < 10))
                strcat(buf2, " is younger than you.\n\r");
            if ((age_diff >= 10) && (age_diff < 20))
                strcat(buf2, " is quite a bit younger than you.\n\r");
            if (age_diff >= 20)
                strcat(buf2, " is very young compared to you.\n\r");
            if ((age_diff < 0) && (age_diff > -4))
                strcat(buf2, " is slightly older than you.\n\r");
            if ((age_diff <= -4) && (age_diff > -10))
                strcat(buf2, " is older than you.\n\r");
            if ((age_diff <= -10) && (age_diff > -20))
                strcat(buf2, " is quite a bit older than you.\n\r");
            if (age_diff <= -20)
                strcat(buf2, " is very old compared to you.\n\r");

            if (ch != tar_ch)
                send_to_char(buf2, ch);
        } /* End display age difference */

        // always show age category
        age_int = age_class(get_virtual_age(target_age.year, tch_apparent_race));
        age_int = MAX(0, age_int);
        age_int = MIN(5, age_int);
        sprintf(buf2, "%s appears %s for %s race.\n\r",
         ((GET_SEX(tar_ch) == SEX_MALE) ? "He" 
          : ((GET_SEX(tar_ch) == SEX_FEMALE) ? "She" : "It")),
         age_names[age_int], HSHR(tar_ch));

        if (ch != tar_ch)
            send_to_char(buf2, ch);

        /* Height comparison */
        ch_int = ch->player.height;
        tar_int = tar_ch->player.height;

        /* If you are wearing high-heeled shoes, add two inches */
        /* Sanvean 2/27/2004                                    */
        shoes = ch->equipment[WEAR_FEET];

        if (shoes) {
            if ((isname("high-heeled", OSTR(shoes, name)))
                || (isname("platform", OSTR(shoes, name)))
                || (isname("stiletto", OSTR(shoes, name)))
                || (isname("elevated", OSTR(shoes, name))))
                ch_int += 2;
        }

        tshoes = tar_ch->equipment[WEAR_FEET];

        if (tshoes) {
            if ((isname("high-heeled", OSTR(tshoes, name)))
                || (isname("platform", OSTR(tshoes, name)))
                || (isname("stiletto", OSTR(tshoes, name)))
                || (isname("elevated", OSTR(tshoes, name))))
                tar_int += 2;
        }

        sprintf(buf2, "%s",
                ((GET_SEX(tar_ch) ==
                  SEX_MALE) ? "He" : ((GET_SEX(tar_ch) == SEX_FEMALE) ? "She" : "It")));

        if (ch_int > tar_int) { /* Ch is taller than tar_ch */
            if ((ch_int / 5) >= tar_int)
                strcat(buf2, " is many times shorter than you.\n\r");
            else if ((ch_int / 3) >= tar_int)
                strcat(buf2, " is less than a third your height.\n\r");
            else if ((ch_int / 2) >= tar_int)
                strcat(buf2, " is less than half your height.\n\r");
            else if ((ch_int - tar_int) <= 5)
                strcat(buf2, " is slightly shorter than you.\n\r");
            else if ((ch_int - tar_int) <= 12)
                strcat(buf2, " is shorter than you.\n\r");
            else
                strcat(buf2, " is quite a bit shorter than you.\n\r");
        } else {
            if (ch_int < tar_int) {     /* Tar_ch is taller than ch */
                if ((tar_int / 5) >= ch_int)
                    strcat(buf2, " towers above you, many times your height.\n\r");
                else if ((tar_int / 3) >= ch_int)
                    strcat(buf2, " is over three times your height.\n\r");
                else if ((tar_int / 2) >= ch_int)
                    strcat(buf2, " is over twice your height.\n\r");
                else if ((tar_int - ch_int) <= 5)
                    strcat(buf2, " is slightly taller than you.\n\r");
                else if ((tar_int - ch_int) <= 12)
                    strcat(buf2, " is taller than you.\n\r");
                else
                    strcat(buf2, " is quite a bit taller than you.\n\r");
            } else              /* Same size */
                strcat(buf2, " is about the same size as you.\n\r");
        }

        if (ch != tar_ch)
            send_to_char(buf2, ch);

        ch_int = ch->player.weight;
        tar_int = tar_ch->player.weight;
        if (ch_int > tar_int) { /* Ch is heavier than tar_ch */
            if ((ch_int / 5) >= tar_int)
                strcpy(buf2, "You are many times heavier than ");
            else if ((ch_int / 3) >= tar_int)
                strcpy(buf2, "You are over three times heavier than ");
            else if ((ch_int / 2) >= tar_int)
                strcpy(buf2, "You are over twice as heavy as ");
            else if ((ch_int - tar_int) <= 5)
                strcpy(buf2, "You are slightly heavier than ");
            else if ((ch_int - tar_int) <= 12)
                strcpy(buf2, "You are heavier than ");
            else
                strcpy(buf2, "You are quite a bit heavier than ");

            if (GET_SEX(tar_ch) == SEX_MALE)
                strcat(buf2, "him.\n\r");
            else {
                if (GET_SEX(tar_ch) == SEX_FEMALE)
                    strcat(buf2, "her.\n\r");
                else
                    strcat(buf2, "it.\n\r");
            }
        } else {
            sprintf(buf2, "%s",
                    ((GET_SEX(tar_ch) ==
                      SEX_MALE) ? "He" : ((GET_SEX(tar_ch) == SEX_FEMALE) ? "She" : "It")));
            if (ch_int < tar_int) {     /* Tar_ch is heavier than ch */
                if ((tar_int / 5) >= ch_int)
                    strcat(buf2, " is many times heavier than you.\n\r");
                else if ((tar_int / 3) >= ch_int)
                    strcat(buf2, " is over three times heavier than you.\n\r");
                else if ((tar_int / 2) >= ch_int)
                    strcat(buf2, " is over twice as heavy as you.\n\r");
                else if ((tar_int - ch_int) <= 5)
                    strcat(buf2, " is slightly heavier than you.\n\r");
                else if ((tar_int - ch_int) <= 12)
                    strcat(buf2, " is heavier than you.\n\r");
                else
                    strcat(buf2, " is quite a bit heavier than you.\n\r");
            } else              /* Same weight */
                strcat(buf2, " weighs about the same as you.\n\r");
        }

        if (ch != tar_ch)
            send_to_char(buf2, ch);
    }

    /*   End Verbose mode */
    /* I don't know how this is possible but it's best to be */
    /* bulletproof in case something strange happens.  -Ur   */
    if (GET_MAX_HIT(tar_ch) > 0)
        percent = ((100 * GET_HIT(tar_ch)) / GET_MAX_HIT(tar_ch));
    else
        percent = -1;

    strcpy(buf2, capitalize(PERS(ch, tar_ch)));

    if (percent >= 100)
        strcat(buf2, " is in excellent condition.\n\r");
    else if (percent >= 90)
        strcat(buf2, " looks relatively fit.\n\r");
    else if (percent >= 70)
        strcat(buf2, " is in moderate condition.\n\r");
    else if (percent >= 50)
        strcat(buf2, " does not look well.\n\r");
    else if (percent >= 30)
        strcat(buf2, " is in poor condition.\n\r");
    else if (percent >= 15)
        strcat(buf2, " is in terrible condition.\n\r");
    else if (percent >= 0)
        strcat(buf2, " looks near death.\n\r");
    else
        strcat(buf2, " is in critical condition.\n\r");

    send_to_char(buf2, ch);

    if (GET_MAX_MOVE(tar_ch) > 0)
        percent = ((100 * GET_MOVE(tar_ch)) / GET_MAX_MOVE(tar_ch));
    else
        percent = -1;

    strcpy(buf2, capitalize(PERS(ch, tar_ch)));

    if (percent >= 100)
        strcat(buf2, " looks completely rested.\n\r");
    else if (percent >= 90)
        strcat(buf2, " does not look tired.\n\r");
    else if (percent >= 70)
        strcat(buf2, " looks a little weary.\n\r");
    else if (percent >= 50)
        strcat(buf2, " looks tired.\n\r");
    else if (percent >= 30)
        strcat(buf2, " looks very tired.\n\r");
    else if (percent >= 15)
        strcat(buf2, " looks exhausted.\n\r");
    else if (percent >= 0)
        strcat(buf2, " looks completely exhausted.\n\r");
    else
        strcat(buf2, " looks dead.\n\r");

    send_to_char( buf2, ch );

    if (GET_MAX_STUN(tar_ch) > 0)
        percent = ((100 * GET_STUN(tar_ch)) / GET_MAX_STUN(tar_ch));
    else
        percent = -1;

/*
    strcpy(buf2, capitalize(PERS(ch, tar_ch)));

    if (percent <= 0 )
        strcat(buf2, " is unconscious.\n\r");
    else if (percent <= 15)
        strcat(buf2, " looks very dazed.\n\r");
    else if (percent <= 30)
        strcat(buf2, " looks dazed.\n\r");
    else if (percent <= 50)
        strcat(buf2, " looks slightly dazed.\n\r");

    if( percent <= 50 )
        send_to_char( buf2, ch );
*/

    if( suffering_from_poison(tar_ch, POISON_GENERIC) ) {
       strcpy(buf2, capitalize(PERS(ch, tar_ch)));
       strcat( buf2, " looks very sick and is shivering uncontrollably.\n\r");
       send_to_char( buf2, ch );
    }

    if( suffering_from_poison(tar_ch, POISON_METHELINOC) ) {
       strcpy(buf2, capitalize(PERS(ch, tar_ch)));
       strcat( buf2, "'s lips are a pale shade of blue.\n\r");
       send_to_char( buf2, ch );
    }

    if( suffering_from_poison(tar_ch, POISON_GRISHEN) ) {
       strcpy(buf2, capitalize(PERS(ch, tar_ch)));
       sprintf( buf1, "'s veins are bulging and %s breathing is shallow.\n\r",
        HSHR(tar_ch));
       strcat(buf2, buf1);
       send_to_char( buf2, ch );
    }

    buf2[0] = '\0';

    /* For those who can... sense a like elementalist */
    if (is_guild_elementalist(GET_GUILD(ch)) && (tar_ch != ch)
        && (GET_GUILD(ch) == GET_GUILD(tar_ch)) && affected_by_spell(ch, SPELL_IDENTIFY)) {
        sprintf(buf1, "You sense a familiar presence within %s.\n\r",
                ((GET_SEX(tar_ch) ==
                  SEX_MALE) ? "him" : ((GET_SEX(tar_ch) == SEX_FEMALE) ? "her" : "it")));
        strcat(buf2, buf1);
    }

    /* back to verbose checks, just want them to output later */
    if (verbose) {
        int n = -1;
        /* show if they're armed */
        if (get_weapon(tar_ch, &n) != NULL) {
            sprintf(buf1, "%s is armed.\n\r", capitalize(PERS(ch, tar_ch)));
            strcat(buf2, buf1);
        }
    }

    send_to_char(buf2, ch);

}

int
is_secret(struct room_direction_data *dir)
{

    return (IS_SET(dir->exit_info, EX_SECRET) && (dir->to_room->sector_type != SECT_AIR));
}

/* Started 05/17/2000 -nessalin */
void
sand_wall_messages(struct char_data *ch)
{
    int ex;
    char buffer[MAX_STRING_LENGTH] = "The area is thronged with sand walls.";
    int EXIT_ONE, EXIT_TWO, EXIT_THREE, EXIT_FOUR, EXIT_FIVE, EXIT_SIX, wall_counter =
        0, B_NORTH, B_SOUTH, B_EAST, B_WEST, B_UP, B_DOWN;

    EXIT_ONE = -1;
    EXIT_TWO = -1;
    EXIT_THREE = -1;
    EXIT_FOUR = -1;
    EXIT_FIVE = -1;
    EXIT_SIX = -1;
    B_NORTH = 0;
    B_EAST = 0;
    B_SOUTH = 0;
    B_WEST = 0;
    B_UP = 0;
    B_DOWN = 0;

    for (ex = 0; ex < 6; ex++)
        if (ch->in_room->direction[ex]
            && IS_SET(ch->in_room->direction[ex]->exit_info, EX_SPL_SAND_WALL)) {
            if (EXIT_ONE == -1)
                EXIT_ONE = ex;
            else if (EXIT_TWO == -1)
                EXIT_TWO = ex;
            else if (EXIT_THREE == -1)
                EXIT_THREE = ex;
            else if (EXIT_FOUR == -1)
                EXIT_FOUR = ex;
            else if (EXIT_FIVE == -1)
                EXIT_FIVE = ex;
            else if (EXIT_SIX == -1)
                EXIT_SIX = ex;

            if (ex == 0)
                B_NORTH = 1;
            else if (ex == 1)
                B_EAST = 1;
            else if (ex == 2)
                B_SOUTH = 1;
            else if (ex == 3)
                B_WEST = 1;
            else if (ex == 4)
                B_UP = 1;
            else if (ex == 5)
                B_DOWN = 1;
            wall_counter++;
        }

    if (wall_counter) {
        switch (wall_counter) {
        case 1:
            sprintf(buffer, "A giant wall of sand blocks the %s " "exit.\n\r", dirs[EXIT_ONE]);
            break;
        case 2:
            if ((B_NORTH && (B_EAST || B_WEST))
                || (B_SOUTH && (B_EAST || B_WEST)))
                sprintf(buffer, "A giant, l-shaped wall of sand blocks the %s " "and %s exits.\n\r",
                        dirs[EXIT_ONE], dirs[EXIT_TWO]);
            else if ((B_NORTH && B_SOUTH) || (B_EAST && B_WEST)) {
                sprintf(buffer, "A giant wall of sand blocks the %s exit.\n\r", dirs[EXIT_ONE]);
                send_to_char(buffer, ch);
                sprintf(buffer, "A giant wall of sand blocks the %s exit.\n\r", dirs[EXIT_TWO]);
            } else if (B_UP) {
                sprintf(buffer,
                        "A giant wall of sand blocks the %s exit, "
                        "curving overhead to block the way up.\n\r", dirs[EXIT_ONE]);
            } else {            /* has to be down */

                sprintf(buffer,
                        "The ground here is a solid mass of sand, with a"
                        " wall growing up to the %s blocking the way.\n\r", dirs[EXIT_ONE]);
            }
            break;
        case 3:
            if (!B_UP && !B_DOWN)
                sprintf(buffer,
                        "A giant, u-shaped wall of sand blocks the %s, %s," " and %s exits.\n\r",
                        dirs[EXIT_ONE], dirs[EXIT_TWO], dirs[EXIT_THREE]);
            else if (B_UP) {
                if ((B_NORTH && B_SOUTH) || (B_EAST && B_WEST))
                    sprintf(buffer,
                            "An arch of sand stands here, blocking the " "%s, %s and %s exits.\n\r",
                            dirs[EXIT_ONE], dirs[EXIT_TWO], dirs[EXIT_THREE]);
                else
                    sprintf(buffer, "A partial dome of sand blocks the %s, %s and " "%s exits.\n\r",
                            dirs[EXIT_ONE], dirs[EXIT_TWO], dirs[EXIT_THREE]);
            } else {
                sprintf(buffer,
                        "The ground here is a solid mass of sand, with "
                        "walls growing up to the %s and %s, blocking the way.\n\r",
                        dirs[EXIT_ONE], dirs[EXIT_TWO]);
            }
            break;
        case 4:
            if (!B_UP && !B_DOWN)
                sprintf(buffer, "A giant ring of sand blocks the %s, %s, %s, and " "%s exits.\n\r",
                        dirs[EXIT_ONE], dirs[EXIT_TWO], dirs[EXIT_THREE], dirs[EXIT_FOUR]);
            else {
                if (B_UP && B_DOWN) {
                    if (B_EAST && B_WEST)
                        sprintf(buffer,
                                "A tunnel of sand runs north-south here, "
                                "blocking the %s, %s, %s, and %s exits.\n\r", dirs[EXIT_ONE],
                                dirs[EXIT_TWO], dirs[EXIT_THREE], dirs[EXIT_FOUR]);
                    else {
                        if (B_NORTH && B_SOUTH)
                            sprintf(buffer,
                                    "A tunnel of sand runs east-west here, "
                                    "blocking the %s, %s, %s, and %s exits.\n\r", dirs[EXIT_ONE],
                                    dirs[EXIT_TWO], dirs[EXIT_THREE], dirs[EXIT_FOUR]);
                        else
                            sprintf(buffer,
                                    "A partial sphere of sand blocks the %s, "
                                    "%s, %s, and %s exits.\n\r", dirs[EXIT_ONE], dirs[EXIT_TWO],
                                    dirs[EXIT_THREE], dirs[EXIT_FOUR]);
                    }
                }
            }
            break;
        case 5:
            sprintf(buffer,
                    "A partial sphere of sand blocks the %s, %s, %s, %s," " and %s exits.\n\r",
                    dirs[EXIT_ONE], dirs[EXIT_TWO], dirs[EXIT_THREE], dirs[EXIT_FOUR],
                    dirs[EXIT_FIVE]);
            break;
        case 6:
            sprintf(buffer, "A sphere of solid sand encloses this area blocking" " all exits.\n\r");
            break;
        default:
            errorlog("Too many blocked exits with wall of sand.");
        }
        send_to_char(buffer, ch);
    }
}

/* Started 05/17/2000 -nessalin */
void
blade_wall_messages(struct char_data *ch)
{
    int ex;
    char buffer[MAX_STRING_LENGTH];
    int EXIT_ONE, EXIT_TWO, EXIT_THREE, EXIT_FOUR, EXIT_FIVE, EXIT_SIX, wall_counter =
        0, B_NORTH, B_SOUTH, B_EAST, B_WEST, B_UP, B_DOWN;

    EXIT_ONE = -1;
    EXIT_TWO = -1;
    EXIT_THREE = -1;
    EXIT_FOUR = -1;
    EXIT_FIVE = -1;
    EXIT_SIX = -1;
    B_NORTH = 0;
    B_EAST = 0;
    B_SOUTH = 0;
    B_WEST = 0;
    B_UP = 0;
    B_DOWN = 0;

    for (ex = 0; ex < 6; ex++)
        if (ch->in_room->direction[ex]
            && IS_SET(ch->in_room->direction[ex]->exit_info, EX_SPL_BLADE_BARRIER)) {
            if (EXIT_ONE == -1)
                EXIT_ONE = ex;
            else if (EXIT_TWO == -1)
                EXIT_TWO = ex;
            else if (EXIT_THREE == -1)
                EXIT_THREE = ex;
            else if (EXIT_FOUR == -1)
                EXIT_FOUR = ex;
            else if (EXIT_FIVE == -1)
                EXIT_FIVE = ex;
            else if (EXIT_SIX == -1)
                EXIT_SIX = ex;

            if (ex == 0)
                B_NORTH = 1;
            else if (ex == 1)
                B_EAST = 1;
            else if (ex == 2)
                B_SOUTH = 1;
            else if (ex == 3)
                B_WEST = 1;
            else if (ex == 4)
                B_UP = 1;
            else if (ex == 5)
                B_DOWN = 1;
            wall_counter++;
        }

    if (wall_counter) {
        switch (wall_counter) {
        case 1:
            sprintf(buffer, "A mass of whirling blades blocks the %s exit.\n\r", dirs[EXIT_ONE]);
            break;
        case 2:
            if ((B_NORTH && (B_EAST || B_WEST))
                || (B_SOUTH && (B_EAST || B_WEST)))
                sprintf(buffer,
                        "A giant, l-shaped mass of whirling blades blocks"
                        " the %s and %s exits.\n\r", dirs[EXIT_ONE], dirs[EXIT_TWO]);
            else if ((B_NORTH && B_SOUTH) || (B_EAST && B_WEST)) {
                sprintf(buffer, "A mass of whirling blades blocks the %s exit.\n\r",
                        dirs[EXIT_ONE]);
                send_to_char(buffer, ch);
                sprintf(buffer, "A mass of whirling blades blocks the %s exit.\n\r",
                        dirs[EXIT_TWO]);
            } else if (B_UP) {
                sprintf(buffer,
                        "A mass of whirling blades blocks %s exit, "
                        "curving overhead to block the way up.\n\r", dirs[EXIT_ONE]);
            } else {            /* has to be down */

                sprintf(buffer,
                        "The ground here is a spinning pattern of blades"
                        ", with a wall growing up to the %s, blocking the way.\n\r",
                        dirs[EXIT_ONE]);
            }
            break;
        case 3:
            if (!B_UP && !B_DOWN)
                sprintf(buffer,
                        "A giant, u-shaped mass of spinning blades blocks"
                        " the %s, %s, and %s exits.\n\r", dirs[EXIT_ONE], dirs[EXIT_TWO],
                        dirs[EXIT_THREE]);
            else if (B_UP) {
                if ((B_NORTH && B_SOUTH) || (B_EAST && B_WEST))
                    sprintf(buffer,
                            "An arch of moving blades is here, blocking "
                            "the %s, %s and %s exits.\n\r", dirs[EXIT_ONE], dirs[EXIT_TWO],
                            dirs[EXIT_THREE]);
                else
                    sprintf(buffer,
                            "A partial dome of moving blades blocks the %s,"
                            " %s and %s exits.\n\r", dirs[EXIT_ONE], dirs[EXIT_TWO],
                            dirs[EXIT_THREE]);
            } else {
                sprintf(buffer,
                        "The ground here is a spinning pattern of blades"
                        " with a wall growing up to the %s and %s, blocking the way.\n\r",
                        dirs[EXIT_ONE], dirs[EXIT_TWO]);
            }
            break;
        case 4:
            if (!B_UP && !B_DOWN)
                sprintf(buffer,
                        "A spinning ring of blades surrounds this area "
                        "blocking the %s, %s, %s, and %s exits.\n\r", dirs[EXIT_ONE],
                        dirs[EXIT_TWO], dirs[EXIT_THREE], dirs[EXIT_FOUR]);
            else {
                if (B_UP && B_DOWN) {   /*here */
                    if (B_EAST && B_WEST)
                        sprintf(buffer,
                                "A tunnel of shifting blades runs north-south"
                                " here, blocking the %s, %s, %s, and %s exits.\n\r", dirs[EXIT_ONE],
                                dirs[EXIT_TWO], dirs[EXIT_THREE], dirs[EXIT_FOUR]);
                    else {
                        if (B_NORTH && B_SOUTH)
                            sprintf(buffer,
                                    "A tunnel of shifting blades runs east-west "
                                    "here, blocking the %s, %s, %s, and %s exits.\n\r",
                                    dirs[EXIT_ONE], dirs[EXIT_TWO], dirs[EXIT_THREE],
                                    dirs[EXIT_FOUR]);
                        else
                            sprintf(buffer,
                                    "A partial sphere of moving blades blocks"
                                    " the %s, %s, %s, and %s exits.\n\r", dirs[EXIT_ONE],
                                    dirs[EXIT_TWO], dirs[EXIT_THREE], dirs[EXIT_FOUR]);
                    }
                }
            }
            break;
        case 5:
            sprintf(buffer,
                    "A partial sphere of moving blades blocks the %s, %s,"
                    " %s, %s, and %s exits.\n\r", dirs[EXIT_ONE], dirs[EXIT_TWO], dirs[EXIT_THREE],
                    dirs[EXIT_FOUR], dirs[EXIT_FIVE]);
            break;
        case 6:
            sprintf(buffer,
                    "A sphere of moving blades encloses this area " "blocking all exits.\n\r");
            break;
        default:
            errorlog("Too many blocked exits with wall of blades.");
        }
        send_to_char(buffer, ch);
    }
}

/* Started 05/17/2000 -nessalin */
void
thorn_wall_messages(struct char_data *ch)
{
    int ex;
    char buffer[MAX_STRING_LENGTH];
    int EXIT_ONE, EXIT_TWO, EXIT_THREE, EXIT_FOUR, EXIT_FIVE, EXIT_SIX, wall_counter =
        0, B_NORTH, B_SOUTH, B_EAST, B_WEST, B_UP, B_DOWN;

    EXIT_ONE = -1;
    EXIT_TWO = -1;
    EXIT_THREE = -1;
    EXIT_FOUR = -1;
    EXIT_FIVE = -1;
    EXIT_SIX = -1;
    B_NORTH = 0;
    B_EAST = 0;
    B_SOUTH = 0;
    B_WEST = 0;
    B_UP = 0;
    B_DOWN = 0;

    for (ex = 0; ex < 6; ex++)
        if (ch->in_room->direction[ex]
            && IS_SET(ch->in_room->direction[ex]->exit_info, EX_SPL_THORN_WALL)) {
            if (EXIT_ONE == -1)
                EXIT_ONE = ex;
            else if (EXIT_TWO == -1)
                EXIT_TWO = ex;
            else if (EXIT_THREE == -1)
                EXIT_THREE = ex;
            else if (EXIT_FOUR == -1)
                EXIT_FOUR = ex;
            else if (EXIT_FIVE == -1)
                EXIT_FIVE = ex;
            else if (EXIT_SIX == -1)
                EXIT_SIX = ex;

            if (ex == 0)
                B_NORTH = 1;
            else if (ex == 1)
                B_EAST = 1;
            else if (ex == 2)
                B_SOUTH = 1;
            else if (ex == 3)
                B_WEST = 1;
            else if (ex == 4)
                B_UP = 1;
            else if (ex == 5)
                B_DOWN = 1;
            wall_counter++;
        }

    if (wall_counter) {
        switch (wall_counter) {
        case 1:
            sprintf(buffer, "A tangled wall of thorns blocks the %s " "exit.\n\r", dirs[EXIT_ONE]);
            break;
        case 2:
            if ((B_NORTH && (B_EAST || B_WEST))
                || (B_SOUTH && (B_EAST || B_WEST)))
                sprintf(buffer,
                        "A giant, l-shaped wall of tangled thorns blocks the"
                        " %s and %s exits.\n\r", dirs[EXIT_ONE], dirs[EXIT_TWO]);
            else if ((B_NORTH && B_SOUTH) || (B_EAST && B_WEST)) {
                sprintf(buffer, "A tangled wall of thorns blocks the %s exit.\n\r", dirs[EXIT_ONE]);
                send_to_char(buffer, ch);
                sprintf(buffer, "A tangled wall of thorns blocks the %s exit.\n\r", dirs[EXIT_TWO]);
            } else if (B_UP) {
                sprintf(buffer,
                        "A tangled wall of thorns blocks the %s exit, "
                        "curving overhead to block the way up.\n\r", dirs[EXIT_ONE]);
            } else {            /* has to be down */

                sprintf(buffer,
                        "The ground here is covered with a thick growth"
                        " of tangled thorns, a wall growing up to the %s, blocking the way.\n\r",
                        dirs[EXIT_ONE]);
            }
            break;
        case 3:
            if (!B_UP && !B_DOWN)
                sprintf(buffer,
                        "A tangled, u-shaped wall of thorns blocks the %s, "
                        "%s, and %s exits.\n\r", dirs[EXIT_ONE], dirs[EXIT_TWO], dirs[EXIT_THREE]);
            else if (B_UP) {
                if ((B_NORTH && B_SOUTH) || (B_EAST && B_WEST))
                    sprintf(buffer,
                            "An arch of tangled thorns stands here, "
                            "blocking the %s, %s and %s exits.\n\r", dirs[EXIT_ONE], dirs[EXIT_TWO],
                            dirs[EXIT_THREE]);
                else
                    sprintf(buffer,
                            "A partial dome of thorns blocks the %s, %s and " "%s exits.\n\r",
                            dirs[EXIT_ONE], dirs[EXIT_TWO], dirs[EXIT_THREE]);
            } else {
                sprintf(buffer,
                        "The ground here is covered with a thick growth"
                        " of tangled thorns, a wall growing up to the %s and %s,"
                        " blocking the way.\n\r", dirs[EXIT_ONE], dirs[EXIT_TWO]);
            }
            break;
        case 4:
            if (!B_UP && !B_DOWN)
                sprintf(buffer,
                        "A tangled ring of thorns have grown here, "
                        "blocking the %s, %s, %s, and %s exits.\n\r", dirs[EXIT_ONE],
                        dirs[EXIT_TWO], dirs[EXIT_THREE], dirs[EXIT_FOUR]);
            else {
                if (B_UP && B_DOWN) {
                    if (B_EAST && B_WEST)
                        sprintf(buffer,
                                "A tunnel of thorns runs north-south here, "
                                "blocking the %s, %s, %s, and %s exis.\n\r", dirs[EXIT_ONE],
                                dirs[EXIT_TWO], dirs[EXIT_THREE], dirs[EXIT_FOUR]);
                    else {
                        if (B_NORTH && B_SOUTH)
                            sprintf(buffer,
                                    "A tunnel of thorns runs east-west here, "
                                    "blocking the %s, %s, %s, and %s exits.\n\r", dirs[EXIT_ONE],
                                    dirs[EXIT_TWO], dirs[EXIT_THREE], dirs[EXIT_FOUR]);
                        else
                            sprintf(buffer,
                                    "A partial sphere of thorns blocks the "
                                    "%s, %s, %s, and %s exits.\n\r", dirs[EXIT_ONE], dirs[EXIT_TWO],
                                    dirs[EXIT_THREE], dirs[EXIT_FOUR]);
                    }
                }
            }
            break;
        case 5:
            sprintf(buffer,
                    "A partial sphere of thorns blocks the %s, %s, %s, %s," " and %s exits.\n\r",
                    dirs[EXIT_ONE], dirs[EXIT_TWO], dirs[EXIT_THREE], dirs[EXIT_FOUR],
                    dirs[EXIT_FIVE]);
            break;
        case 6:
            sprintf(buffer,
                    "A sphere of thickly interwoven thorns encloses this "
                    "area blocking all exits.\n\r");
            break;
        default:
            errorlog("Too many blocked exits with wall of thorns.");
        }
        send_to_char(buffer, ch);
    }
}

/* Started 05/17/2000 -nessalin */
void
fire_wall_messages(struct char_data *ch)
{
    int ex;
    char buffer[MAX_STRING_LENGTH];
    int EXIT_ONE, EXIT_TWO, EXIT_THREE, EXIT_FOUR, EXIT_FIVE, EXIT_SIX, wall_counter =
        0, B_NORTH, B_SOUTH, B_EAST, B_WEST, B_UP, B_DOWN;

    EXIT_ONE = -1;
    EXIT_TWO = -1;
    EXIT_THREE = -1;
    EXIT_FOUR = -1;
    EXIT_FIVE = -1;
    EXIT_SIX = -1;
    B_NORTH = 0;
    B_EAST = 0;
    B_SOUTH = 0;
    B_WEST = 0;
    B_UP = 0;
    B_DOWN = 0;

    /* Following not used (I don't think) -nessalin */
    /* EXIT_ONE=-1;EXIT_TWO=-1;EXIT_THREE=-1;EXIT_FOUR=-1;EXIT_FIVE=-1;
     * EXIT_SIX=-1; */
    /*   B_NORTH=0;B_EAST=0;B_SOUTH=0;B_WEST=0;B_UP=0;B_DOWN=0; */
    for (ex = 0; ex < 6; ex++)
        if (ch->in_room->direction[ex]
            && IS_SET(ch->in_room->direction[ex]->exit_info, EX_SPL_FIRE_WALL)) {
            if (EXIT_ONE == -1)
                EXIT_ONE = ex;
            else if (EXIT_TWO == -1)
                EXIT_TWO = ex;
            else if (EXIT_THREE == -1)
                EXIT_THREE = ex;
            else if (EXIT_FOUR == -1)
                EXIT_FOUR = ex;
            else if (EXIT_FIVE == -1)
                EXIT_FIVE = ex;
            else if (EXIT_SIX == -1)
                EXIT_SIX = ex;

            if (ex == 0)
                B_NORTH = 1;
            else if (ex == 1)
                B_EAST = 1;
            else if (ex == 2)
                B_SOUTH = 1;
            else if (ex == 3)
                B_WEST = 1;
            else if (ex == 4)
                B_UP = 1;
            else if (ex == 5)
                B_DOWN = 1;
            wall_counter++;
        }

    if (wall_counter) {
        switch (wall_counter) {
        case 1:
            sprintf(buffer, "A giant wall of fire blocks the %s " "exit.\n\r", dirs[EXIT_ONE]);
            break;
        case 2:
            if ((B_NORTH && (B_EAST || B_WEST))
                || (B_SOUTH && (B_EAST || B_WEST)))
                sprintf(buffer, "A giant, l-shaped wall of fire blocks the %s " "and %s exits.\n\r",
                        dirs[EXIT_ONE], dirs[EXIT_TWO]);
            else if ((B_NORTH && B_SOUTH) || (B_EAST && B_WEST)) {
                sprintf(buffer, "A giant wall of fire blocks the %s exit.\n\r", dirs[EXIT_ONE]);
                send_to_char(buffer, ch);
                sprintf(buffer, "A giant wall of fire blocks the %s exit.\n\r", dirs[EXIT_TWO]);
            } else if (B_UP) {
                sprintf(buffer,
                        "A giant wall of fire blocks the %s exit, "
                        "curving overhead to block the way up.\n\r", dirs[EXIT_ONE]);
            } else {            /* has to be down */

                sprintf(buffer,
                        "The ground here is covered with flames, with a"
                        " wall growing up to the %s blocking the way.\n\r", dirs[EXIT_ONE]);
            }
            break;
        case 3:
            if (!B_UP && !B_DOWN)
                sprintf(buffer,
                        "A giant, u-shaped wall of fire blocks the %s, %s," " and %s exits.\n\r",
                        dirs[EXIT_ONE], dirs[EXIT_TWO], dirs[EXIT_THREE]);
            else if (B_UP) {
                if ((B_NORTH && B_SOUTH) || (B_EAST && B_WEST))
                    sprintf(buffer,
                            "An arch of fire stands here, blocking the " "%s, %s and %s exits.\n\r",
                            dirs[EXIT_ONE], dirs[EXIT_TWO], dirs[EXIT_THREE]);
                else
                    sprintf(buffer, "A partial dome of fire blocks the %s, %s and " "%s exits.\n\r",
                            dirs[EXIT_ONE], dirs[EXIT_TWO], dirs[EXIT_THREE]);
            } else {
                sprintf(buffer,
                        "The ground here is covered with fire, with "
                        "walls growing up to the %s and %s, blocking the way." "\n\r",
                        dirs[EXIT_ONE], dirs[EXIT_TWO]);
            }
            break;
        case 4:
            if (!B_UP && !B_DOWN)
                sprintf(buffer,
                        "A giant ring of fire burns here, blocking the "
                        "%s, %s, %s, and %s exits.\n\r", dirs[EXIT_ONE], dirs[EXIT_TWO],
                        dirs[EXIT_THREE], dirs[EXIT_FOUR]);
            else {
                if (B_UP && B_DOWN) {
                    if (B_EAST && B_WEST)
                        sprintf(buffer,
                                "A tunnel of fire runs north-south here, "
                                "blocking the %s, %s, %s, and %s exits.\n\r", dirs[EXIT_ONE],
                                dirs[EXIT_TWO], dirs[EXIT_THREE], dirs[EXIT_FOUR]);
                    else {
                        if (B_NORTH && B_SOUTH)
                            sprintf(buffer,
                                    "A tunnel of fire runs east-west here, "
                                    "blocking the %s, %s, %s, and %s exits.\n\r", dirs[EXIT_ONE],
                                    dirs[EXIT_TWO], dirs[EXIT_THREE], dirs[EXIT_FOUR]);
                        else
                            sprintf(buffer,
                                    "A partial sphere of fire blocks the "
                                    "%s, %s, %s, and %s exits.\n\r", dirs[EXIT_ONE], dirs[EXIT_TWO],
                                    dirs[EXIT_THREE], dirs[EXIT_FOUR]);
                    }
                }
            }
            break;
        case 5:
            sprintf(buffer,
                    "A partial sphere of fire blocks the %s, %s, %s, %s," " and %s exits.\n\r",
                    dirs[EXIT_ONE], dirs[EXIT_TWO], dirs[EXIT_THREE], dirs[EXIT_FOUR],
                    dirs[EXIT_FIVE]);
            break;
        case 6:
            sprintf(buffer,
                    "A sphere of leaping flames encloses this area " "blocking all exits.\n\r");
            break;
        default:
            errorlog("Too many blocked exits with wall of fire.");
        }
        send_to_char(buffer, ch);
    }
}


/* Started 05/17/2000 -nessalin */
void
wind_wall_messages(struct char_data *ch)
{
    int ex;
    char buffer[MAX_STRING_LENGTH];
    int EXIT_ONE, EXIT_TWO, EXIT_THREE, EXIT_FOUR, EXIT_FIVE, EXIT_SIX, wall_counter =
        0, B_NORTH, B_SOUTH, B_EAST, B_WEST, B_UP, B_DOWN;

    EXIT_ONE = -1;
    EXIT_TWO = -1;
    EXIT_THREE = -1;
    EXIT_FOUR = -1;
    EXIT_FIVE = -1;
    EXIT_SIX = -1;
    B_NORTH = 0;
    B_EAST = 0;
    B_SOUTH = 0;
    B_WEST = 0;
    B_UP = 0;
    B_DOWN = 0;

    for (ex = 0; ex < 6; ex++)
        if (ch->in_room->direction[ex]
            && IS_SET(ch->in_room->direction[ex]->exit_info, EX_SPL_WIND_WALL)) {
            if (EXIT_ONE == -1)
                EXIT_ONE = ex;
            else if (EXIT_TWO == -1)
                EXIT_TWO = ex;
            else if (EXIT_THREE == -1)
                EXIT_THREE = ex;
            else if (EXIT_FOUR == -1)
                EXIT_FOUR = ex;
            else if (EXIT_FIVE == -1)
                EXIT_FIVE = ex;
            else if (EXIT_SIX == -1)
                EXIT_SIX = ex;

            if (ex == 0)
                B_NORTH = 1;
            else if (ex == 1)
                B_EAST = 1;
            else if (ex == 2)
                B_SOUTH = 1;
            else if (ex == 3)
                B_WEST = 1;
            else if (ex == 4)
                B_UP = 1;
            else if (ex == 5)
                B_DOWN = 1;
            wall_counter++;
        }

    if (wall_counter) {
        switch (wall_counter) {
        case 1:
            sprintf(buffer, "A wall of gusting winds block the %s exit.\n\r", dirs[EXIT_ONE]);
            break;
        case 2:
            if ((B_NORTH && (B_EAST || B_WEST))
                || (B_SOUTH && (B_EAST || B_WEST)))
                sprintf(buffer,
                        "A gusting, l-shaped wall of wind blocks the %s " "and %s exits.\n\r",
                        dirs[EXIT_ONE], dirs[EXIT_TWO]);
            else if ((B_NORTH && B_SOUTH) || (B_EAST && B_WEST)) {
                sprintf(buffer, "A gusting wall of wind blocks the %s exit.\n\r", dirs[EXIT_ONE]);
                send_to_char(buffer, ch);
                sprintf(buffer, "A gusting wall of wind blocks the %s exit.\n\r", dirs[EXIT_TWO]);
            } else if (B_UP) {
                sprintf(buffer,
                        "A gusting wall of wind blocks the %s exit, "
                        "curving overhead to block the way up.\n\r", dirs[EXIT_ONE]);
            } else {            /* has to be down */

                sprintf(buffer,
                        "Gusting winds blow across the ground here, with"
                        " a wall growing up to the %s blocking the way.\n\r", dirs[EXIT_ONE]);
            }
            break;
        case 3:
            if (!B_UP && !B_DOWN)
                sprintf(buffer,
                        "A gusting, u-shaped wall of wind blocks the %s, " "%s, and %s exits.\n\r",
                        dirs[EXIT_ONE], dirs[EXIT_TWO], dirs[EXIT_THREE]);
            else if (B_UP) {
                if ((B_NORTH && B_SOUTH) || (B_EAST && B_WEST))
                    sprintf(buffer,
                            "An arch of gusting wind stands here, blocking"
                            " the %s, %s and %s exits.\n\r", dirs[EXIT_ONE], dirs[EXIT_TWO],
                            dirs[EXIT_THREE]);
                else
                    sprintf(buffer, "A partial dome of wind blocks the %s, %s and " "%s exits.\n\r",
                            dirs[EXIT_ONE], dirs[EXIT_TWO], dirs[EXIT_THREE]);
            } else {
                sprintf(buffer,
                        "Gusting winds blow across the ground here, with "
                        "walls growing up to the %s and %s, blocking the way." "\n\r",
                        dirs[EXIT_ONE], dirs[EXIT_TWO]);
            }
            break;
        case 4:
            if (!B_UP && !B_DOWN)
                sprintf(buffer,
                        "A gusting wind blows around in a giant circle, "
                        "here, blocking the %s, %s, %s, and %s exits.\n\r", dirs[EXIT_ONE],
                        dirs[EXIT_TWO], dirs[EXIT_THREE], dirs[EXIT_FOUR]);
            else {
                if (B_UP && B_DOWN) {
                    if (B_EAST && B_WEST)
                        sprintf(buffer,
                                "A tunnel of wind runs north-south here, "
                                "blocking the %s, %s, %s, and %s exits.\n\r", dirs[EXIT_ONE],
                                dirs[EXIT_TWO], dirs[EXIT_THREE], dirs[EXIT_FOUR]);
                    else {
                        if (B_NORTH && B_SOUTH)
                            sprintf(buffer,
                                    "A tunnel of wind runs east-west here, "
                                    "blocking the %s, %s, %s, and %s exits.\n\r", dirs[EXIT_ONE],
                                    dirs[EXIT_TWO], dirs[EXIT_THREE], dirs[EXIT_FOUR]);
                        else
                            sprintf(buffer,
                                    "A partial sphere of wind blocks the "
                                    "%s, %s, %s, and %s exits.\n\r", dirs[EXIT_ONE], dirs[EXIT_TWO],
                                    dirs[EXIT_THREE], dirs[EXIT_FOUR]);
                    }
                }
            }
            break;
        case 5:
            sprintf(buffer,
                    "A partial sphere of wind blocks the %s, %s, %s, %s," " and %s exits.\n\r",
                    dirs[EXIT_ONE], dirs[EXIT_TWO], dirs[EXIT_THREE], dirs[EXIT_FOUR],
                    dirs[EXIT_FIVE]);
            break;
        case 6:
            sprintf(buffer, "A sphere of wind encloses this area blocking all " "exits.\n\r");
            break;
        default:
            errorlog("Too many blocked exits with wall of wind.");
        }
        send_to_char(buffer, ch);
    }
}

void
get_skellebain_room_desc(CHAR_DATA * ch, int skell_room, int sector_select)
{

    switch (sector_select) {

        case SECT_INSIDE:
            switch (skell_room) {
                case 0:
                    send_to_char("   Everywhere you step seems to be made of rotted wood, planks hardly held\n\r"
                                 "together and warped either from time or ruinous materials poured over them.\n\r"
                                 "Every step forward seems to make the wood give and almost shatter, making\n\r"
                                 "you hesitant to step forward, or to step backwards. The walls are made of\n\r"
                                 "the same material, and the ceiling, giving you a boxed in feeling of\n\r"
                                 "claustrophobia. The air feels hot and stale, and all the exits seem at once\n\r"
                                 "welcoming and too far to reach.\n\r", ch);
                    break;
                case 1:
                    send_to_char("   The first thing you notice is the blood. The second thing you notice is the\n\r"
                                 "slickness under your boots, that must be the blood that ran under them. The\n\r"
                                 "altar, set on a dias several cords away from you, is the third thing you\n\r"
                                 "take in about this awful room. Then the torch sconces that are human hands,\n\r"
                                 "and the piles of bodies and dismembered limbs scattered carelessly around\n\r"
                                 "the stone chamber. Whatever uses this place seems to have vacated. For now.\n\r"
                                 "Several of the heads eyelessly blink and watch you in undeath, flapping\n\r"
                                 "their mouths open and shut in wordless agony.\n\r", ch);
                    break;
                case 2:
                    send_to_char("   None of the dimensions of this room seem appropriate, though several dozen\n\r"
                                 "tables are stacked up side by side, and all of the occupants of this stone\n\r"
                                 "walled tavern seem content to materialize and dematerialize from one table\n\r"
                                 "to the next without asking. The long bar seems to get longer the closer you\n\r"
                                 "get to it, and the further away you move, the shorter it seems. The windows\n\r"
                                 "show a peculiar landscape surrounding you, though you can't quite make out\n\r"
                                 "the colors or shapes without getting closer. The sound in here is too much.\n\r"
                                 "Every clink of glass seems to be done right in your ear.\n\r", ch);
                    break;
                case 3:
                    send_to_char("   Decadence cannot begin to describe this red-walled chamber. Every inch is\n\r"
                                 "covered in plush, red silk, so that your feet seem to sink in with each\n\r"
                                 "step forwards. Several paintings of posh looking members of society are set\n\r"
                                 "around the bedchamber, and all of their eyes seem to be watching you.\n\r"
                                 "Centered in the room is a gigantic bed, larger than anything you have ever\n\r"
                                 "seen, wrapped in dozens of sheets of pure, red silk. It looks incredibly\n\r"
                                 "inviting. Maybe you should lay down for a while. No one is looking, and\n\r"
                                 "you've had a long day. Such a long...\n\r", ch);
                    break;
                case 4:
                    send_to_char("   Your mind tries to wrap around the room set before you. Stairwells move up,\n\r"
                                 "down, sideways, backwards, and upside-down, and various people seem to walk\n\r"
                                 "up and down each one in unison, none of them falling any which way. The\n\r"
                                 "stairwell in front of you looks normal enough, and seems to descend. Or\n\r"
                                 "does it ascend? The closer you look at it, the harder it is to tell which\n\r"
                                 "way it will take you.\n\r", ch);
                    break;

            }
            break;
        case SECT_CITY:
            switch (skell_room) {
                case 0:
                    send_to_char("   Thousands and thousands of people are packed within this enormous, open aira\n\r"
                                 "market. You notice races of all types, packed shoulder to shoulder, as they\n\r"
                                 "slowly shuffle across the sandy floor. From time to time, you are able to spot\n\r"
                                 "large, red-skinned gith that stand nearly as tall as a half-giant. From a\n\r"
                                 "distance, you briefly glimpse a pair of dwarves wearing only loincloths and\n\r"
                                 "crimson sashes. A muscular elven warrior bumps into you as you notice his\n\r"
                                 "necklace of dozens of teeth, ranging from tiny skeet to the massive fang of a\n\r"
                                 "dujat. The tip of each tooth appears to be darkly stained with old blood. The\n\r"
                                 "sounds of clicks and chittering announce the arrival of a hulking, white\n\r"
                                 "mantis. The long shaggy mane of some long dead beast is draped around its\n\r"
                                 "chitinous neck. As you meander through the crowd, you are overwhelmed with\n\r"
                                 "strange odors and peculiar scents. One smell in particular originates from a\n\r"
                                 "pair of diminutive, mullish children fighting over a single skewer of marilla\n\r"
                                 "glazed, spiced hunks of silt horror tentacle. As the victorious child finishes\n\r"
                                 "the last bite of the kabob, you notice the child's abdomen bulge and stretch.\n\r"
                                 "Clutching his undulating belly, the little mul disappears into the crowd, \n\r"
                                 "screaming in a long forgotten language.\n\r", ch);
                    break;
                case 1:
                    send_to_char("   Under what was once a large, white sandcloth canopy, a shrouded figure draped\n\r"
                                 "in tattered, dark clothing watches potential customers with an ever watchful\n\r"
                                 "eye. The sandcloth tent is now marred with hundreds of splattered blood stains.\n\r"
                                 "Beneath the shade, countless crates, cages, and jugs are stacked to reduce\n\r"
                                 "their exposure to the elements. A string of scalps hangs from a yypr pole. The\n\r"
                                 "predominant scalp of a crimson-maned elf retains a set of previously pierced,\n\r"
                                 "pointed ears. Naked, caged human toddlers reach out grasping at anything that\n\r"
                                 "walks nearby their prisons. A row of shelves stores numerous arms and legs,\n\r"
                                 "neatly stacked into piles loosely based on their race of origin. The partially\n\r"
                                 "butchered torso of a female dwarf hangs from a massive, bone hook, gently\n\r"
                                 "swinging in the breeze. A large, transparent glass cylinder contains the\n\r"
                                 "decapitated head of pale, dark-haired woman. With its mouth and eyes closed,\n\r"
                                 "the head floats in a vat of pale blue liquid. As you tap on the glass, both the\n\r"
                                 "mouth and eyes pop open and the head screams, emitting a muffled, panicked cry.\n\r"
                                 "As she screams, tiny bubbles ranging in color from vivid purple to pale pink\n\r"
                                 "float to the surface of the glass cylinder. Her bimbal-green irises constrict\n\r"
                                 "into quirri-like slits as her eyes focus upon you.\n\r", ch);
                    break;
                case 2:
                    send_to_char("   A lanky elven tribesman watches over a crowd of hundreds as they try to file\n\r"
                                 "into a massive, golden-dyed tent of embroidered cotton. The elf is covered with\n\r"
                                 "numerous scars that often take the shape of swirling patterns and designs. His\n\r"
                                 "hand rests atop the hilt of his bone hawkblade as he nods to various members of\n\r"
                                 "the streaming crowd. The thunderous beat of a drum reverberate from within the\n\r"
                                 "tent as each percussion vibrates your ribs. As you proceed closer to the\n\r"
                                 "entrance, you begin to hear a distant chanting echoing from inside the tent.\n\r"
                                 "Eventually, you are able to discern the sound of dozens of people loudly\n\r"
                                 "singing in every imaginable language. To your amazement, the lyrics to each\n\r"
                                 "song perfectly blend with the others despite the differences in language. You\n\r"
                                 "are enamored by the music and rush forward to hear it more clearly. Despite\n\r"
                                 "continually pressing your way into the crowd, you do not seem to make any\n\r"
                                 "progress at entering the tent.\n\r", ch);
                    break;
                case 3:
                    send_to_char("   Scores of humanoids huddle around a beautiful female elf. She sits,\n\r"
                                 "cross-legged, before a simple linen mat. As you stand on the tips of your toes\n\r"
                                 "to get a better look at her wares, you notice a ivory-hilted, steel scimitar\n\r"
                                 "slung across her back. Sweat drips from her black and crimson armbands as she\n\r"
                                 "gestures to items as members of the crowd bark out bids. As you watch the\n\r"
                                 "auction, you glimpse at the wares covering the mat. An enormous black silt\n\r"
                                 "pearl, the size of a dwarf's head, keeps the mat from moving in the breeze. A\n\r"
                                 "stack of ancient bottles each contain perfectly aged horta, ginka, ocotillo, \n\r"
                                 "vordak, or jallal wine. An unusually detailed deck of kruth cards are displayed\n\r"
                                 "evenly across the mat. A bowl filled with silky brown spice sits beside a book\n\r"
                                 "of rolling papers. Suddenly, you spot something with every desirable \n\r"
                                 "characteristic. As it comes to auction, a brown-skinned half-giant beats you to\n\r"
                                 "the first bid. Stricken with panic, you begin to shout out your own bid. More\n\r"
                                 "bids are shouted in languages that you cannot understand. After what seems to\n\r"
                                 "be an eternity, you finally win the item. As you grasp your perfect purchase\n\r"
                                 "between your hands, you are filled with overwhelming sensation of elation. As\n\r"
                                 "the steel scimitar races through the air toward your neck, you suddenly realize\n\r"
                                 "you offered your life as payment..\n\r", ch);
                    break;
                case 4:
                    send_to_char("   The wind picks up as stinging sands bite against your exposed flesh. A\n\r"
                                 "sandstorm suddenly blows across the massive, crowded market. As the storm\n\r"
                                 "intensifies, people begin to run in every direction in search of shelter. You\n\r"
                                 "watch as a grey-shelled mantis tramples over an elderly half-elven woman. An\n\r"
                                 "angry, copper-skinned dwarf knocks you aside before you can make out the tattoo\n\r"
                                 "on his cheek. You look across the crowd trying to remember where you were\n\r"
                                 "headed. As you get a glimpse of something important, you are knocked prone. The\n\r" 
                                 "crowd rushes forward and begins to trample you. Waves and waves of feet, boots,\n\r"
                                 "and claws stomp across your body. You gasp between footsteps are you struggle\n\r"
                                 "with every breath. Suddenly, you are covered by a cool shadow and the crowd is\n\r"
                                 "gone. As you open your eyes, you have time to make out the gigantic foot of a \n\r"
                                 "thlon-headed, silt giant just before you are crushed into a sandy stain.\n\r", ch);
                    break;

            }
            break;
        case SECT_FIELD:
        case SECT_DESERT:
            switch (skell_room) {
                case 0:
                    send_to_char("   Each step forward seems meaninglessly simple and devoid of purpose. Beyond\n\r"
                                 "the horizon, more dunes seem to surround the landscape. None of them seem\n\r"
                                 "taller or smaller than the others. Just dunes, in all directions. Will this\n\r"
                                 "ever end? Maybe in the distance, you see a few holes of darkness in the\n\r"
                                 "ground.\n\r", ch);
                    break;
                case 1:
                    send_to_char("   A break in the endless monotony shows a wide crevice that opens up and\n\r"
                                 "slopes downwards into a ravine. It seems cooler here, and each step forward\n\r"
                                 "brings you further down into the crevice. You do notice that as you walk,\n\r"
                                 "sand begins to trail in behind you, and each step further trails more sand.\n\r"
                                 "Before you know it, a mound of sand seems to have followed you into this\n\r"
                                 "gap, threatening to close in behind you.\n\r", ch);
                    break;
                case 2:
                    send_to_char("   The sand dunes have gotten larger here. Or have they gotten smaller? You\n\r"
                                 "lost track, and can't tell where some dunes have started and the others\n\r"
                                 "ended. The hazy blur at the edge of the horizon marks little mirages,\n\r"
                                 "colors and shapes that promise hope and an end to this infinite desert.\n\r", ch);
                    break;
                case 3:
                    send_to_char("   Reprieve is instantly found beneath the shade of a massive tree that has\n\r"
                                 "sprouted seemingly from nowhere in this sandy hellhole. It protects a muddy\n\r"
                                 "tract of water that bubbles up from the ground in little spurts, inviting\n\r"
                                 "you to taste its murky depths. It looks tepid and hot, but like real water.\n\r", ch);
                    break;
                case 4:
                    send_to_char("   Well, this looks pretty foreboding. You are at the lip of a precipice that\n\r"
                                 "leads down into a pit that smells awful, and looks worse. You cannot see\n\r"
                                 "the bottom of the pit, and it yawns like an open mouth to the world, like\n\r"
                                 "a hungry creature seeking meat to consume. It doesn't look inviting, and\n\r"
                                 "you can't see a clear way to enter it without falling in.\n\r", ch);
                    break;

            }
            break;
        case SECT_HILLS:
        case SECT_MOUNTAIN:
            switch (skell_room) {
                case 0:
                    send_to_char("   The view is stunning from the highest peak in the world. You can see the\n\r"
                                 "whole known from up here, as clear as day. The sky seems so close you could\n\r"
                                 "almost touch it. In fact, little twinkling sparkles slowly descend from it\n\r"
                                 "like feathers in the breeze. They cover every inch of your body and\n\r"
                                 "clothing, shining brightly. Far below, floating in the breeze and looking\n\r"
                                 "not much larger than a speck, verrin hawks bank from left to right in search\n\r"
                                 "of prey. The only source of noise in this blissful paradise is the rush of\n\r"
                                 "the winds. You are the top of the world.\n\r", ch);
                    break;
                case 1:
                    send_to_char("   Here, the ground seems unstable at best. You look down and see that it is in\n\r"
                                 "fact made of many, many poles of varying shapes and sizes, held up by some\n\r"
                                 "sodding little creatures far below. They seem to be hard at work keeping the\n\r"
                                 "poles stable, but as you carefully shift your weight, the poles supporting\n\r"
                                 "your feet topple over. And then the ones next to those. And then the ones\n\r"
                                 "next to those, and so on, with a deafening roar as they start hitting the\n\r"
                                 "ground. Once one lost balance, everything started falling apart, and you are\n\r"
                                 "finding less and less poles to hang onto, until you end up carefully\n\r"
                                 "balancing on one of the few upright poles left. The ground below you is\n\r"
                                 "littered with the poles, and the only way out is to jump down, get pushed\n\r"
                                 "up, or jump from pole to pole, keeping your balance. This would certainly\n\r"
                                 "not be a good time to discover that you have a fear of heights.\n\r", ch);
                    break;
                case 2:
                    send_to_char("   Your vision distorts and everything grows larger and much bigger. Or are you\n\r"
                                 "growing smaller? You find yourself in a small, grassy knoll that encloses\n\r"
                                 "around you like a mouse hole. Tufts of fur and pellets litter the floor, and\n\r"
                                 "suddenly you find yourself eyeing a little nest of straw, grass and more\n\r"
                                 "fur. It looks so cozy that you think you just have to touch it! But\n\r"
                                 "suddenly, rats begin to skitter all over the floor, surrounding you with\n\r"
                                 "their scratching feet, and gnawing, biting teeth.\n\r", ch);
                    break;
                case 3:
                    send_to_char("   Here, the floor is hot, red molten rock. This volcano must be a newborn, and\n\r"
                                 "it's looking really angry. Spewing forth molten hot rocks and flowing rivers\n\r"
                                 "of lava, it annihilates anything in it's way. You find cover behind one of\n\r"
                                 "the burning rocks that planted itself in the ground, and the situation seems\n\r"
                                 "hopeless. Noxious fumes launch forth into the air up and around you, and as\n\r"
                                 "you give a small gasp of surprise, it enters into the depths of your lungs\n\r"
                                 "and obstructs your vision. From the depths of the volcano, a deafening roar\n\r"
                                 "nearly bursts your ears.\n\r", ch);
                    break;
                case 4:
                    send_to_char("   The rocks beneath your feet seem to squish as if they are made of dung\n\r"
                                 "before springing back to their normal shape.  High above your head is a\n\r"
                                 "spire that seems to be made of living, black decaying flesh.  A pungent odor\n\r"
                                 "akin to rotting fruit assaults your nose. The spire twists in upon itself\n\r"
                                 "time and time again seeming to get taller as you watch it.  Red veins \n\r"
                                 "sometimes pulse on its surface and the ground slowly heaves up and down in a\n\r"
                                 "steady rhythm.  As your gaze pans across the rest of the landscape you\n\r"
                                 "realize this spire is just one of thousands, each twisting and twitching in\n\r"
                                 "their own rhythm.\n\r", ch);
                    break;

            }
            break;
        case SECT_SILT:
        case SECT_SHALLOWS:
            switch (skell_room) {
                case 0:
                    send_to_char("   You suddenly find it exceptionally hard to keep your balance. Before you\n\r"
                                 "know it, you are knee deep in ground, and it's bouncing about. Sand and\n\r"
                                 "earth alike moves about just as the silt of a sea, forming rolling waves\n\r"
                                 "that crash onto themselves, hitting and pushing you, and much like the silt,\n\r"
                                 "find their way into your clothes and ears, and push into your nostrils.\n\r"
                                 "Suddenly, bizarre-looking arm-like things sprout out from the depths,\n\r"
                                 "reaching towards you as if to pull you down, and you feel something brushing\n\r"
                                 "against your leg. Every inch of you is covered in dirt as you sink deeper\n\r"
                                 "with each passing minute.\n\r", ch);
                    break;
                case 1:
                    send_to_char("   A sight to behold, the greatest incentive turned into the biggest of fears.\n\r"
                                 "This spiral threatens to pull whatever approaches it into the depths of it\n\r"
                                 "relentless assault. Countless obsidian pieces rattle deafeningly as they\n\r"
                                 "knock into each other, jumping up erratically, with potentially deadly\n\r"
                                 "consequences. This Maelstrom might be enough to buy the whole Known, but it\n\r"
                                 "would more likely suffocate it. Maybe you should pick some on your way out\n\r"
                                 "if you ever find passage.\n\r", ch);
                    break;
                case 2:
                    send_to_char("   As you approach, calm, smooth silts surround you, going deep enough that you\n\r"
                                 "can't feel the bottom with your feet. An amazing scene forms around you.\n\r"
                                 "Pretty silhouettes either bathe in Krath's rays, or rest in the shade of\n\r"
                                 "yypr trees. trunks, which sprout out from the depths. Noticing you, they\n\r"
                                 "beckon lasciviously, standing on their floating rafts and striking alluring\n\r"
                                 "poses while enjoying refreshing-looking, vibrantly colorful beverages from\n\r"
                                 "finely crafted chalices. Even as you approach one, you never seem to get to\n\r"
                                 "them. They seem like a dream that is never quite within reach.\n\r", ch);
                    break;
                case 3:
                    send_to_char("   Around you in every direction is nothing but thick, choking silt.  You can\n\r"
                                 "hardly see your hand in front of your face, the visibility being terribly\n\r"
                                 "poor. Tiny attempts at movement takes great exertion with very little\n\r"
                                 "effect. When you finally attempt to take a breath you find nothing but silt\n\r"
                                 "fills your mouth, causing you to gag, reflexively trying to breathe once\n\r"
                                 "more.  In your panic you have the sense of something flitting past, quickly.\n\r"
                                 "As it passes it snakes around attempting to grasp your leg.\n\r", ch);
                    break;
                case 4:
                    send_to_char("   At first glance, slowly rolling silt sea can be seen in all directions with\n\r"
                                 "no land in sight.  While it is odd that you are not sinking, for some reason\n\r"
                                 "you aren't terribly bothered by the fact.  Suddenly you get a sense that\n\r"
                                 "something or someone is watching you as the silt begins to heavy up with a\n\r"
                                 "whooshing down and then returns down slowly, almost as if it were breathing.\n\r"
                                 "Moments later, four cord across, an eye snaps opens up just in front of your\n\r"
                                 "feet causing you to startle, stepping back.  Mere moments before you fall in\n\r"
                                 "you realize a gaping hole with silt teeth has opened up behind you.\n\r", ch);
                    break;

            }
            break;
        case SECT_AIR:
            switch (skell_room) {
                case 0:
                    send_to_char("   Soft, fluffy clouds surround your head, obstructing your vision, while the\n\r"
                                 "rest of your body seems to be fine. Cool winds and breezes flow around your\n\r"
                                 "body, and your feet are firmly planted on the ground. Shifting your weight,\n\r"
                                 "you feel something sharp in your foot, then more sharp things. Before long,\n\r"
                                 "it feels like your feet are being repeatedly jabbed with daggers, but you\n\r"
                                 "can't see a thing! Things tug at your fingers, crawling over your skin,\n\r"
                                 "gripping to it, but they seem to fall off at the slightest movement. As you\n\r"
                                 "walk around, you feel many squishy things being crushed under your feet,\n\r"
                                 "and occasionally hit something hard, which immediately topples over. You\n\r"
                                 "hear a distant sound from below, sort of like the buzzing of a swarm of\n\r"
                                 "flies.\n\r", ch);
                    break;
                case 1:
                    send_to_char("   You made a wrong turn. This is bad. The walls surround you and begin to cry\n\r"
                                 "blood. This is very bad. You begin to hear a distant wailing sound, or is\n\r"
                                 "it you making the noise? The ground begins to rumble and shake, and you\n\r"
                                 "see several distinct pathways open up and close at once, leading to bright\n\r"
                                 "white light that drowns the senses. The wailing increases in volume, and\n\r"
                                 "the walls are now covered with the slick, red blood that seeps from the\n\r"
                                 "cracks in-between the grey bricks.\n\r", ch);
                    break;
                case 2:
                    send_to_char("   Here, on top of the clouds, sit many silhouettes, playing various games for\n\r"
                                 "stakes so high they must be in the order of thousands of large. The tables\n\r"
                                 "are made out of the clouds themselves, and they made chairs, too. They\n\r"
                                 "don't seem to be really paying attention to the game, though, as they have\n\r"
                                 "lively conversations, drink from milky white chalices, and smoke various\n\r"
                                 "spices from ivory, or perhaps cloud, pipes, although the heavy smoke\n\r"
                                 "doesn't stay for long as the wind gently blows it away. Meanwhile, from\n\r"
                                 "inside neighbouring clouds lascivious chuckling and the sounds of pleasure\n\r"
                                 "can be heard every once in a while, as people move to and from there,\n\r"
                                 "undressing and dressing, respectively. The men at the gambling table beckon\n\r"
                                 "you to join them, offering a pipe, a chair, and lots of ways to keep\n\r"
                                 "yourself busy.\n\r", ch);
                    break;
                case 3:
                    send_to_char("   You are trapped within a wind! How in the Kn1own did you get here? This\n\r"
                                 "wind is full of glittering motes of sand which are blown about, mingling\n\r"
                                 "with Suk-Krath's rays. The sand whips against your features, burning your\n\r"
                                 "eyes and making them water profusely as your skin feels like it is being\n\r"
                                 "flayed from the bone by the miniscule pieces. You feel wet droplets drip\n\r"
                                 "down your brow which could only mean blood. You begin to swoon, your vision\n\r"
                                 "becoming dark and you stagger to the side, only to tumble down, plummeting\n\r"
                                 "out of the wind violently. If only there was something to grab hold of....\n\r", ch);
                    break;
                case 4:
                    send_to_char("   A wagon stands here, suspended in the air by four rotating wheels of cloud,\n\r"
                                 "lazily floating on the breeze. Four airy inix, at the front of the wagon\n\r"
                                 "but doing absolutely nothing to help it along, float along, their cloudy\n\r"
                                 "white shells absorbing and reflecting the rays of the sun. The driver of\n\r"
                                 "the wagon is visible up in his perch, and man dressed in white, with white\n\r"
                                 "hair, and white skin, his entire form seeming to be nothing more than air\n\r"
                                 "and wind trapped into humanoid form. He waves at you as he floats by.\n\r", ch);
                    break;

            }
            break;
        case SECT_LIGHT_FOREST:
        case SECT_HEAVY_FOREST:
            switch (skell_room) {
                case 0:
                    send_to_char("   A myriad of colors assaults you as far as the eye can see, every tree in\n\r"
                                 "this dense wood seeming alive with colors that pulse and change in the span\n\r"
                                 "of every breath taken. What seemed like a red tree only a moment before\n\r"
                                 "shifts to blue, then purple, then green, and then back again, only to change\n\r"
                                 "to yellow a second after! The constantly shifting colors are enough to make\n\r"
                                 "you dizzy, as there seems to be no break in the constant changes, and it\n\r"
                                 "even seems the ground upon which you walk mirrors those very colors, leaving\n\r"
                                 "only what you hope is the true color of the sky above as the only solid\n\r"
                                 "thing to focus on.\n\r", ch);
                    break;
                case 1:
                    send_to_char("   Even the grass here looks to be made of the highest, largest, smoothest\n\r"
                                 "obsidian you've ever seen.  Standing taller than many buildings the massive\n\r"
                                 "trees glints in Suk-Krath's rays but remains solid and unwavering.  Not far\n\r"
                                 "off lies a pool of what appears to be liquid obsidian, lined with smooth,\n\r"
                                 "black stone creatures frozen in a moment in time as they drink from the\n\r"
                                 "glistening black pool.  You feel drawn to the pool, bending over as you cup\n\r"
                                 "your hand and enjoying the cool touch of the liquid stone before taking a\n\r"
                                 "sip.  As the obsidian touches your lips a soft crackling noise buffets your\n\r"
                                 "ears an instant before you notice your own form, clothing and all, turning\n\r"
                                 "to solid, unmoving smooth black stone. \n\r", ch);
                    break;
                case 2:
                    send_to_char("   Clearly within a gigantic room, this forest has sprung from the ground and\n\r"
                                 "encapsulates all of your sense. You know that the sky is kept away, because\n\r"
                                 "all of the furry creatures that dance around your feet and sing quiet songs\n\r"
                                 "from the trees all seem so forlorn and sad. Trees sprout up from the\n\r"
                                 "grassy ground, huge baobabs and thin cunyatis and robust agafaris, and\n\r"
                                 "other trees you are completely baffled by and unfamiliar with. It smells\n\r"
                                 "like fresh tilled earth, and comforting and peaceful here.\n\r", ch);
                    break;
                case 3:
                    send_to_char("   Here, the forest is made up entirely of weird, twisting trees with what look\n\r"
                                 "like terrified faces, gaping holes opened in a scream of terror under a pair\n\r"
                                 "of eyes that beg for mercy and two flaring nostrils. As you approach you can\n\r"
                                 "feel your feet getting heavier, and your skin turns stiff, gaining the\n\r"
                                 "texture of bark. As you move forwards, you can feel your legs being pulled\n\r"
                                 "back by something stuck in the ground and look down to see that your\n\r"
                                 "stiffening legs are growing roots, which are immediately trying to latch\n\r"
                                 "into the maar-colored, burnt ground. A devious chuckle and the sound of a\n\r"
                                 "torch being lit can be heard in the distance.\n\r", ch);
                    break;
                case 4:
                    send_to_char("   As you approach this location, the murmur and noise of activity reaches your\n\r"
                                 "ears. Looking up, you can see many wooden houses built on the trees around\n\r"
                                 "you for several leagues. People of various races move from house to house\n\r"
                                 "through perilous-looking rope bridges, although the inhabitants of this\n\r"
                                 "sanctuary must have gotten used to them a long time ago. Hunters, crafters,\n\r"
                                 "and various travellers take refuge here, possibly from the oppression and\n\r"
                                 "corruption of the big cities. Some of them notice you, and draw their bows,\n\r"
                                 "pointing thorned arrows at you, while a grizzled, old hunter lowers a rope\n\r"
                                 "for you to climb up and introduce yourself.\n\r", ch);
                    break;

            }
            break;
        case SECT_THORNLANDS:
        case SECT_GRASSLANDS:
        case SECT_COTTONFIELD:
            switch (skell_room) {
                case 0:
                    send_to_char("   The putrid, nauseating smell of decaying roses and the coppery smell of\n\r"
                                 "blood assaults your senses, forcing bile into your throat. The room is dank,\n\r"
                                 "stagnant and humid, crawling with a tangled mess of rotting roses, vines and\n\r"
                                 "thorns of various sizes that cover the walls and ceiling. Each thorn drips\n\r"
                                 "with the blood of all those here before you, puddling at the base of the\n\r"
                                 "plants and slowly seeping into the soft, squishy earth. If you try to wind\n\r"
                                 "your way through the maze, your clothing snags so that moving through the\n\r"
                                 "claustrophobic, prickly area is not only prohibitive but painful as the\n\r"
                                 "thorns pierce and brutally scratch your skin. While there are exits\n\r"
                                 "everywhere, each of the ways out is unyieldingly blocked by more of the\n\r"
                                 "twisted, blood soaked spines, forcing you to remain in the center of the\n\r"
                                 "room, unable to move at all.\n\r", ch);
                    break;
                case 1:
                    send_to_char("   A thorned creeper latches onto your leg and drags you into a bramble. Within\n\r"
                                 "the tangle of thorny vines, you see a great deal of buds on each of the\n\r"
                                 "oddly black vines. As you focus on one bud in particular, it begins to swell\n\r"
                                 "in size until it bursts open into a vibrantly hued purple flower, the petals\n\r"
                                 "ringed with tiny teeth made of sharp thorns. As the one bud bursts into\n\r"
                                 "bloom, so do the others, wrapping you up in a mess of flowers. Tiny teeth\n\r"
                                 "dig into your bare flesh, leaving little scrapes and nicks upon it that\n\r"
                                 "tingle strangely and make your unscratched skin crawl. Wait, did you forget\n\r"
                                 "that you were naked?\n\r", ch);
                    break;
                case 2:
                    send_to_char("   It's dark, wet and warm in here. Gloomy, cruelly tight and you can't move\n\r"
                                 "within the aqueous environment. You hear the far-off sound of a heart\n\r"
                                 "beating. Thud-THUD. Thud-THUD. Thud-THUD. Its pace is one of exertion, one\n\r"
                                 "of fear. You try to scream but everything sounds gurgled, echoing, loud\n\r"
                                 "and invasive to your ears. You find yourself so densely compacted that all\n\r"
                                 "you can do is wriggle as if you were bound. Your arms are crossed in front\n\r"
                                 "of your chest, drawn tightly across your body to the point where even\n\r"
                                 "opening your fisted hands is arduous. Your legs are bent impossibly, knees\n\r"
                                 "to your chest and the realization hits you... You are in the fetal position\n\r"
                                 "and entrapped within a you-sized womb.\n\r", ch);
                    break;
                case 3:
                    send_to_char("   You come across a clearing, the normal greenish-brown color of mostly dried\n\r"
                                 "grass tickling across your legs. As you move along, the grass changes colors\n\r"
                                 "into a wide spectrum of hues from every color. But wait... is the grass\n\r"
                                 "changing? Or is it your own sight? Red fills your vision and a pounding\n\r"
                                 "begins in your temple, and you feel moisture dripping from your eyes. You\n\r"
                                 "lift your fingers to your cheeks, wiping away the wetness. Is that blood on\n\r"
                                 "your fingers?\n\r", ch);
                    break;
                case 4:
                    send_to_char("   You find yourself dwarfed by everything around you, as if the whole forest\n\r"
                                 "grew to improbable heights or you shrunk to the size of a mere roach. It\n\r"
                                 "smells of earth and moisture, of plants and life and Suk-Krath's rays slice\n\r"
                                 "through what appear to be blades of pech-grass, though they are much larger,\n\r"
                                 "humungous in fact. Overhead you can see the gigantic leaves of trees falling\n\r"
                                 "from the branches and drifting down to the ground, spinning of their own\n\r"
                                 "accord and coming alarmingly close to you. Details you've never seen before\n\r"
                                 "come into view, the intricate veins of the leaves you'd ordinarily hold in\n\r"
                                 "your hand, hair-like fibers on stalks, the vivid colors of the plant life,\n\r"
                                 "bugs ordinarily small enough to be crushed between your fingers now large\n\r"
                                 "enough for you to ride. Wonder turns to horror when you hear the\n\r"
                                 "unmistakable slithering of a snake behind you and when you whirl around it\n\r"
                                 "towers above you, poised to strike.\n\r", ch);
                    break;

            }
            break;
        case SECT_SALT_FLATS:
            switch (skell_room) {
                case 0:
                    send_to_char("   Tribals of all shapes and sizes are gathered here on a twisting track marked\n\r"
                                 "with wooden poles in the ground. Lightweight wagons pulled by Erdlu zip by,\n\r"
                                 "coming out of nowhere and always barely missing you. The crowd is cheering,\n\r"
                                 "and it's hard to tell who they're mostly cheering for, such that they are\n\r"
                                 "probably just enjoy the show, the pompously decorated wagons and the\n\r"
                                 "merciless pilots that whip the poor creatures until they turn red. Soon,\n\r"
                                 "they actually turn completely red, and the salt does the same, but they\n\r"
                                 "just keep going as the crowd roars.\n\r", ch);
                    break;
                case 1:
                    send_to_char("   Every step you take forwards seems to angle downwards, even though the\n\r"
                                 "street in front of you seems to angle upwards. Your body lurches and feels\n\r"
                                 "an acute sense of vertigo as you move forwards. Those around you populating\n\r"
                                 "the street seem to walk upwards, and downwards, separate of each other. The\n\r"
                                 "offshoots from this intersection are similarly devoid of any sense of up\n\r"
                                 "or down, seeming to go both ways at once.\n\r", ch);
                    break;
                case 2:
                    send_to_char("   Before you lies the entirety of the salt flats, some aspects greatly\n\r"
                                 "exaggerated in size, while others are so small they are miniscule. As you\n\r"
                                 "move along, it is clear that the ground is entirely unsteady, ready to break\n\r"
                                 "away at any moment. A strip of seemingly steady land abruptly stretches\n\r"
                                 "before your feet, gargantuan purple, blue and white salt crystals growing\n\r"
                                 "from the ground along the path. As the crystals grow three times larger than\n\r"
                                 "you, trickles of viscous red blood begin leaking down the edges and pooling\n\r"
                                 "in the sand around each crystal, leaving bloody impressions in geometric\n\r"
                                 "shapes along the path.\n\r", ch);
                    break;
                case 3:
                    send_to_char("   Salt crystals, thousands of them, seem to stand and walk on their own across\n\r"
                                 "the barren Flats, some looking as if they are dancing! Tiny little movements\n\r"
                                 "catch the eye in almost every direction, as if the ground beneath your feet\n\r"
                                 "may sweep you into this glittering salt dance yourself! The Krath's rays\n\r"
                                 "play havoc across the scene, the reflected light bouncing from crystal to\n\r"
                                 "crystal, like a thousand torches flickering on and off all at once.\n\r", ch);
                    break;
                case 4:
                    send_to_char("   Hurtling around, spinning at great speed, salt engulfs you in a spiral of\n\r"
                                 "small crystals, tearing at your skin and flying into your eyes, immediately\n\r"
                                 "embedding themselves in the wound and cutting through it. The longer you\n\r"
                                 "stay here, the deeper the salt cuts, coloring it red. The merciless salt\n\r"
                                 "storm is worse than its fiercest sand counterpart, each wound feeling as if\n\r"
                                 "fire is tearing the tiny cut asunder.\n\r", ch);
                    break;

            }
            break;
        case SECT_RUINS:
            switch (skell_room) {
                case 0:
                    send_to_char("   You find yourself in a stifling, enclosed space, the bone-white walls\n\r"
                                 "covered in swaths of tattered white silk that twist and snap as though the\n\r"
                                 "wind pulls at them. Despite the seeming breeze that moves the cloth, the\n\r"
                                 "air is stale and has an old, rancid scent. A multitude of old, decomposed\n\r"
                                 "bodies of varying sizes lie against the walls, rust-hued cloths wrapped\n\r"
                                 "around each of the cadavers. Abruptly, white then dark fills your vision as\n\r"
                                 "one of the hanging lengths of silk wrap around you and pull you along,\n\r"
                                 "trying to force you down beside the shrouded figures to become one\n\r"
                                 "yourself.\n\r", ch);
                    break;
                case 1:
                    send_to_char("   A shaft of light illuminates this silty crevice beneath a blocky piece of\n\r"
                                 "sandstone, revealing a scene best described as impossible. Through the\n\r"
                                 "narrow opening, you can see bits of debris scattered atop a battered docks.\n\r"
                                 "In the middle of one stack of splintered wooden planks, the scruffy\n\r"
                                 "dockmaster calls out, hawking skimmer pendants and spice sifters to a pile\n\r"
                                 "of corpses. While turning to find an exit, you notice a half dozen baby\n\r"
                                 "silt horrors dancing to a jaunty tune off to the side of the stone's edge.\n\r"
                                 "Two of them are passing around a massive tube of spice. The smoke billows\n\r"
                                 "up under the rock, then forms the shape of a miniature scrab. You blink,\n\r"
                                 "then blink again, but the image of the scrab lifting its glass of ale in a\n\r"
                                 "toast to you just refuses to fade. Suddenly, a green-eyed mul wearing a\n\r"
                                 "tattered yellow silk party gown appears from the mist that wasn't there a\n\r"
                                 "moment ago, and invites you to the dance.\n\r", ch);
                    break;
                case 2:
                    send_to_char("   As you enter, torches cast warm light over ancient drawings and symbols,\n\r"
                                 "displaying strange architecture of impossible geometry, and various figures\n\r"
                                 "with faces that scream of desperation, bodies contorted and tied or nailed\n\r"
                                 "to the pompous shapes of these alien constructions. Soon, the corridor\n\r"
                                 "splits. But it isn't a regular split. Not in two, not in four, but in six\n\r"
                                 "different directions, including the one you came from. There are various\n\r"
                                 "cracks and spaces in the walls to grab onto, but at the end of each\n\r"
                                 "corridor, there is another such split, and so on. As you arrive at each\n\r"
                                 "split, the room turns randomly, such that up might become down, left might\n\r"
                                 "become forwards, or any number of disorienting shifts, which can cause one\n\r"
                                 "to fall down a corridor.\n\r", ch);
                    break;
                case 3:
                    send_to_char("   Merely stepping into this room forces bile into your mouth. Spiky things,\n\r"
                                 "razors, stretching apparatus, mutilating machines, eye pokers, emblazoning\n\r"
                                 "tools, knives, spears, poison vials and splatters of blood line the walls\n\r"
                                 "of this chamber. People, half dead and half alive, in contorted poses, or\n\r"
                                 "with missing limbs, screaming in agony or taking their last breaths, are\n\r"
                                 "hung on the ceiling and spread upon tables in the center of the room.\n\r"
                                 "Several dark silhouettes are gathered around them, and noticing your\n\r"
                                 "arrival, turn and approach you, with a grin, motioning to an empty space on\n\r"
                                 "the wall behind you.\n\r", ch);
                    break;
                case 4:
                    send_to_char("   This street is full of a bustling crowd, shouting hawkers, busy tradesmen,\n\r"
                                 "and the general fare of city life. Several market stalls and tents make the\n\r"
                                 "passage here arduous and difficult. The closer you look, though, you notice\n\r"
                                 "all of the people here are covered in a sheen of black, spiny hair. They\n\r"
                                 "seem to notice you as you walk by, stopping in their daily chores of sales\n\r"
                                 "and excuses, to stare at you. They are all staring at you, and the street\n\r"
                                 "has gone silent, except for the echos of your footfalls.\n\r", ch);
                    break;

            }
            break;
        case SECT_CAVERN:
        case SECT_SOUTHERN_CAVERN:
        case SECT_SEWER:
            switch (skell_room) {
                case 0:
                    send_to_char("   A pleasurable scent suddenly assaults your nose, destroying the pungent air\n\r"
                                 "of the rotting sewer around you. As if betraying the natural flow of decay,\n\r"
                                 "flowers of many colors have sprung up along the slimy walls, colors of\n\r"
                                 "crimson, black, and silver twinkling in the murky light. Their sweet smells,\n\r"
                                 "accompanied by the lush green moss that suddenly encircle your feet, combine\n\r"
                                 "to bring a freshness to every breath you take, as if someone had decided it\n\r"
                                 "was great idea to grow a garden of beauty in the center of death.\n\r", ch);
                    break;
                case 1:
                    send_to_char("   It is immediately obvious that there is something odd happening here. Out of\n\r"
                                 "the corner of your eye, you can just manage to catch flashes of see through\n\r"
                                 "figures dancing and laughing along the stone walls, the echo of their glee\n\r"
                                 "haunting the ears with a sweet softness of tone. It would seem that a tavern\n\r"
                                 "may have once been here, as the smell of ale and brandy linger on the air.\n\r"
                                 "Trees have sprung from the floor, grey leaves lush along the branches,\n\r"
                                 "arching over the ghostly visage of a bar. Music plays somewhere in the\n\r"
                                 "distance, a happy sound of paradise, of making love, and enjoying life,\n\r"
                                 "completely at odds with everything else in the world.\n\r", ch);
                    break;
                case 2:
                    send_to_char("   The large cavern is as dark as caverns should be as you make your way in. As\n\r"
                                 "you make your way in, you notice a multitude of small, waist high walls that\n\r"
                                 "are smothered in mud. You cannot seem to help but reach out and touch the\n\r"
                                 "mud, brushing some from the surface of the wall. Once you do that, the mud\n\r"
                                 "begins to drip down from all the walls in the grandly sized cavern,\n\r"
                                 "revealing a multitude of gold-colored bricks that are as soft to the touch\n\r"
                                 "as a harder clay. The golden bricks fill the room and begin to emit a strong\n\r"
                                 "glow, lighting the walls of this cavern. As you see the vast stretches of\n\r"
                                 "wall in the cavern and the gold that glows, you also see the strung up\n\r"
                                 "corpses hung along the perimeter of the wall and you get the sinking feeling\n\r"
                                 "that you're next as a mad cackle echoes throughout the cavern.\n\r", ch);
                    break;
                case 3:
                    send_to_char("   The walls of this stone chamber seem to close in on you; yet when you reach\n\r"
                                 "out, they become infinitely far away. Claustrophobia and agoraphobia mingle\n\r"
                                 "in your mind, and you can barely breathe. The air is thick and pungent, like\n\r"
                                 "rancid escru cream. Every inhalation reeks of decay. Bloodstains splatter on\n\r"
                                 "the walls and floor; a hand-smudge painting in sepia tones. Your feet drift\n\r"
                                 "over the floor, but catch on mounds of coagulated fat, which and slither and\n\r"
                                 "spread beneath you. An eyeball, blood vessels clotted and crimson, bounces\n\r"
                                 "through the air toward you. In its pupil you see the face of an old woman;\n\r"
                                 "her cheeks sunken in and silver hair tangled over her empty eye sockets. She\n\r"
                                 "opens her toothless mouth - and you hear her scream.\n\r", ch);
                    break;
                case 4:
                    send_to_char("   All around you is a writhing mass of flesh, pressing you from all sides\n\r"
                                 "instilling an extreme sense of claustrophobia.  Normally, you can see\n\r"
                                 "nothing but periodically light that briefly streams in through an opening\n\r"
                                 "before it abruptly goes black once more.  You get the sensation of very\n\r"
                                 "rapid, jarring  movement as direction is changed many times per heartbeat.\n\r"
                                 "As the light streams in once more, you make for the open only to be grabbed\n\r"
                                 "by two massive tentacles. In a brief moment you get a glimpse of waves of\n\r"
                                 "silt and a massive grey-shelled beast before being devoured once more in a\n\r"
                                 "single snap of a maw.\n\r", ch);
                    break;

            }
            break;
        case SECT_ROAD:
            switch (skell_room) {
                case 0:
                    send_to_char("   The road before you seems to have come to life, as it moves right in front\n\r"
                                 "of your eyes! The path sways and shifts, coiling and uncoiling like the body\n\r"
                                 "of a snake, kicking up sand and rock in the process. At times it seems to\n\r"
                                 "rise straight up, as if reaching for the sky, only to fall back to the earth\n\r"
                                 "and plunge deeply into the ground. No matter which way you start, it seems\n\r"
                                 "the road chooses the direction for you, leaving you dizzy and confused as to\n\r"
                                 "how to tame this beast of a trail.\n\r", ch);
                    break;
                case 1:
                    send_to_char("   This well-maintained road is paved with shimmering white rocks, giving off a\n\r"
                                 "clean reflection. The second you get distracted, even by blinking, it\n\r"
                                 "disappears from where you last saw it. You look around and notice that the\n\r"
                                 "road is now a couple of leagues off in another direction. You don't remember\n\r"
                                 "moving so much. Looking back, the shimmering white road is immediately in\n\r"
                                 "front of your feet.  With every attempted step the road blinks, sometimes to\n\r"
                                 "one side, other time to the edge of your visibility, never allowing you any\n\r"
                                 "forward movement.\n\r", ch);
                    break;
                case 2:
                    send_to_char("   As you walk along the road, you catch sight of two figures in the distance,\n\r"
                                 "walking together with their hands clasped. From the distance the soft sound\n\r"
                                 "of sobbing can be heard. As you follow the path, behind the two distant\n\r"
                                 "figures you see more and more shadowy figures join the pair in front of you\n\r"
                                 "and the sounds of sorrow that they begin to emit are heartrending. The sand\n\r"
                                 "at your feet is decidedly moist as you pass over it, and the thought of the\n\r"
                                 "wasted water chokes at your throat until you can't help but give a little\n\r"
                                 "sob yourself. By this time, you have caught up with the other mourners and\n\r"
                                 "join their rank and file, your sobs matching theirs in sadness until you\n\r"
                                 "catch a glimpse of one of their faces - a skeletal cadaver stares back at\n\r"
                                 "you. Your sobs quickly turn to a gut-wrenching terror when you realize that\n\r"
                                 "you will become just another cadaver in their wake.\n\r", ch);
                    break;
                case 3:
                    send_to_char("   In every cardinal directions a long, straight and flat road stretches out\n\r"
                                 "drawing to a point on the horizon.  The road is covered in a fine, red dust\n\r"
                                 "and there are no footprints in any direction, not even the direction from\n\r"
                                 "which you came.  Lining the sides of the road is a tall drop off made of a\n\r"
                                 "shiny black stone, though looking down you cannot see if there is even a\n\r"
                                 "bottom.  As you walk in any direction you find yourself standing again in\n\r"
                                 "the middle of the crossroads, again with no prints in the fine red sands.\n\r", ch);
                    break;
                case 4:
                    send_to_char("   As you walk along, you notice that this is no ordinary paved road. Runes are\n\r"
                                 "inscribed onto the pavement stones it looks huge, spanning for many leagues\n\r"
                                 "in width. However, as soon as you step on it, the pavement underneath you\n\r"
                                 "fades away, revealing the ground underneath, making your feet fall down on\n\r"
                                 "the arid earth. As you go on, you find that you constantly have to raise\n\r"
                                 "your foot to move on, and then the stone simply disappears, such that it is\n\r"
                                 "very hard to advance. Running would only cause one to trip, falling flat on\n\r"
                                 "the road, which would cause the stone to vanish...\n\r", ch);
                    break;

            }
            break;
        case SECT_NILAZ_PLANE:
            switch (skell_room) {
                case 0:
                    send_to_char("   You seem to be in a depression where bloodied arms, legs and various severed\n\r"
                                 "decaying body parts are piled up in tiny mountains around you.  There is no\n\r"
                                 "way to determine how many bodies make up these piles and they seem to be\n\r"
                                 "comprised of all of the known races and even some that look too foreign for\n\r"
                                 "your mind to comprehend.  As you try and climb out of the depression within\n\r"
                                 "the stacks of remains, the pile in front of you grows and severed hands\n\r"
                                 "weakly grasp as your legs. There seems to be unfathomable numbers of rotting\n\r"
                                 "bodies here as far as the eye can see, the piles growing as you try to find\n\r"
                                 "a way past.\n\r", ch);
                    break;
                case 1:
                    send_to_char("   Bits of sinew, sticky with coagulated blood, are braided together with\n\r"
                                 "delicate strips of dried skin to form a semi-circular nest. Padding the\n\r"
                                 "floor are piles of cracked, leathery shells that give off the stench of\n\r"
                                 "rotten eggs, feces, and rancid regurgitated meat. You are in the middle of\n\r"
                                 "the nest, watching dozens of halfling-sized insectoids slither up and down\n\r"
                                 "the walls. Some of them notice you and start to approach. You notice these\n\r"
                                 "three-eyed creatures each have a single fang extending almost a cord down\n\r"
                                 "from the middle of their mouths. The fang drips with venom. You start to\n\r"
                                 "turn away, but stumble. At your feet lies a dwarf, his hands and feet ripped\n\r"
                                 "away from his limbs. His bloodied stumps are blackened and oozing yellow\n\r"
                                 "puss. A sharpened femur, about the size of a human thigh-bone, stakes the\n\r"
                                 "dwarf to the nest. One of the insects hops over to the dwarf and begins\n\r"
                                 "gnawing on his unclothed stomach. His screams are shrill and biting, and you\n\r"
                                 "try to run away. But some unseen force has rendered you unable to move. You\n\r"
                                 "can only stare as the insects hop closer, and closer...\n\r", ch);
                    break;
                case 2:
                    send_to_char("   A cavernous chamber is here, the walls covered in a deep brownish red hue.\n\r"
                                 "In one corner, a halfling-sized nest made of sinew and hair contains a young\n\r"
                                 "creature, stretching six arms up and mewling softly. In one of his hands, he\n\r"
                                 "holds a broken soldier toy. In another, a bloody pointed elf ear. He is\n\r"
                                 "teething on a dull black gem. The rest of his body is hidden by the huge\n\r"
                                 "female walking your way. Neither insect nor humanoid, she possesses features\n\r"
                                 "of both. Her slanted eyes glow a hazy green light and mauve chitin replaces\n\r"
                                 "skin, her body segmented from top to bottom. Her bottom six limbs end in\n\r"
                                 "razor-sharp claws that curve under and forward, like a raptor's talons. The\n\r"
                                 "top four limbs end in hands, each with two fingers and an opposable thumb.\n\r"
                                 "She chitters softly at you, and a stream of foetid pus oozes out from her\n\r"
                                 "nostrils. You start, suddenly - and the female hisses at you, snapping out a\n\r"
                                 "tiny pair of translucent, shimmering wings from behind her neck. She moves\n\r"
                                 "closer, venom dripping from her impossibly long fang. You turn to leave, but\n\r"
                                 "discover the entryway no longer exists.\n\r", ch);
                    break;
                case 3:
                    send_to_char("   A headless man and woman lay entwined in a carnal embrace on a central\n\r"
                                 "obsidian platform; their naked bloodied bodies only barely obscured by the\n\r"
                                 "man's Militia aba and the woman's Legion tabard. The head of a grey-skinned\n\r"
                                 "man watches the coupling from a shelf against one wall, his green eyes stuck\n\r"
                                 "open with wooden splinters through the lids. Opposite his head is his\n\r"
                                 "sobbing mate - the blood-spattered head of a brown-haired elven woman whose\n\r"
                                 "sliced, long pointed ears dangle from trinket-adorned ribbons off the sides\n\r"
                                 "of her face. Pegs made of finger bones jut out from the walls encircling\n\r"
                                 "these shelves, dangling with Templar pendants and dull obsidian gems. A\n\r"
                                 "ceiling painting depicts a blood-painted Jihae and fat-stucco Lirathu.\n\r"
                                 "Silver rings of Dasari, Winrothol, Borsail, Oash, Negean, and Fale hang down\n\r"
                                 "between the moons from silt-horror tentacles, turning slowly like a baby's\n\r"
                                 "mobile from silt-horror tentacles. The beheaded couple slowly rise from the\n\r"
                                 "platform and take a bow, motioning with their pallid arms for you to join\n\r"
                                 "them.\n\r", ch);
                    break;
                case 4:
                    send_to_char("   Darkness glinting with faint, pulsating illumination swirls before and\n\r"
                                 "behind you. You are suspended in the air, your feet finding no purchase but\n\r"
                                 "neither are you falling, or hanging. Your left arm suddenly jerks up and\n\r"
                                 "falls back down to your side. Then your right arm does the same, lingering a\n\r"
                                 "scant moment to wave your hand. Laughter echoes in what you think might be a\n\r"
                                 "chamber, or tunnel, or is it an arena? The insipid cackling of insanity; but\n\r"
                                 "you see nothing through this void of darkness. Your knee is bent; foot kicks\n\r"
                                 "back as though aimed at your own backside. You open your mouth to protest,\n\r"
                                 "and a wave of horror overtakes you as dozens of insect-like creatures crawl\n\r"
                                 "out from between your lips and squiggle up into your nose. Your eyes itch -\n\r"
                                 "something is crawling on them too. The light breaks; a single candle\n\r"
                                 "flickers up ahead. It serves only to show you your reflection in a\n\r"
                                 "meticulously polished obsidian mirror hanging from the opposite wall.\n\r"
                                 "Your arms and legs are severed from your torso, dangling from their joint sockets\n\r"
                                 "with braids of dried sinew. Unseen hands cradle your head. Your vision is\n\r"
                                 "shifted as you are moved back, back, back. Finally you are turned around.\n\r"
                                 "You squint downward - a shelf of some kind. You look ahead - there is an\n\r"
                                 "obsidian platform in the middle of the chamber. The headless body sitting\n\r"
                                 "upon it looks familiar. It is yours.\n\r", ch);
                    break;

            }
            break;
        case SECT_FIRE_PLANE:
            switch (skell_room) {
                case 0:
                    send_to_char("   Completely surrounding you on all sides and just inches from your skin\n\r"
                                 "rotates fire, hotter than anything you have imagined before.  Flame licked\n\r"
                                 "faces of long lost friends and family form and disappear in the fires, never\n\r"
                                 "visible for more than the briefest of moments. A sense of longing to join\n\r"
                                 "the faces in the flames grows from deep within.  As you finally lose what\n\r"
                                 "restraint you had and move into the flames, the fire winks out only to\n\r"
                                 "return, once more, just mere inches from your face.\n\r", ch);
                    break;
                case 1:
                    send_to_char("   As you approach this location, tongues of fire burst out from the ground,\n\r"
                                 "creating a matrix of singeing flame, seemingly blocking you from going in\n\r"
                                 "any direction. Slowly but surely, the cage is getting tighter, bringing the\n\r"
                                 "flames closer and closer to you, until you can almost touch them; but they\n\r"
                                 "never actually stop. Staying inside could be more painful than trying to\n\r"
                                 "brave one side, but maybe if you stand still it will stop before cutting\n\r"
                                 "through you... and then what?\n\r", ch);
                    break;
                case 2:
                    send_to_char("   A continuous stream of flames has erupted from above here, scorching or\n\r"
                                 "evaporating anything that dares brave the singeing fire. Seeming to be\n\r"
                                 "alive, the fire cascades and spreads across the ground, turning it a maar\n\r"
                                 "shade, and radiates its heat a great distance.  Everything that touches it,\n\r"
                                 "regardless of its flammability bursts into flame and is slowly consumed.\n\r", ch);
                    break;
                case 3:
                    send_to_char("   Hundreds, maybe thousands of cradles inhabit this pitch black, windowless\n\r"
                                 "and eerily silent room. They have been set up in long rows that span the\n\r"
                                 "length of the rectangular chamber on both the eastern and western walls.\n\r"
                                 "They rock on their own, not the neat synchronicity of musicians playing the\n\r"
                                 "same tune but the bedlam of riots, the hysteria of the fleeing. Back and\n\r"
                                 "forth they rock, the wood of their construction making no noise whatsoever.\n\r"
                                 "You dare to look inside and find each infant slaughtered where they slumber.\n\r"
                                 "Burned to a crisp and with flies creeping out of their nostrils and open\n\r"
                                 "mouths, each of their dead eyes are wide, frozen in anguish  The center of\n\r"
                                 "the room has been kept free of cradles, a passageway for ease to the\n\r"
                                 "caretaker. There at the northern end of the room is a lone, female figure,\n\r"
                                 "rocking to and fro, soothing an infant that you cannot hear. Compelled to by\n\r"
                                 "some unknown force, you approach her and see that she is made of shadows,\n\r"
                                 "serene and grey and unfeeling. The child in her arms is alive, engulfed in\n\r"
                                 "flames and screaming as it's being scorched alive and she just rocks it,\n\r"
                                 "watching it as it transitions from Suk-Krath's fiery death to the peace of\n\r"
                                 "Drov.\n\r", ch);
                    break;
                case 4:
                    send_to_char("   Before you lies something equally beautiful and potentially dangerous. From\n\r"
                                 "the searing flames, an alluring figure, striking lascivious poses beckons\n\r"
                                 "you to come and satisfy it's burning desire. As you look closer, you notice\n\r"
                                 "that the flames not only lick its body, singing it as the figure yells with\n\r"
                                 "pleasure, but erupt from it, in a show of masochistic elementalism. The\n\r"
                                 "flames burn strongly, but carnal desires burn stronger.\n\r", ch);
                    break;

            }
            break;
        case SECT_WATER_PLANE:
            switch (skell_room) {
                case 0:
                    send_to_char("   The crinkle of dried, dead foliage crunches under your feet as you open the\n\r"
                                 "gate to the garden. Devoid of life, this plot of earth has been desecrated\n\r"
                                 "by something until no patch of green can be seen in any of the leaves or\n\r"
                                 "stems, no swatch of color is visible from the shriveled blooms. In its\n\r"
                                 "center is a gorgeous fountain that oscillates magickally. A nude, willowy\n\r"
                                 "dancer that's been carved out of the purest alabaster twirls, her\n\r"
                                 "thigh-length hair wrapped around her body demurely as she spins, arms\n\r"
                                 "extended and head thrown joyfully back. Water sprouts from her hands,\n\r"
                                 "sprinkling everything near it. You can smell the water in the air, abundant\n\r"
                                 "and fresh, clear and clean, but before a single drop can touch any plant, it\n\r"
                                 "evaporates. You can hear the trickle of the life giving liquid as it drips\n\r"
                                 "back into the fountain's pool under the spiralling maiden but there is no\n\r"
                                 "explanation for why the water isn't reaching the dead plant life.\n\r", ch);
                    break;
                case 1:
                    send_to_char("   You find yourself inside a warbling blue sphere that seems oddly refreshing.\n\r"
                                 "Movement is possible but takes much more effort than you expected.  Speaking\n\r"
                                 "is impossible, and as you try, water fills your mouth and you suddenly\n\r"
                                 "realize that you cannot breathe!  Any frantic motion simply stirs the water\n\r"
                                 "around you and is replaced immediately with more liquid.  You can see, just\n\r"
                                 "beyond your reach it is dry and there is plentiful air but no amount of\n\r"
                                 "effort allows you to get any closer to your salvation!\n\r", ch);
                    break;
                case 2:
                case 3:
                    send_to_char("   A swirling whirlpool of water is suddenly surrounding you and threatening to\n\r"
                                 "pull you into its depths. As you rotate around the outer edges of its churning\r"
                                 "you begin moving closer to the center, the water spinning you around, having\n\r"
                                 "an extremely disorienting effect. Even as you try to pull yourself away from it,\n\r"
                                 "you are merely delaying the fact, as it takes immense speed and strength to fight\n\r"
                                 "both spin and the pull of this deadly whirlpool.\n\r", ch);
                    break;
                case 4:
                    send_to_char("   This deadly whirlwind of destruction swoops up everything in it's path,\n\r"
                                 "flattening constructions and ripping off anything not stuck deep into the\n\r"
                                 "ground. It then showers whatever it catches in a massive quantity of water,\n\r"
                                 "which makes you cringe, because not only is water wasted, but it is used for\n\r"
                                 "pointless destruction. It is not hard to tell that it is of magickal origin,\n\r"
                                 "as there would never be enough water for such an event to take place in the\n\r"
                                 "Known.\n\r", ch);
                    break;

            }
            break;
        case SECT_EARTH_PLANE:
            switch (skell_room) {
                case 0:
                    send_to_char("   The solid earth beneath you gives way, becoming looser and looser until your\n\r"
                                 "feet fall from under you as the earth begins to swallow you whole! You fall,\n\r"
                                 "plummet into the sinkhole with no possibility of escape. The sand around you\n\r"
                                 "spins as the tunnel of sand widens, an endless vortex of golden sand whose\n\r"
                                 "grains sting your skin as you continue to fall. Lodged in the crumbling\n\r"
                                 "walls of this tunnel are disembodied heads, screaming in terror as the wind\n\r"
                                 "and earth batter their sand-scarred faces and causes their hair to whip\n\r"
                                 "chaotically around them. You become lodged into the wall too as a sudden\n\r"
                                 "gust of Whira slams your body against the wall and sand makes you a\n\r"
                                 "permanent addition to that living sculpture, leaving only your head out to\n\r"
                                 "watch the eternal storm.\n\r", ch);
                    break;
                case 1:
                    send_to_char("   Large trunks rise every few cords from the dun colored ground.  They lean\n\r"
                                 "slightly to one side and seem to sway slightly in the breeze.  As you\n\r"
                                 "inspect these very unusual looking trees you find that they have no branches\n\r"
                                 "and no foliage of any type.  Movement through this forest of smooth, trunk\n\r"
                                 "like stalks is difficult due to their tendency to all sway at once causing\n\r"
                                 "you to stumble, nearly losing your footing.  You sense a disturbance in the\n\r"
                                 "forest in front of you, the stalks seeming to move in separate directions as\n\r"
                                 "a fingertip, the size of at least twenty half-giants bears down on you,\n\r"
                                 "dragging along the forest floor, narrowly avoiding the very spot you were\n\r"
                                 "standing.\n\r", ch);
                    break;
                case 2:
                    send_to_char("   Large trunks rise every few cords from the dun colored ground.  They lean\n\r"
                                 "slightly to one side and seem to sway slightly in the breeze.  As you\n\r"
                                 "inspect these very unusual looking trees you find that they have no branches\n\r"
                                 "and no foliage of any type.  Movement through this forest of smooth, trunk\n\r"
                                 "like stalks is difficult due to their tendency to all sway at once causing\n\r"
                                 "you to stumble, nearly losing your footing.  You sense a disturbance in the\n\r"
                                 "forest in front of you, the stalks seeming to move in separate directions as\n\r"
                                 "a fingertip, the size of at least twenty half-giants bears down on you,\n\r"
                                 "dragging along the forest floor, narrowly avoiding the very spot you were\n\r"
                                 "standing.\n\r", ch);
                    break;
                case 3:
                    send_to_char("   All around you, the buildings seem to lean up towards the sky, and down\n\r"
                                 "towards you all at the same time. Their looming presence seems almost\n\r"
                                 "sentient, as if they are judging you and about to pass the sentence for\n\r"
                                 "execution. The windows that riddle their surfaces seem almost eye-like,\n\r"
                                 "while other orifices shift into mouths that grin and sneer. The street\n\r"
                                 "beneath you seems wiggly and warped, making each step worse than the last.\n\r"
                                 "The buildings press against each other as they struggle for dominance, to\n\r"
                                 "shut out the light in the sky into utter darkness.\n\r", ch);
                    break;
                case 4:
                    send_to_char("   When you started into this alleyway, it seemed wide enough for a few people\n\r"
                                 "to walk shoulder-to-shoulder. Each step you take forwards seems to narrow,\n\r"
                                 "and when you look back over your shoulder, it seems like this alleyway was\n\r"
                                 "always this small. The sky above seems like a tiny crack above you, and you\n\r"
                                 "can't seem to breathe. You can only go forward, and maybe side-to-side, as\n\r"
                                 "you notice more alleys open up from this one, just as narrow as the primary.\n\r"
                                 "Any words you utter seem dull, and do not echo in this place.\n\r", ch);
                    break;

            }
            break;
        case SECT_SHADOW_PLANE:
            switch (skell_room) {
                case 0:
                    send_to_char("   The darkness here is absolute.  No manner of light seems to penetrate the\n\r"
                                 "inky blackness surrounding you.  Sounds are muffled but you can hear\n\r"
                                 "something moving just beyond your touch.  A sense of ill ease creeps into\n\r"
                                 "you, feeling like something is stalking you just beyond your senses.\n\r", ch);
                    break;
                case 1:
                    send_to_char("   Here there is nothing, except you.  No sound can be heard and there is inky\n\r"
                                 "blackness in every direction.  You can't make out any source for light, yet\n\r"
                                 "you find you are nevertheless illuminated fully.  Floating in place,\n\r"
                                 "movement seems impossible as a sense of hopelessness creeps through the\n\r"
                                 "recesses of your mind.\n\r", ch);
                    break;
                case 2:
                    send_to_char("   The brightness in this room is blinding. You seem to be surrounded by it,\n\r"
                                 "consumed by it, your clothing and gear gone. You have no perception of\n\r"
                                 "space, everything around you is bathed in that ferocious glow this light\n\r"
                                 "radiates and you can't tell if you're in a room or whether you're in some\n\r"
                                 "sort of suspended animation. Suddenly a drop of liquid falls from above and\n\r"
                                 "lands in front of you and even though you don't see it hit the ground, or\n\r"
                                 "where the ground should be, you hear the hiss of it as it burns. Another\n\r"
                                 "falls, and another and yet another and you watch in quiet horror as more and\n\r"
                                 "more droplets of liquid, quiet fire drip down from that incandescent ceiling\n\r"
                                 "of light, searing a floor you cannot see. The blazing white of the room is\n\r"
                                 "now sporadically pierced by fat droplets of lava that glow a luminous orange\n\r"
                                 "and come closer and closer to you, closing in.\n\r", ch);
                    break;
                case 3:
                case 4:
                    send_to_char("   The cavern around you is so dim that you can just barely make out the walls,\n\r"
                                 "ceiling and floors.  As you fumble, looking for an exit you trip, stumbling\n\r"
                                 "on what appears, in the near darkness, to be a human femur.  Continuing\n\r"
                                 "along your footsteps start to crunch on the rocks of the cavern floor.\n\r"
                                 "Finally approaching the wall you can make out some movement, leaning in to\n\r"
                                 "inspect.  With a sudden realization you notice that all the surfaces of the\n\r"
                                 "walls are covered with writhing tiny insects.  As you attempt to clear the\n\r"
                                 "insects from a section of wall, looking for an exit many of them crawl up\n\r"
                                 "your arms, burrowing under your clothing, taking tiny bites.  The wall in\n\r"
                                 "front of you morphs as the insects eject from the wall, covering you\n\r"
                                 "entirely, taking speck sized bites at you are devoured.\n\r", ch);
                    break;

            }
            break;
        case SECT_AIR_PLANE:
            switch (skell_room) {
                case 0:
                    send_to_char("   Visible in all directions, fluffy clouds can be seen floating everywhere.\n\r"
                                 "As you inspect your immediate surroundings you find that you are inside a\n\r"
                                 "pulsating cloud of every imaginable color, just like the ones in every\n\r"
                                 "direction.  You find movement is as easy as imagining yourself moving\n\r"
                                 "causing you soar along.  Oddly, though there appears to be no end to these\n\r"
                                 "lines of pulsing brightly colored clouds.\n\r", ch);
                    break;
                case 1:
                    send_to_char("   The surface below your feet has a strange texture, crunching with every\n\r"
                                 "step.  There is a cavernous echo with ever sound, repeating for an\n\r"
                                 "impossibly long time.  As barren as the black surface below your feet is,\n\r"
                                 "wind whips past almost unimaginably fast while not even slightly tugging at\n\r"
                                 "your clothing. Strange eyeless, ten-legged creatures with glowing purple\n\r"
                                 "dots on their chitin skitter past your feet seemingly unaware of your\n\r"
                                 "presence.\n\r", ch);
                    break;
                case 2:
                    send_to_char("   You suddenly find yourself falling!  There is nothing around you but red\n\r"
                                 "sky and the fast approaching sands below.  You cringe, preparing for a\n\r"
                                 "crushing death as the wind is whipping past, moments from impact.  As you\n\r"
                                 "impact with the sands, there is a brief intense pain when the ground gives\n\r"
                                 "way and you find yourself falling again, the sands far below but\n\r"
                                 "approaching quickly.\n\r", ch);
                    break;
                case 3:
                case 4:
                    send_to_char("   As far as the eye can see there is nothing but a vast expanse of salt.  The\n\r"
                                 "surface of the visible salt seems to chun slowly, centered always at your\n\r"
                                 "feet causing a slight disorientation.  The center of the brine remains\n\r"
                                 "under your feet independant of your speed or direction.  The sky flashes,\n\r"
                                 "from time to time, to green then blue but returns to its normal rusty hue\n\r"
                                 "when you blink.  Any time spent standing still causes your feet to begin to\n\r"
                                 "disappear down into the churning salts giving you a nagging sense that you\n\r"
                                 "should keep moving.\n\r", ch);
                    break;

            }
            break;
        case SECT_LIGHTNING_PLANE:
            switch (skell_room) {
                case 0:
                    send_to_char("   It is impossible to discern anything about your surroundings as flashes of\n\r"
                                 "bright, unnatural colors fill your eyes with bright yellow, orange, green,\n\r"
                                 "pink, blue and purple. Each flash comes sporadically and pulses a few\n\r"
                                 "times, nearly blinding you and most definitely setting your heart to\n\r"
                                 "pounding rapidly.\n\r", ch);
                    break;
                case 1:
                    send_to_char("   Darkness and surrounds you as the only sound you can hear is the whistling\n\r"
                                 "winds of Whira.  Assaulting your senses for a moment is an intense flash of\n\r"
                                 "light, quickly followed by a deafening boom.  During the brief moment of\n\r"
                                 "illumination you make out a twisting storm whipping in all directions.  The\n\r"
                                 "series of flashes of intense light and concussive booms intensifies in\n\r"
                                 "frequency, causing you to cover your ears instinctively.  An endless\n\r"
                                 "assault of flashes and thunderous booms continue, your hands unable to\n\r"
                                 "muffle the mind numbing noise and your eyelid not darkening the blinding\n\r"
                                 "light in the slightest.\n\r", ch);
                    break;
                case 2:
                case 3:
                case 4:
                    send_to_char("   The desolate, darkened landscape before you is flat and completely devoid\n\r"
                                 "of vegetation of any kind.  The ground seems to be composed of a hard\n\r"
                                 "packed ruddy clay like substance.  Soft glows in the far distance light the\n\r"
                                 "sky for a moment followed many moments later by a long, low rumbling.  As\n\r"
                                 "you inspect the nearby landscape it appears to be pockmarked every few\n\r"
                                 "cords with blackened holes a few inches in width and depth.  Scanning\n\r"
                                 "further out along the landscape you see that there are absolutely countless\n\r"
                                 "holes as far as you can see in this this dim and foreboding environment.\n\r"
                                 "Suddenly, you feel a strange sense of vibration in the air a moment before\n\r"
                                 "blinding light and a thunderous roar assault all of you senses.  After a\n\r"
                                 "few more moments pass you are finally able to make out a new, smoking\n\r"
                                 "pockmark in the ground a cord from where you are standing.  Continuing with\n\r"
                                 "increasing frequency, a series of flashes and booms continue to rock your\n\r"
                                 "senses time and time again.\n\r", ch);
                    break;

            }
            break;
        default:
            switch (skell_room) {
                case 0:
                case 1:
                case 2:
                case 3:
                case 4:
                    send_to_char("   You can see nothing but a bright, pure, white light.  It surrounds you and\n\r"
                                 "permeates you.  You cannot seem to feel anything and movement is impossible\n\r"
                                 "as you are suspended in what feels like physical light. Your mind is\n\r"
                                 "sensorially overloaded and a sense of hopelessness creeps through the\n\r"
                                 "recesses of your mind despite the radiance caressing your body.\n\r", ch);
                    break;

            }
            break;
    }

}

void
get_skellebain_room_name(CHAR_DATA * ch, char * tmp, int skell_room, int sector_select)
{

    switch (sector_select) {
        case SECT_INSIDE:
            switch (skell_room) {
                case 0:
                    strcpy(tmp, "A Room with Rotted Wood");
                    break;
                case 1:
                    strcpy(tmp, "A Sacrificial Chamber");
                    break;
                case 2:
                    strcpy(tmp, "A Stone-Walled Tavern");
                    break;
                case 3:
                    strcpy(tmp, "The Coziest Bedroom");
                    break;
                case 4:
                    strcpy(tmp, "A Room Filled with Stairwells");
                    break;

            }
            break;
        case SECT_CITY:
            switch (skell_room) {
                case 0:
                    strcpy(tmp, "The Center of a Overcrowded, Gigantic Market");
                    break;
                case 1:
                    strcpy(tmp, "Beneath the Flesh Merchant's Canopy");
                    break;
                case 2:
                    strcpy(tmp, "Before a Congested, Golden-dyed Tent");
                    break;
                case 3:
                    strcpy(tmp, "Huddled Around the Relics Dealer");
                    break;
                case 4:
                    strcpy(tmp, "Lost in a Maze of People");
                    break;

            }
            break;
        case SECT_FIELD: 
        case SECT_DESERT:
            switch (skell_room) {
                case 0:
                    strcpy(tmp, "A Series of Endless Dunes");
                    break;
                case 1:
                    strcpy(tmp, "A Gap in the Dunes");
                    break;
                case 2:
                    strcpy(tmp, "The Dunes Roll Onwards");
                    break;
                case 3:
                    strcpy(tmp, "A Shady Oasis");
                    break;
                case 4:
                    strcpy(tmp, "A Dank, Dark Pit Entrance");
                    break;

            }
            break;

        case SECT_HILLS:
        case SECT_MOUNTAIN:
            switch (skell_room) {
                case 0:
                    strcpy(tmp, "On Top of the World");
                    break;
                case 1:
                    strcpy(tmp, "On Top of Many Poles");
                    break;
                case 2:
                    strcpy(tmp, "Within a Tiny Burrow");
                    break;
                case 3:
                    strcpy(tmp, "On the Edge of a Volcano");
                    break;
                case 4:
                    strcpy(tmp, "Beneath a Twisting Spire");
                    break;

            }
            break;

        case SECT_SILT:
        case SECT_SHALLOWS:
            switch (skell_room) {
                case 0:
                    strcpy(tmp, "Sinking in Tumbling Earth");
                    break;
                case 1:
                    strcpy(tmp, "A Maelstrom of Coins");
                    break;
                case 2:
                    strcpy(tmp, "On a Silt Oasis");
                    break;
                case 3:
                    strcpy(tmp, "Deep Under the Silt");
                    break;
                case 4:
                    strcpy(tmp, "Atop the Living Silt");
                    break;
            }
            break;

        case SECT_AIR:
            switch (skell_room) {
                case 0:
                    strcpy(tmp, "A Cool Grey Haze");
                    break;
                case 1:
                    strcpy(tmp, "The Cloud Wagon");
                    break;
                case 2:
                    strcpy(tmp, "The Den in the Clouds");
                    break;
                case 3:
                    strcpy(tmp, "Within a Glittering Gust of Wind");
                    break;
                case 4:
                    strcpy(tmp, "The Cloud Wagon");
                    break;

            }
            break;

        case SECT_LIGHT_FOREST:
        case SECT_HEAVY_FOREST:
            switch (skell_room) {
                case 0:
                    strcpy(tmp, "The Colorful Woods");
                    break;
                case 1:
                    strcpy(tmp, "The Obsidian Vale");
                    break;
                case 2:
                    strcpy(tmp, "The Indoor Forest");
                    break;
                case 3:
                    strcpy(tmp, "The Forest of People");
                    break;
                case 4:
                    strcpy(tmp, "The Treehouse Village");
                    break;

            }
            break;
        case SECT_THORNLANDS:
        case SECT_GRASSLANDS:
        case SECT_COTTONFIELD:
            switch (skell_room) {
                case 0:
                    strcpy(tmp, "The Rose Room");
                    break;
                case 1:
                    strcpy(tmp, "Within a Bramble of Violet-Hued Flowers");
                    break;
                case 2:
                    strcpy(tmp, "The Womb");
                    break;
                case 3:
                    strcpy(tmp, "A Stretch of Multi-Hued Grass");
                    break;
                case 4:
                    strcpy(tmp, "The Tiny Forest");
                    break;

            }
            break;

        case SECT_SALT_FLATS:
            switch (skell_room) {
                case 0:
                    strcpy(tmp, "A Tribal Racing Competition");
                    break;
                case 1:
                    strcpy(tmp, "A Wide, Sloping Intersection");
                    break;
                case 2:
                    strcpy(tmp, "A Bloody Salt Crystal Lined Path");
                    break;
                case 3:
                    strcpy(tmp, "The Moving Salts");
                    break;
                case 4:
                    strcpy(tmp, "Within a Salt Demon");
                    break;

            }
            break;

        case SECT_RUINS:
            switch (skell_room) {
                case 0:
                    strcpy(tmp, "The Shroud Room");
                    break;
                case 1:
                    strcpy(tmp, "Under a Rock");
                    break;
                case 2:
                    strcpy(tmp, "Within the Maze");
                    break;
                case 3:
                    strcpy(tmp, "The Chamber of Torture");
                    break;
                case 4:
                    strcpy(tmp, "A Crowded Street");
                    break;

            }
            break;

        case SECT_CAVERN:
        case SECT_SOUTHERN_CAVERN:
        case SECT_SEWER:
            switch (skell_room) {
                case 0:
                    strcpy(tmp, "The Sweet Smell of Decay");
                    break;
                case 1:
                    strcpy(tmp, "The Cave of Wonders");
                    break;
                case 2:
                    strcpy(tmp, "A Golden Cavern");
                    break;
                case 3:
                    strcpy(tmp, "A Dank, Vacuous Chamber");
                    break;
                case 4:
                    strcpy(tmp, "Inside the Beast");
                    break;

            }
            break;

        case SECT_ROAD:
            switch (skell_room) {
                case 0:
                    strcpy(tmp, "The Snaking Trail");
                    break;
                case 1:
                    strcpy(tmp, "The Shifting Road");
                    break;
                case 2:
                    strcpy(tmp, "The Trail of Tears");
                    break;
                case 3:
                    strcpy(tmp, "The Middle of a Crossroads");
                    break;
                case 4:
                    strcpy(tmp, "The Most Tiring Road");
                    break;

            }
            break;

        case SECT_NILAZ_PLANE:
            switch (skell_room) {
                case 0:
                    strcpy(tmp, "A Pile of Remains");
                    break;
                case 1:
                    strcpy(tmp, "A Musty Foetid Nest");
                    break;
                case 2:
                    strcpy(tmp, "Queen's Room");
                    break;
                case 3:
                    strcpy(tmp, "Trophy Room");
                    break;
                case 4:
                    strcpy(tmp, "The Worm Hole");
                    break;

            }
            break;
        case SECT_FIRE_PLANE:
            switch (skell_room) {
                case 0:
                    strcpy(tmp, "Within a Column of Fire");
                    break;
                case 1:
                    strcpy(tmp, "In a Cage of Fire");
                    break;
                case 2:
                    strcpy(tmp, "Under a Shower of Fire");
                    break;
                case 3:
                    strcpy(tmp, "The Nursery");
                    break;
                case 4:
                    strcpy(tmp, "The Burning Desire");
                    break;

            }
            break;
        case SECT_WATER_PLANE:            
            switch (skell_room) {
                case 0:
                    strcpy(tmp, "The Parched Garden");
                    break;
                case 1:
                    strcpy(tmp, "In a Blue Sphere");
                    break;
                case 2:
                case 3:
                    strcpy(tmp, "In the Whirlpool");
                    break;
                case 4:
                    strcpy(tmp, "The Hurricane");
                    break;

            }
            break;
        case SECT_EARTH_PLANE:
            switch (skell_room) {
                case 0:
                    strcpy(tmp, "The Sinkhole");
                    break;
                case 1:
                    strcpy(tmp, "Walking in a Forest of Hair");
                    break;
                case 2:
                    strcpy(tmp, "Undulating, Rolling Hills");
                    break;
                case 3:
                    strcpy(tmp, "Surrounded by Leaning Buildings");
                    break;
                case 4:
                    strcpy(tmp, "An Incredibly Narrow Alleyway");
                    break;

            }
            break;
        case SECT_SHADOW_PLANE:
            switch (skell_room) {
                case 0:
                    strcpy(tmp, "Darkness");
                    break;
                case 1:
                    strcpy(tmp, "The Nothing");
                    break;
                case 2:
                    strcpy(tmp, "Quiet Fire");
                    break;
                case 3:
                case 4:
                    strcpy(tmp, "A Writhing Cavern");
                    break;

            }
            break;
        case SECT_AIR_PLANE:
            switch (skell_room) {
                case 0:
                    strcpy(tmp, "In a Pulsating, Multi-Colored Cloud");
                    break;
                case 1:
                    strcpy(tmp, "On the Black Moon");
                    break;
                case 2:
                    strcpy(tmp, "In the Air");
                    break;
                case 3:
                case 4:
                    strcpy(tmp, "A Rotating Whirlpool of Salt");
                    break;

            }
            break;
        case SECT_LIGHTNING_PLANE:
            switch (skell_room) {
                case 0:
                    strcpy(tmp, "Flashing Fluorescent Colors");
                    break;
                case 1:
                    strcpy(tmp, "Amid a Dark, Deafening Storm");
                    break;
                case 2:
                case 3:
                case 4:
                    strcpy(tmp, "On a Lightning-blasted Plains");
                    break;

            }
            break;

        default:
            switch (skell_room) {
                case 0:
                case 2:
                case 3:
                case 4:
                    strcpy(tmp, "In a Pure Light"); 
                    break;

            }
            break;

    }

}   

void
cmd_look(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{                               /* Cmd_Look */
    char buffer[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH], buf2[MAX_STRING_LENGTH], buf3[MAX_STRING_LENGTH];
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char arg3[MAX_STRING_LENGTH];
    char pos_pers[MAX_STRING_LENGTH];
    char pos_item[MAX_STRING_LENGTH];
    char doorname[MAX_STRING_LENGTH];
    /* needed for command emotives */
    char to_char[MAX_STRING_LENGTH];
    char to_room[MAX_STRING_LENGTH];
    char args[MAX_STRING_LENGTH];
    char preHow[MAX_STRING_LENGTH];
    char postHow[MAX_STRING_LENGTH];
    /* end command emotives */
    /*  char error_log[MAX_STRING_LENGTH]; */

    char exits[256];
    int distance, dir, ex;
    int keyword_no, eq_pos;
    int ch_height, tmp_height;

    int i = 0;
    int j, bits, vis, num_objs;

    int skell_room;             /* Room to diasplay on Skellebain */
    skell_room = number(0,4);   /* Select room on each look while on Skellebain */

    int display_skell;
    if (affected_by_spell(ch, POISON_SKELLEBAIN)) {
        display_skell = number(0, 1);  /* Displays skell name 50%*/
    } else if (affected_by_spell(ch, POISON_SKELLEBAIN_2)) {
        display_skell = number(0, 3);  /* Displays skell name 25%*/
    } else if (affected_by_spell(ch, POISON_SKELLEBAIN_3)) {
        display_skell = number(0, 6);  /* Displays skell name 15%*/
    }

    int sector_select;

    if (number(1, 100) < 25) {         // 25% of the time it will show a rand sector
        sector_select = number(0, 26); // Choose random sector to display
    } else {
        sector_select = ch->in_room->sector_type;
    }

    sh_int dmin = 1;            /* For look <dir> - Kel */
    bool cansee;                /* Added for use with cvict - Kelvik */
    bool found;
    ROOM_DATA *tar_room, *test_room, *was_in;
    OBJ_DATA *tmp_object, *found_object;
    CHAR_DATA *tmp_char;
    CHAR_DATA *cvict;           /* Added for use with cansee - Kelvik */
    OBJ_DATA *tmpob;
    char *tmp_desc = NULL, *tmp_desc2 = NULL;
    char tmp[256];
    /*char buf1[MAX_STRING_LENGTH];*/

    static const char * const keywords[] = {
        /* THese are used for the switch (keyword_no) you'll find below */
        "north",                /* 0  *//* Directions */
        "east",                 /* 1  *//* Directions */
        "south",                /* 2  *//* Directions */
        "west",                 /* 3  *//* Directions */
        "up",                   /* 4  *//* Directions */
        "down",                 /* 5  *//* Directions */
        "in",                   /* 6  *//* Containers, mostly */
        "at",                   /* 7  *//* same as look, really */
        "",                     /* 8  *//* Look at '' case */
        "out",                  /* 9  *//* Within wagons */
        "outside",              /* 10 *//* Within wagons */
        "room",                 /* 11 *//* Room descriptions when in brief mode */
        "on",                   /* 12 *//* Furniture */
        "\n"
    };

    /* Players without links don't get sent information, desc means descriptor */
    if (!ch || !ch->desc)
        return;

    if (!ch->in_room)
        return;

    /* Remove blurred vision affect that was placed by Scrub Fever disease */
    /*if (TRUE == get_char_extra_desc_value(ch, "[BLURRED_VISION]", buf1, MAX_STRING_LENGTH)) {
        if ((!strcmp(buf1, "Scrub Fever")) && (!affected_by_disease(ch, DISEASE_SCRUB_FEVER))) {
            rem_char_extra_desc_value(ch, "[BLURRED_VISION]");
            send_to_char("Your vision returns to normal.\n\r", ch);
        }
    }*/

    if (GET_POS(ch) < POSITION_SLEEPING) {
        if (!IS_AFFECTED(ch, CHAR_AFF_SUBDUED))
            cprintf(ch, "You can't see anything but stars!\n\r");
    } else if (GET_POS(ch) == POSITION_SLEEPING) {
        if (!IS_AFFECTED(ch, CHAR_AFF_SUBDUED))
            cprintf(ch, "You can't see anything; you're sleeping!\n\r");
    } else if (IS_AFFECTED(ch, CHAR_AFF_BLIND)) {
        if (!IS_AFFECTED(ch, CHAR_AFF_SUBDUED))
            cprintf(ch, "You can't see a thing; you're blinded!\n\r");
    } else if (affected_by_spell(ch, SPELL_ELEMENTAL_FOG) && has_sight_spells_resident(ch)) {
        if (!IS_AFFECTED(ch, CHAR_AFF_SUBDUED))
            cprintf(ch, "You can't see a thing; A fog covers your eyes!\n\r");
    } else if (IS_DARK(ch->in_room) && !IS_AFFECTED(ch, CHAR_AFF_INFRAVISION) && !IS_IMMORTAL(ch)
               && (GET_RACE(ch) != RACE_HALFLING) && (GET_RACE(ch) != RACE_MANTIS)
               && (GET_RACE(ch) != RACE_GHOUL) && (GET_RACE(ch) != RACE_VAMPIRE)) {
        if (!IS_AFFECTED(ch, CHAR_AFF_SUBDUED)) {
            cprintf(ch, "Darkness\n\r");
            if (!IS_SET(ch->specials.brief, BRIEF_ROOM))
                cprintf(ch,
                        "   Total darkness surrounds you, preventing you from seeing anything\n\r"
                        "at all.  You have trouble telling where to put your feet when you walk.\n\r");
        }
    } else if (!IS_IMMORTAL(ch) && ((vis = visibility(ch)) < 0)) {
        if (!IS_AFFECTED(ch, CHAR_AFF_SUBDUED))
            cprintf(ch, "You can't see a thing; sand swirls about you!\n\r");
    } else {

        extract_emote_arguments(argument, preHow, sizeof(preHow), args, sizeof(args), postHow,
                                sizeof(postHow));
        argument = one_argument(args, arg1, sizeof(arg1));
        keyword_no = search_block(arg1, keywords, FALSE);

        if ((keyword_no == -1) && *arg1) {
            keyword_no = 7;
            strcpy(arg2, arg1);
            strcpy(arg3, argument);
            /* Let arg2 become the target object (arg1) */
        } else {
            argument = one_argument(argument, arg2, sizeof(arg2));
            strcpy(arg3, argument);
        }

        /* look tables */
        if (!strcmp(arg1, "tables")) {
            if (*preHow || *postHow) {
                sprintf(to_char, "you look at the tables");
                sprintf(to_room, "@ looks around the room");

                // if the emote fails, don't scan the tables
                if (!send_to_char_and_room_parsed
                    (ch, preHow, postHow, to_char, to_room, to_room, MONITOR_OTHER)) {
                    return;
                }
            }
            show_tables(ch);
            return;
        }

        found = FALSE;
        tmp_object = 0;
        tmp_char = 0;
        tmp_desc = 0;

        /* The first 6 keyword_nos are directions, so this is n, e, s, w, u, d */
        if ((keyword_no < 0) || (keyword_no > 5)) {
            if (IS_DARK(ch->in_room) && !IS_IMMORTAL(ch) && (GET_RACE(ch) != RACE_GHOUL)
                && (GET_RACE(ch) != RACE_VAMPIRE) && (GET_RACE(ch) != RACE_HALFLING)
                && (GET_RACE(ch) != RACE_MANTIS) && !IS_AFFECTED(ch, CHAR_AFF_INFRAVISION)) {
                cprintf(ch, "It's too dark to see anything.\n\r");
                return;
            }
        }

        switch (keyword_no) {
            /* look <dir> */
        case 0:                /* n */
        case 1:                /* e */
        case 2:                /* s */
        case 3:                /* w */
        case 4:                /* u */
        case 5:                /* d */
            if (*preHow || *postHow) {
                sprintf(to_char, "you look %s", dir_name[keyword_no]);
                sprintf(to_room, "@ looks %s", dir_name[keyword_no]);

                // if the emote fails, don't scan the tables
                if (!send_to_char_and_room_parsed
                    (ch, preHow, postHow, to_char, to_room, to_room, MONITOR_OTHER)) {
                    return;
                }
            }
            if (EXIT(ch, keyword_no)) {
                if (is_secret(EXIT(ch, keyword_no))
                    && IS_SET(EXIT(ch, keyword_no)->exit_info, EX_CLOSED))
                    cprintf(ch, "You see nothing special.\n\r");
                else {          /* 1 */
                    /* Added door_name by Morg 3/18/98 */
                    char door_name[MAX_STRING_LENGTH];
                    ROOM_DATA *to_room = EXIT(ch, keyword_no)->to_room;
                    char scan_dir_name[MAX_STRING_LENGTH];
                    char dir_name_store[MAX_STRING_LENGTH];

                    if (EXIT(ch, keyword_no)->general_description
                        && EXIT(ch, keyword_no)->general_description[0] != '\0'
                        && EXIT(ch, keyword_no)->general_description[0] != '\n')
                        cprintf(ch, "%s", EXIT(ch, keyword_no)->general_description);
                    else {      /* 2 */
                        /*
                         * new code for showing default what's in a direction
                         * Ex: A door to the south leads to the Red Sun Commons.
                         * Ex: To the south is Meleth Circle.
                         * Morg 3/18/98
                         */
                        if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_SPL_SAND_WALL)) {
                            strcpy(door_name, "a giant wall of sand");
                        }
                        if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_SPL_FIRE_WALL)) {
                            strcpy(door_name, "a giant wall of fire");
                        }
                        if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_SPL_WIND_WALL)) {
                            strcpy(door_name, "a gusting wind");
                        }
                        if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_SPL_BLADE_BARRIER)) {
                            strcpy(door_name, "a mass of whirling blades");
                        }
                        if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_THORN)
                            || IS_SET(EXIT(ch, keyword_no)->exit_info, EX_SPL_THORN_WALL)) {
                            strcpy(door_name, "many thorn plants");
                        } else if (EXIT(ch, keyword_no)->keyword) {
                            strcpy(buf, first_name(EXIT(ch, keyword_no)->keyword, buf2));
                            strcpy(door_name, LOWER(buf[0]) == 'a' || LOWER(buf[0]) == 'e'
                                   || LOWER(buf[0]) == 'i' || LOWER(buf[0]) == 'o'
                                   || LOWER(buf[0]) == 'u' ? "an " : "a ");
                            strcat(door_name, buf);

                        } else {
                            door_name[0] = '\0';
                        }

                        strcpy(scan_dir_name, scan_dir[keyword_no]);
                        strcpy(dir_name_store, dir_name[keyword_no]);
                        if ((!IS_SET(EXIT(ch, keyword_no)->exit_info, EX_CLOSED)
                             || IS_SET(EXIT(ch, keyword_no)->exit_info, EX_TRANSPARENT))
                            && !IS_SET(EXIT(ch, keyword_no)->exit_info, EX_SPL_SAND_WALL)) {
                            switch (number(0, 2)) {
                            case 0:
                                if  (affected_by_spell(ch, POISON_SKELLEBAIN) && display_skell > 0) {
                                    get_skellebain_room_name(ch, tmp, skell_room, sector_select);
                                    cprintf(ch, "%s%s%s%s %s %s.\n\r",
                                        door_name[0] != '\0' ? "Through " : "",
                                        door_name[0] != '\0' ? door_name : "",
                                        door_name[0] != '\0' ? " " : "",
                                        door_name[0] != '\0' ? scan_dir_name : CAP(scan_dir_name),
                                        IS_DARK(to_room) ? "is" : is_are(to_room->name),
                                        IS_DARK(to_room) ? "darkness" :
                                        tmp);
                                } else if (affected_by_spell(ch, POISON_SKELLEBAIN_2) && display_skell > 0) {
                                    get_skellebain_room_name(ch, tmp, skell_room, sector_select);
                                    cprintf(ch, "%s%s%s%s %s %s.\n\r",
                                        door_name[0] != '\0' ? "Through " : "",
                                        door_name[0] != '\0' ? door_name : "",
                                        door_name[0] != '\0' ? " " : "",
                                        door_name[0] != '\0' ? scan_dir_name : CAP(scan_dir_name),
                                        IS_DARK(to_room) ? "is" : is_are(to_room->name),
                                        IS_DARK(to_room) ? "darkness" :
                                        tmp);       
                                } else if (affected_by_spell(ch, POISON_SKELLEBAIN_3) && display_skell > 0) {
                                    get_skellebain_room_name(ch, tmp, skell_room, sector_select);
                                    cprintf(ch, "%s%s%s%s %s %s.\n\r",
                                        door_name[0] != '\0' ? "Through " : "",
                                        door_name[0] != '\0' ? door_name : "",
                                        door_name[0] != '\0' ? " " : "",
                                        door_name[0] != '\0' ? scan_dir_name : CAP(scan_dir_name),
                                        IS_DARK(to_room) ? "is" : is_are(to_room->name),
                                        IS_DARK(to_room) ? "darkness" :
                                        tmp);      
                                } else {
                                    cprintf(ch, "%s%s%s%s %s %s.\n\r",
                                        door_name[0] != '\0' ? "Through " : "",
                                        door_name[0] != '\0' ? door_name : "",
                                        door_name[0] != '\0' ? " " : "",
                                        door_name[0] != '\0' ? scan_dir_name : CAP(scan_dir_name),
                                        IS_DARK(to_room) ? "is" : is_are(to_room->name),
                                        IS_DARK(to_room) ? "darkness" :
                                        lower_article_first(to_room->name));

                                }
                                break;
                            case 1:
                                if (affected_by_spell(ch, POISON_SKELLEBAIN) && display_skell > 0) {
                                    get_skellebain_room_name(ch, tmp, skell_room, sector_select);
                                    cprintf(ch, "%s%s%s%s %s.\n\r",
                                        door_name[0] != '\0' ? CAP(door_name) : "",
                                        door_name[0] != '\0' ? " " : "",
                                        door_name[0] != '\0' ? scan_dir_name : CAP(scan_dir_name),
                                        door_name[0] != '\0' ? " leads to" : " is",
                                        IS_DARK(to_room) ? "darkness" :
                                        tmp);
                                } else if (affected_by_spell(ch, POISON_SKELLEBAIN_2) && display_skell > 0) {
                                    get_skellebain_room_name(ch, tmp, skell_room, sector_select);
                                    cprintf(ch, "%s%s%s%s %s.\n\r",
                                        door_name[0] != '\0' ? CAP(door_name) : "",
                                        door_name[0] != '\0' ? " " : "",
                                        door_name[0] != '\0' ? scan_dir_name : CAP(scan_dir_name),
                                        door_name[0] != '\0' ? " leads to" : " is",
                                        IS_DARK(to_room) ? "darkness" :
                                        tmp);
                                } else if (affected_by_spell(ch, POISON_SKELLEBAIN_3) && display_skell > 0) {
                                    get_skellebain_room_name(ch, tmp, skell_room, sector_select);
                                    cprintf(ch, "%s%s%s%s %s.\n\r",
                                        door_name[0] != '\0' ? CAP(door_name) : "",
                                        door_name[0] != '\0' ? " " : "",
                                        door_name[0] != '\0' ? scan_dir_name : CAP(scan_dir_name),
                                        door_name[0] != '\0' ? " leads to" : " is",
                                        IS_DARK(to_room) ? "darkness" :
                                        tmp);      
                                } else {
                                    cprintf(ch, "%s%s%s%s %s.\n\r",
                                        door_name[0] != '\0' ? CAP(door_name) : "",
                                        door_name[0] != '\0' ? " " : "",
                                        door_name[0] != '\0' ? scan_dir_name : CAP(scan_dir_name),
                                        door_name[0] != '\0' ? " leads to" : " is",
                                        IS_DARK(to_room) ? "darkness" :
                                        lower_article_first(to_room->name));
                                }
                                break;
                            case 2:
                            default:
                                if (affected_by_spell(ch, POISON_SKELLEBAIN) && display_skell > 0) {
                                    get_skellebain_room_name(ch, tmp, skell_room, sector_select);
                                    cprintf(ch, "%s%s%s%s %s %s.\n\r", CAP(dir_name_store),
                                        door_name[0] != '\0' ? ", through " : keyword_no >=
                                        DIR_UP ? " from here" : " of here",
                                        door_name[0] != '\0' ? door_name : "",
                                        door_name[0] != '\0' ? "," : "",
                                        IS_DARK(to_room) ? "is" : is_are(to_room->name),
                                        IS_DARK(to_room) ? "darkness" :
                                        tmp);
                                } else if (affected_by_spell(ch, POISON_SKELLEBAIN_2) && display_skell > 0) {
                                    get_skellebain_room_name(ch, tmp, skell_room, sector_select); 
                                    cprintf(ch, "%s%s%s%s %s %s.\n\r", CAP(dir_name_store),
                                        door_name[0] != '\0' ? ", through " : keyword_no >=
                                        DIR_UP ? " from here" : " of here",
                                        door_name[0] != '\0' ? door_name : "",
                                        door_name[0] != '\0' ? "," : "",
                                        IS_DARK(to_room) ? "is" : is_are(to_room->name),
                                        IS_DARK(to_room) ? "darkness" :
                                        tmp);
                                } else if (affected_by_spell(ch, POISON_SKELLEBAIN_3) && display_skell > 0) {
                                    get_skellebain_room_name(ch, tmp, skell_room, sector_select);
                                    cprintf(ch, "%s%s%s%s %s %s.\n\r", CAP(dir_name_store),
                                        door_name[0] != '\0' ? ", through " : keyword_no >=
                                        DIR_UP ? " from here" : " of here",
                                        door_name[0] != '\0' ? door_name : "",
                                        door_name[0] != '\0' ? "," : "",
                                        IS_DARK(to_room) ? "is" : is_are(to_room->name),
                                        IS_DARK(to_room) ? "darkness" :
                                        tmp);
                                } else {
                                    cprintf(ch, "%s%s%s%s %s %s.\n\r", CAP(dir_name_store),
                                        door_name[0] != '\0' ? ", through " : keyword_no >=
                                        DIR_UP ? " from here" : " of here",
                                        door_name[0] != '\0' ? door_name : "",
                                        door_name[0] != '\0' ? "," : "",
                                        IS_DARK(to_room) ? "is" : is_are(to_room->name),
                                        IS_DARK(to_room) ? "darkness" :
                                        lower_article_first(to_room->name));
                                }
                                break;
                            }
                        }
                    }

                    if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_SPL_SAND_WALL)) {
                        cprintf(ch, "A giant wall of sand blocks the way.\n\r");
                    }
                    if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_SPL_FIRE_WALL)) {
                        cprintf(ch, "A giant wall of fire blocks the way.\n\r");
                    }
                    if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_THORN)
                        || IS_SET(EXIT(ch, keyword_no)->exit_info, EX_SPL_THORN_WALL)) {
                        cprintf(ch, "Thorny plants block the way.\n\r");
                    }
                    if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_SPL_WIND_WALL)) {
                        cprintf(ch, "Gusting winds block the way.\n\r");
                    }
                    if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_SPL_BLADE_BARRIER)) {
                        cprintf(ch, "A whirling mass of blades block the way.\n\r");
                    }

                    else if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_CLOSED)
                             && !IS_SET(EXIT(ch, keyword_no)->exit_info, EX_SECRET)
                             && EXIT(ch, keyword_no)->keyword) {
                        strcpy(doorname, first_name(EXIT(ch, keyword_no)->keyword, buf2));
                        if (doorname[strlen(doorname) - 1] == 's')
                            strcpy(buf3, "are");
                        else
                            strcpy(buf3, "is");
                        cprintf(ch, "The %s %s closed.\n\r",
                                first_name(EXIT(ch, keyword_no)->keyword, buf2), buf3);
                    } else {
                        if (IS_SET(EXIT(ch, keyword_no)->exit_info, EX_ISDOOR)
                            && !IS_SET(EXIT(ch, keyword_no)->exit_info, EX_SECRET)
                            && EXIT(ch, keyword_no)->keyword) {
                            strcpy(doorname, first_name(EXIT(ch, keyword_no)->keyword, buf2));
                            if (doorname[strlen(doorname) - 1] == 's')
                                strcpy(buf3, "are");
                            else
                                strcpy(buf3, "is");
                            cprintf(ch, "The %s %s open.\n\r",
                                    first_name(EXIT(ch, keyword_no)->keyword, buf2), buf3);
                        }
                    }
                }

                distance = (vis = visibility(ch));
                dir = keyword_no;

                if (distance == -1) {
                    if ((tar_room = get_room_distance(ch, 1, dir)) && IS_LIGHT(tar_room))
                        distance = 1;
                    else {
                        cprintf(ch, "You can't see a thing!\n\r");
                        return;
                    }
                } else if (distance == 0) {
                    if ((tar_room = get_room_distance(ch, 1, dir)) && IS_LIGHT(tar_room))
                        distance = 1;
                    else {
                        cprintf(ch, "You can't see that far.\n\r");
                        return;
                    }
                }
                for (; distance > 0; distance--) {
                    if ((tar_room = get_room_distance(ch, distance, dir))) {
                        /* 
                         * if (distance == 1)
                         * from_room = ch->in_room;
                         * else
                         * from_room = get_room_distance(ch, distance - 1, dir);
                         * if (is_secret(from_room->direction[dir])) {
                         * distance = 0;
                         * continue;
                         * }
                         */

                        if (tar_room == EXIT(ch, dir)->to_room) {
                            distance = 1;
                        } else {
                            test_room = get_room_distance(ch, distance - 1, dir);
                            if (test_room == tar_room)
                                distance--;
                        }

                        /*  Let's see if this object seeing thing works.  - Kel  */
                        if (distance < 1)
                            continue;
                        if (distance > 3) {
                            dmin = 10000;
                            cprintf(ch, "[In the faint distance]\n\r");
                        } else if (distance == 3) {
                            dmin = 3000;
                            cprintf(ch, "[Very far]\n\r");
                        } else if (distance == 2) {
                            dmin = 1000;
                            cprintf(ch, "[Far]\n\r");
                        } else if (distance == 1) {
                            dmin = 500;
                            cprintf(ch, "[Near]\n\r");
                        }

                        if ((IS_DARK(tar_room)) && !IS_IMMORTAL(ch)) {
                            // distance = MIN (distance, 1);
                            cprintf(ch, "It's completely dark over there.\n\r");
                            continue;
                        }

                        if ((IS_SET(tar_room->room_flags, RFL_SPL_ALARM))
                            && (IS_AFFECTED(ch, CHAR_AFF_DETECT_MAGICK)))
                            cprintf(ch,
                                    "A shimmering, translucent weave of magick surrounds the entire area.\n\r");

                        /* Vis ppl check */
                        for (cvict = tar_room->people, cansee = FALSE; cvict;
                             cvict = cvict->next_in_room)
                            if (CAN_SEE(ch, cvict))
                                cansee = TRUE;
                        if (cansee)
                            list_char_to_char(tar_room->people, ch, 0);
                        /* Now for objs */
                        for (tmpob = tar_room->contents, found = FALSE; tmpob;
                             tmpob = tmpob->next_content)
                            if (CAN_SEE_OBJ(ch, tmpob))
                                found = TRUE;
                        if (found)
                            /* Found vis objs,
                             * now check weights */
                            for (tmpob = tar_room->contents, found = FALSE; tmpob;
                                 tmpob = tmpob->next_content)
                                if ((CAN_SEE_OBJ(ch, tmpob)) &&
                                    /* use SIZE here!!! */
                                    (tmpob->obj_flags.weight / 100) >= dmin) {
                                    show_obj_to_char(tmpob, ch, 0);
                                    found = TRUE;
                                }
                        if (!(found) && !(cansee))
                            cprintf(ch, "Nothing.\n\r");

                        /* Don't let them see past 1 room if their vision is blurred. */
                        if ((distance > 1)
                                && (find_ex_description("[BLURRED_VISION]", ch->ex_description, TRUE))) {
                            send_to_char("Your blurred vision prevents you from seeing any further.\n\r", ch);
                            return;
                        }
                    }
                }
            } else
                cprintf(ch, "You see nothing special.\n\r");
            break;

        case 6:                /* look 'in'      */
            if (*arg2) {
                int find_bits = FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP;

                /* Item carried */
                bits = generic_find(arg2, find_bits, ch, &tmp_char, &tmp_object);
                if (bits) {     /* Found something */
                    if (*preHow || *postHow) {
                        char obj_keyword[MAX_INPUT_LENGTH];

                        find_obj_keyword(tmp_object, find_bits, ch, obj_keyword,
                                         sizeof(obj_keyword));

                        sprintf(to_char, "you look in ~%s", obj_keyword);
                        sprintf(to_room, "@ looks in ~%s", obj_keyword);

                        // if the emote fails, don't scan the tables
                        if (!send_to_char_and_room_parsed
                            (ch, preHow, postHow, to_char, to_room, to_room, MONITOR_OTHER)) {
                            return;
                        }
                    }
                    switch (GET_ITEM_TYPE(tmp_object)) {
                    case ITEM_DRINKCON:
                        show_drink_container_fullness(ch, tmp_object);
                        break;

                    case ITEM_CONTAINER:
                        if (is_closed_container(ch, tmp_object)) {
                            if (IS_IMMORTAL(ch)) {
                                act("You stick your head into $p and look" " around.", FALSE, ch,
                                    tmp_object, NULL, TO_CHAR);
                            } else {
                                cprintf(ch, "It is closed.\n\r");
                                return;
                            }
                        }
                        /* handle light objects (still handling containers) */
                    case ITEM_LIGHT:
                        if (GET_ITEM_TYPE(tmp_object) == ITEM_LIGHT) {
                            if (IS_REFILLABLE(tmp_object)) {
                                if (IS_LIT(tmp_object)) {
                                    cprintf(ch, "You can't look in %s, it's lit.\n\r",
                                            format_obj_to_char(tmp_object, ch, 1));
                                    return;
                                } else {
                                    if ((tmp_object->obj_flags.value[0] > 0)
                                        && (tmp_object->obj_flags.value[1] > 0)) {
                                        int temp =
                                            tmp_object->obj_flags.value[0] * 3 /
                                            tmp_object->obj_flags.value[1];

                                        temp = MAX(0, MIN(3, temp));
                                        cprintf(ch, "It's %sfull.\n\r", fullness[temp]);
                                        return;
                                    } else {
                                        cprintf(ch, "It's empty.\n\r");
                                        return;
                                    }

                                }
                            } else if (!IS_CAMPFIRE(tmp_object)) {
                                cprintf(ch, "You can't look in %s.\n\r",
                                        format_obj_to_char(tmp_object, ch, 1));
                                return;
                            }
                        }

                        for (num_objs = 0, tmpob = tmp_object->contains; tmpob;
                             tmpob = tmpob->next_content)
                            if (CAN_SEE_OBJ(ch, tmpob))
                                num_objs++;

                        show_list_of_obj_to_char( tmp_object, tmp_object->contains, ch, "In", bits );
                        break;  /* end ITEM_CONTAINER / ITEM_LIGHT */

                    case ITEM_WAGON:
                        tar_room = get_room_num(tmp_object->obj_flags.value[0]);
                        if (!tar_room) {
                            cprintf(ch, "You can't look inside that.\n\r");
                            return;
                        }

                        /* to keep people from seeing the LIMBO room 
                         * 4/21/2001 -Nessalin */
                        if (!tmp_object->obj_flags.value[0]) {
                            cprintf(ch, "You can't see into it.\n\r");
                            return;
                        }
                        was_in = ch->in_room;

                        ch->in_room = tar_room;
                        cprintf(ch, "Looking inside %s, you see:\n\r",
                                OSTR(tmp_object, short_descr));
                        cmd_look(ch, "", 15, 0);
                        ch->in_room = was_in;
                        break;

                    default:
                        cprintf(ch, "That is not a container.\n\r");
                    }           /* end switch GET_ITEM_TYPE */
                } /* end if bits */
                else {          /* wrong argument */
                    cprintf(ch, "You do not see that item here.\n\r");
                }
            } /* end if !*arg2 */
            else                /* no argument */
                cprintf(ch, "Look in what?!?\n\r");
            break;

            /* look 'at'        */
        case 7:
            if (*arg2) {
                int find_bits = FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_CHAR_ROOM;

                bits = generic_find(arg2, find_bits, ch, &tmp_char, &found_object);

                if (tmp_char) { /* Looking at a character, PC or NPC */
                    /*  Added checks for hidden or imm  */
                    if (IS_IMMORTAL(ch)) {
                        show_char_to_char(tmp_char, ch, 1);
                        return;
                    }

                    if (ch != tmp_char) {
                        CHAR_DATA *rch;
                        ch_height =
                            ((GET_POS(ch) >=
                              POSITION_FIGHTING) ? GET_HEIGHT(ch) : (GET_HEIGHT(ch) / 2));
                        if (GET_POS(ch) < POSITION_RESTING)
                            ch_height = 1;

                        if( ch->specials.riding != NULL ) {
                           ch_height = GET_HEIGHT(ch) / 2 + GET_HEIGHT(ch->specials.riding);
                        }

                        tmp_height =
                            ((GET_POS(tmp_char) >=
                              POSITION_FIGHTING) ? GET_HEIGHT(tmp_char) : (GET_HEIGHT(tmp_char)
                                                                           / 2));
                        if (GET_POS(tmp_char) < POSITION_RESTING)
                            tmp_height = 1;

                        if( tmp_char->specials.riding != NULL ) {
                           tmp_height = GET_HEIGHT(tmp_char) / 2 + GET_HEIGHT(tmp_char->specials.riding);
                        }

                        if (ch_height > (tmp_height + 4))
                            strcpy(buf, "down at");
                        else if (tmp_height > (ch_height + 4))
                            strcpy(buf, "up at");
                        else
                            strcpy(buf, "at");

                        sprintf(buf3, "you look %s ~%s", buf, arg2);
                        prepare_emote_message(preHow, buf3, postHow, to_char);

                        /* if emote fails, return */
                        if (!send_to_char_parsed(ch, ch, to_char, FALSE)) {
                            return;
                        }

                        if (IS_AFFECTED(ch, CHAR_AFF_HIDE) || IS_AFFECTED(ch, CHAR_AFF_SNEAK)
                        || IS_IMMORTAL(ch) || affected_by_spell(ch, PSI_VANISH)) {

                        } else {
                            sprintf(buf3, "@ looks %s ~%s", buf, arg2);
                            prepare_emote_message(preHow, buf3, postHow, to_room);

                            if (CAN_SEE(tmp_char, ch))
                                send_to_char_parsed(ch, tmp_char, to_room, FALSE);

                            for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
                                if (rch == tmp_char || rch == ch)
                                    continue;

                                if (CAN_SEE(rch, ch))
                                    send_to_char_parsed(ch, rch, to_room, FALSE);
                            }
                        }
                    }
                    show_char_to_char(tmp_char, ch, 1);
                    return;
                }

                /* light_stuff_here */
                if (!found && found_object) {
                    if (*preHow || *postHow) {
                        sprintf(to_char, "you look at ~%s", arg2);
                        sprintf(to_room, "@ looks at ~%s", arg2);

                        if (!send_to_char_and_room_parsed
                            (ch, preHow, postHow, to_char, to_room, to_room, MONITOR_OTHER)) {
                            return;
                        }
                    }

                    if (OSTR(found_object, description))
                        send_to_char(OSTR(found_object, description), ch);
                    else
                        show_obj_to_char(found_object, ch, 5);
                    show_obj_properties(found_object, ch, bits);
                    return;
                }


                /* Done looking at character, PC or NPC */
                /* Search for Extra Descriptions in room and items */
                /* Extra description in room?? */
                if (!found) {
                    if (!IS_IMMORTAL(ch)
                        && (!strcmp(arg2, "night") || !strcmp(arg2, "mask_desc")))
                        found = FALSE;
                    else {
                        tmp_desc = find_ex_description(arg2, ch->in_room->ex_description, FALSE);
                        if (tmp_desc) {
                            page_string(ch->desc, tmp_desc, 0);
                            return;     /* RETURN, IT WAS A ROOM DESCRIPTION */
                            /* Old system was: found = TRUE; */
                        }
                    }
                }

                /* look at <object> on <object>
                 * or look at <object> <object> (for tables) */
                if (!found) {
                    /* if they use 'on' pop it off and use next to find table */
                    if (!strcmp(arg3, "on") || !strcmp(arg3, "in"))
                        argument = one_argument(argument, arg3, sizeof(argument));

                    if (arg3[0] != '\0') {
                        OBJ_DATA *table;        /* works for containers too -Morg */

                        if ((table = get_obj_in_list_vis(ch, arg3, ch->in_room->contents)) != NULL) {
                            for (tmp_object = table->contains; tmp_object;
                                 tmp_object = tmp_object->next_content) {
                                if (CAN_SEE_OBJ(ch, tmp_object)) {
                                    if (isname(arg2, OSTR(tmp_object, name))) {
                                        if (*preHow || *postHow) {
                                            sprintf(to_char, "you look at %s",
                                                    OSTR(tmp_object, short_descr));
                                            sprintf(to_room, "@ looks at %s",
                                                    OSTR(tmp_object, short_descr));

                                            if (!send_to_char_and_room_parsed
                                                (ch, preHow, postHow, to_char, to_room, to_room,
                                                 MONITOR_OTHER)) {
                                                return;
                                            }
                                        }

                                        page_string(ch->desc, OSTR(tmp_object, description), 1);
                                        show_obj_properties(tmp_object, ch, bits);
                                        found = TRUE;
                                    } else {
                                        tmp_desc =
                                            find_ex_description(arg2, tmp_object->ex_description,
                                                                FALSE);
                                        if (*preHow || *postHow) {
                                            sprintf(to_char, "you look at %s",
                                                    OSTR(tmp_object, short_descr));
                                            sprintf(to_room, "@ looks at %s",
                                                    OSTR(tmp_object, short_descr));

                                            if (!send_to_char_and_room_parsed
                                                (ch, preHow, postHow, to_char, to_room, to_room,
                                                 MONITOR_OTHER)) {
                                                return;
                                            }
                                        }

                                        if (tmp_desc) {
                                            page_string(ch->desc, tmp_desc, 1);
                                            return;
                                        }
                                        found = TRUE;
                                    }
                                }
                            }
                        }
                    }
                }

                /* Search for extra descriptions in items */
                /* Equipment Used */
                if (!found) {
                    for (j = 0; j < MAX_WEAR && !found; j++) {
                        if (ch->equipment[j]) {
                            if (CAN_SEE_OBJ(ch, ch->equipment[j])) {
                                tmp_desc =
                                    find_ex_description(arg2, ch->equipment[j]->ex_description,
                                                        FALSE);
                                if (tmp_desc) {
                                    if (*preHow || *postHow) {
                                        char obj_keyword[MAX_INPUT_LENGTH];

                                        find_obj_keyword(ch->equipment[j], find_bits, ch, obj_keyword,
                                                         sizeof(obj_keyword));

                                        sprintf(to_char, "you look at ~%s", obj_keyword);
                                        sprintf(to_room, "@ looks at ~%s", obj_keyword);

                                        if (!send_to_char_and_room_parsed
                                            (ch, preHow, postHow, to_char, to_room, to_room,
                                             MONITOR_OTHER)) {
                                            return;
                                        }
                                    }

                                    page_string(ch->desc, tmp_desc, 1);
                                    return;
                                }
                            }
                        }
                    }
                }

                /* In inventory */
                if (!found) {
                    for (tmp_object = ch->carrying; tmp_object && !found;
                         tmp_object = tmp_object->next_content) {
                        if (CAN_SEE_OBJ(ch, tmp_object)) {
                            tmp_desc = find_ex_description(arg2, tmp_object->ex_description, FALSE);

                            if (tmp_desc) {
                                page_string(ch->desc, tmp_desc, 1);
                                return;
                            }
                        }
                    }
                }

                /* Object In room */
                if (!found) {
                    for (tmp_object = ch->in_room->contents; tmp_object && !found;
                         tmp_object = tmp_object->next_content) {
                        if (CAN_SEE_OBJ(ch, tmp_object)) {
                            tmp_desc = find_ex_description(arg2, tmp_object->ex_description, FALSE);
                            if (tmp_desc) {
                                if (*preHow || *postHow) {
                                    char obj_keyword[MAX_INPUT_LENGTH];

                                    find_obj_keyword(tmp_object, find_bits, ch, obj_keyword,
                                                     sizeof(obj_keyword));

                                    sprintf(to_char, "you look at ~%s", obj_keyword);
                                    sprintf(to_room, "@ looks at ~%s", obj_keyword);

                                    if (!send_to_char_and_room_parsed
                                        (ch, preHow, postHow, to_char, to_room, to_room,
                                         MONITOR_OTHER)) {
                                        return;
                                    }
                                }

                                page_string(ch->desc, tmp_desc, 1);
                                return;
                            }
                        }
                    }
                }

                /* player edesc's In room */
                if (!found) {
                    struct char_data *tmp_char;

                    for (tmp_char = ch->in_room->people; tmp_char && !found;
                         tmp_char = tmp_char->next_in_room) {
                        if (CAN_SEE(ch, tmp_char)) {
                            tmp_desc = find_ex_description(arg2, tmp_char->ex_description, FALSE);
                            if (tmp_desc) {
                                if (*preHow || *postHow) {
                                    char keyword[MAX_STRING_LENGTH];

                                    find_ch_keyword(tmp_char, ch, keyword, sizeof(keyword));

                                    sprintf(to_char, "you look at ~%s", keyword);
                                    sprintf(to_room, "@ looks at ~%s", keyword);

                                    if (!send_to_char_and_room_parsed
                                        (ch, preHow, postHow, to_char, to_room, to_room,
                                         MONITOR_OTHER)) {
                                        return;
                                    }
                                }

                                page_string(ch->desc, tmp_desc, 1);
                                return;
                            }
                        }
                    }
                }

                /* possesions of another character */
                if (!found && (strlen(arg2) > 2)
                    && (((arg2[strlen(arg2) - 2] == '\'')       /* Azroen's */
                         &&(arg2[strlen(arg2) - 1] == 's'))
                        || (((arg2[strlen(arg2) - 1] == '\'')   /* Morgenes' */
                             &&(arg2[strlen(arg2) - 2] == 's'))))) {
                    strcpy(pos_pers, arg2);
                    strcpy(pos_item, arg3);
                    if (arg2[strlen(arg2) - 1] == '\'')
                        pos_pers[strlen(pos_pers) - 1] = 0;     /* nul terminate */
                    else
                        pos_pers[strlen(pos_pers) - 2] = 0;     /* nul terminate */

                    bits = generic_find(pos_pers, find_bits, ch, &tmp_char, &found_object);

                    if (tmp_char) {
                        found_object =
                            get_object_in_equip_vis(ch, pos_item, tmp_char->equipment, &eq_pos);
                        if (!found_object) {
                            /* player's edescs  */
                            tmp_desc =
                                find_ex_description(pos_item, tmp_char->ex_description, FALSE);
                            if (tmp_desc) {
                                if (*preHow || *postHow) {
                                    sprintf(to_char, "you look at ~%s", pos_pers);
                                    sprintf(to_room, "@ looks at ~%s", pos_pers);

                                    if (!send_to_char_and_room_parsed
                                        (ch, preHow, postHow, to_char, to_room, to_room,
                                         MONITOR_OTHER)) {
                                        return;
                                    }
                                }

                                page_string(ch->desc, tmp_desc, 1);
                                return;
                            }
                        }

                        if (!found_object || is_covered(ch, tmp_char, eq_pos)) {
                            for (i = 0; i < MAX_WEAR_EDESC; i++) {
                                if (!strcmp(pos_item, wear_edesc_long[i].name)) {
                                    tmp_desc =
                                        find_ex_description(wear_edesc_long[i].edesc_name,
                                                            tmp_char->ex_description, TRUE);
                                    tmp_desc2 =
                                        find_ex_description(wear_edesc[i].edesc_name,
                                                            tmp_char->ex_description, TRUE);

                                    if (tmp_desc && tmp_desc2
                                        && !tmp_char->equipment[wear_edesc_long[i].
                                                                location_that_covers]
                                        && !is_desc_covered(ch, tmp_char,
                                                            wear_edesc_long[i].
                                                            location_that_covers)) {
                                        if (*preHow || *postHow) {
                                            sprintf(to_char, "you look at %%%s %s", pos_pers,
                                                    smash_article(tmp_desc2));
                                            sprintf(to_room, "@ looks at %%%s %s", pos_pers,
                                                    smash_article(tmp_desc2));

                                            if (!send_to_char_and_room_parsed
                                                (ch, preHow, postHow, to_char, to_room, to_room,
                                                 MONITOR_OTHER)) {
                                                return;
                                            }
                                        }

                                        page_string(ch->desc, tmp_desc, 1);
                                        return;
                                    } else {
                                        cprintf(ch,
                                                "You see nothing special " "about their %s.\n\r",
                                                pos_item);
                                        return;
                                    }
                                }
                            }
                            act("$E doesn't have anything like that.", FALSE, ch, 0, tmp_char,
                                TO_CHAR);
                            return;
                        }

                        if (found_object) {

                            if (*preHow || *postHow) {
                                sprintf(to_char, "you look at %%%s %s", pos_pers,
                                        smash_article(OSTR(found_object, short_descr)));
                                sprintf(to_room, "@ looks at %%%s %s", pos_pers,
                                        smash_article(OSTR(found_object, short_descr)));

                                // if the emote fails, don't scan the tables
                                if (!send_to_char_and_room_parsed
                                    (ch, preHow, postHow, to_char, to_room, to_room,
                                     MONITOR_OTHER)) {
                                    return;
                                }
                            }

                            if (OSTR(found_object, description)) {
                                send_to_char(OSTR(found_object, description), ch);
                            } else {
                                show_obj_to_char(found_object, ch, 5);
                            }

                            show_obj_properties(found_object, ch, bits);
                            return;
                        };

                    } else if( found_object) {
                        /* object's edescs  */
                        tmp_desc =
                            find_ex_description(pos_item, found_object->ex_description, FALSE);
                        if (tmp_desc) {
                            if (*preHow || *postHow) {
                                sprintf(to_char, "you look at ~%s", pos_pers);
                                sprintf(to_room, "@ looks at ~%s", pos_pers);

                                if (!send_to_char_and_room_parsed
                                    (ch, preHow, postHow, to_char, to_room, to_room,
                                     MONITOR_OTHER)) {
                                    return;
                                }
                            }

                            page_string(ch->desc, tmp_desc, 1);
                            return;
                        }
                        act("$p doesn't have anything like that.", FALSE, ch, found_object, 0, TO_CHAR);
                    } else {
                        sprintf(buf, "You do not see '%s' here.\n\r", pos_pers);
                        send_to_char(buf, ch);
                    }
                }

                /* wrong argument */
                if (bits) {     /* If an object was found */
                    if (!found) /* Show no-description */
                        show_obj_to_char(found_object, ch, 5);
                    else        /* Find hum, glow etc */
                        show_obj_to_char(found_object, ch, 6);
                } else if (!found)
                    send_to_char("You do not see that here.\n\r", ch);
            } else
                /* no argument */
                send_to_char("Look at what?\n\r", ch);
            break;
        case 8:
            {
                if (*preHow || *postHow) {
                    sprintf(to_char, "you look around the room");
                    sprintf(to_room, "@ looks around the room");

                    if (!send_to_char_and_room_parsed
                        (ch, preHow, postHow, to_char, to_room, to_room, MONITOR_OTHER)) {
                        return;
                    }
                }

                /* look ''                */
                if (IS_SWITCHED_IMMORTAL(ch)) {
                    sprintf(tmp, "%s [%d] ", ch->in_room->name, ch->in_room->number);
                    sprint_flag(ch->in_room->room_flags, room_flag, buffer);
                    if (*buffer) {
                        all_cap(buffer, buffer);
                        sprintf(buf2, " [%s]", buffer);
                        strcat(tmp, buf2);
                    }
                } else {
                    /* Change room name if effected by the skellebains */
                    if (affected_by_spell(ch, POISON_SKELLEBAIN) && display_skell > 0) {
                        get_skellebain_room_name(ch, tmp, skell_room, sector_select);
                    } else if (affected_by_spell(ch, POISON_SKELLEBAIN_2) && display_skell > 0) {
                        get_skellebain_room_name(ch, tmp, skell_room, sector_select);
                    } else if (affected_by_spell(ch, POISON_SKELLEBAIN_3) && display_skell > 0) {
                        get_skellebain_room_name(ch, tmp, skell_room, sector_select);
                    } else
                        sprintf(tmp, "%s", ch->in_room->name);
                }
                strcpy(exits, "");
                for (dir = 0; dir < 6; dir++) {
                    if (ch->in_room->direction[dir]
                        && !IS_SET(ch->in_room->direction[dir]->exit_info, EX_SECRET)) {
                        // give a comma and space between exits
                        if (!IS_SET(ch->specials.brief, BRIEF_EXITS)
                         && strlen(exits) > 0) {
                           strcat(exits, ", ");
                        }
                        sprintf(buffer, "%c", UPPER(*dirs[dir]));
                        strcat(exits, buffer);
                    }
                }

                tmpob = find_exitable_wagon_for_room(ch->in_room);
                if (tmpob) {
                    if (*exits) {
                        if( !IS_SET(ch->specials.brief, BRIEF_EXITS)) {
                           strcat(exits, ",");
                        }
                        strcat(exits, " ");
                    }
                    strcat(exits, "Leave");
                }

		if (!IS_SET(ch->specials.brief, BRIEF_NOVICE)) {
		    if (IS_SET(ch->in_room->room_flags, RFL_SAFE)) {
                    if (*exits) {
                        if( !IS_SET(ch->specials.brief, BRIEF_EXITS)) {
                           strcat(exits, ",");
                        }
                        strcat(exits, " ");
                    }
                    strcat(exits, "Quit");
		    }

		    if (is_save_zone(ch->in_room->zone)) {
                    if (*exits) {
                        if( !IS_SET(ch->specials.brief, BRIEF_EXITS)) {
                           strcat(exits, ",");
                        }
                        strcat(exits, " ");
                    }
                    strcat(exits, "Save");
		    }
		}

                if (*exits) {
                    sprintf(buffer, " [%s]", exits);
                    strcat(tmp, buffer);
                }

                strcat(tmp, "\n\r");
                send_to_char(tmp, ch);

                if (!IS_SET(ch->specials.brief, BRIEF_ROOM)) {
                    tmp_desc =
                        find_ex_description("[DET_ETH_DESC]", ch->in_room->ex_description, TRUE);
                    tmp_desc2 =
                        find_ex_description("[DET_INVIS_DESC]", ch->in_room->ex_description, TRUE);
                    if (tmp_desc && (affected_by_spell(ch, SPELL_DETECT_ETHEREAL)
                                     ||
                                     (IS_SET(ch->specials.affected_by, CHAR_AFF_DETECT_ETHEREAL))))
                        send_to_char(tmp_desc, ch);
                    else if (tmp_desc2 && (affected_by_spell(ch, SPELL_DETECT_INVISIBLE)
                                          ||
                                          (IS_SET
                                           (ch->specials.affected_by, CHAR_AFF_DETECT_INVISIBLE))))
                        send_to_char(tmp_desc2, ch);
                    else {
                        if (weather_info.sunlight == SUN_DARK || use_global_darkness) {
                            tmp_desc =
                                find_ex_description("night", ch->in_room->ex_description, FALSE);
                            if (tmp_desc)
                                if (affected_by_spell(ch, POISON_SKELLEBAIN) && display_skell > 0) {
                                    get_skellebain_room_desc(ch, skell_room, sector_select);
                                } else if (affected_by_spell(ch, POISON_SKELLEBAIN_2) && display_skell > 0) {
                                    get_skellebain_room_desc(ch, skell_room, sector_select);
                                } else if (affected_by_spell(ch, POISON_SKELLEBAIN_3) && display_skell > 0) {
                                    get_skellebain_room_desc(ch, skell_room, sector_select);
                                } else {
                                send_to_char(tmp_desc, ch);
                                }
                            else {
                                if (affected_by_spell(ch, POISON_SKELLEBAIN) && display_skell > 0) {
                                    get_skellebain_room_desc(ch, skell_room, sector_select);
                                } else if (affected_by_spell(ch, POISON_SKELLEBAIN_2) && display_skell > 0) {
                                    get_skellebain_room_desc(ch, skell_room, sector_select);
                                } else if (affected_by_spell(ch, POISON_SKELLEBAIN_3) && display_skell > 0) {
                                    get_skellebain_room_desc(ch, skell_room, sector_select);
                                } else {
                                    send_to_char(bdesc_list[ch->in_room->zone]
                                                 [ch->in_room->bdesc_index].desc, ch);
                                    send_to_char(ch->in_room->description, ch);
                                }
                            }
                        } else {
                            if (affected_by_spell(ch, POISON_SKELLEBAIN) && display_skell > 0) {
                                get_skellebain_room_desc(ch, skell_room, sector_select);
                            } else if (affected_by_spell(ch, POISON_SKELLEBAIN_2) && display_skell > 0) {
                                get_skellebain_room_desc(ch, skell_room, sector_select);
                            } else if (affected_by_spell(ch, POISON_SKELLEBAIN_3) && display_skell > 0) {
                                get_skellebain_room_desc(ch, skell_room, sector_select);
                            } else {
                                send_to_char(bdesc_list[ch->in_room->zone]
                                             [ch->in_room->bdesc_index].desc, ch);
                                send_to_char(ch->in_room->description, ch);
                            }
                        }
                    }
                }
                if (IS_SET(ch->in_room->room_flags, RFL_RESTFUL_SHADE))
                    send_to_char("A shadow falls over the area, "
                                 "driving off the uncomfortable heat.\n\r", ch);

                if ((IS_SET(ch->in_room->room_flags, RFL_SPL_ALARM))
                    && (IS_AFFECTED(ch, CHAR_AFF_DETECT_MAGICK)))
                    send_to_char
                        ("A shimmering, translucent weave of magick surrounds the entire area.\n\r",
                         ch);

                /* Wall of Sand Messages here */
                sand_wall_messages(ch);
                thorn_wall_messages(ch);
                wind_wall_messages(ch);
                fire_wall_messages(ch);
                blade_wall_messages(ch);

                /*      for (ex = 0; ex < 6; ex++)
                 * {
                 * if (ch->in_room->direction[ex] &&
                 * IS_SET (ch->in_room->direction[ex]->exit_info,
                 * EX_SPL_FIRE_WALL))
                 * {
                 * sprintf (buffer, "A giant wall of fire blocks "
                 * "the %s exit.\n\r", dirs[ex]);
                 * send_to_char (buffer, ch);
                 * }
                 * }
                 * 
                 * for (ex = 0; ex < 6; ex++)
                 * {
                 * if (ch->in_room->direction[ex] &&
                 * IS_SET (ch->in_room->direction[ex]->exit_info,
                 * EX_SPL_BLADE_BARRIER))
                 * {
                 * sprintf (buffer, "A whirling mass of blades block "
                 * "the %s exit.\n\r", dirs[ex]);
                 * send_to_char (buffer, ch);
                 * }
                 * }
                 * for (ex = 0; ex < 6; ex++)
                 * {
                 * if (ch->in_room->direction[ex] &&
                 * IS_SET (ch->in_room->direction[ex]->exit_info,
                 * EX_SPL_WIND_WALL))
                 * {
                 * sprintf (buffer, "A gusting wind blocks "
                 * "the %s exit.\n\r", dirs[ex]);
                 * send_to_char (buffer, ch);
                 * }
                 * }
                 * for (ex = 0; ex < 6; ex++)
                 * {
                 * if (ch->in_room->direction[ex] &&
                 * IS_SET (ch->in_room->direction[ex]->exit_info,
                 * EX_SPL_THORN_WALL))
                 * {
                 * sprintf (buffer, "A thick growth of thorns blocks "
                 * "the %s exit.\n\r", dirs[ex]);
                 * send_to_char (buffer, ch);
                 * }
                 * }
                 */

                if (IS_SET(ch->in_room->room_flags, RFL_ASH))
                    send_to_char("A fine layer of ash lies over everything." "\n\r", ch);

		// Detect magickal auras
		if (affected_by_spell(ch, SPELL_DETECT_MAGICK)) 
                    show_magick_level_to_char(ch);

                list_obj_to_char(ch->in_room->contents, ch, 0, FALSE);

                list_char_to_char(ch->in_room->people, ch, 0);
            }
            break;
        case 9:
        case 10:
            /* look outside from a wagon.. done correctly this time */
            tmpob = find_wagon_for_room(ch->in_room);
            if (tmpob) {
                if (tmpob->in_room) {
                    if (*preHow || *postHow) {
                        sprintf(to_char, "you look outside");
                        sprintf(to_room, "@ looks outside");

                        // if the emote fails, don't scan the tables
                        if (!send_to_char_and_room_parsed
                            (ch, preHow, postHow, to_char, to_room, to_room, MONITOR_OTHER)) {
                            return;
                        }
                    }

                    was_in = ch->in_room;

                    /* since this is only temporary, don't call char_from/to_room
                     * as that breaks subduing, lights, etc...  Just temporarily
                     * swap out their in_room pointer     -Morg
                     */
                    ch->in_room = tmpob->in_room;

                    /* char_from_room_move should only be called for real
                     * movement -Morg
                     */
                    /* char_from_room_move (ch); 
                     * char_to_room (ch, tmpob->in_room); */
                    cmd_look(ch, "", 15, 0);

                    ch->in_room = was_in;

                    /* char_from_room_move should only be called for real
                     * movement -Morg
                     */
                    /* char_from_room_move (ch);
                     * char_to_room (ch, was_in); */
                } else
                    send_to_char("Your wagon isn't in a room!!\n\r", ch);
            } else {
                if (ch->in_room->sector_type == SECT_INSIDE) {
                    send_to_char("You can't see out from here.\n\r", ch);
                } else {
                    send_to_char("You're not inside anything!\n\r", ch);
                }
            }
            break;
        case 11:
            {
                if (*preHow || *postHow) {
                    sprintf(to_char, "you look around the room");
                    sprintf(to_room, "@ looks around the room");

                    // if the emote fails, don't scan the tables
                    if (!send_to_char_and_room_parsed
                        (ch, preHow, postHow, to_char, to_room, to_room, MONITOR_OTHER)) {
                        return;
                    }
                }

                /* look 'room' for people in brief mode - Kelvik          */
                if (IS_SWITCHED_IMMORTAL(ch)) {
                    sprintf(tmp, "%s [%d]", ch->in_room->name, ch->in_room->number);
                    sprint_flag(ch->in_room->room_flags, room_flag, buffer);
                    if (*buffer) {
                        all_cap(buffer, buffer);
                        sprintf(buf2, " [%s]", buffer);
                        strcat(tmp, buf2);
                    }
                } else
                    sprintf(tmp, "%s", ch->in_room->name);

                strcpy(exits, "");
                for (dir = 0; dir < 6; dir++) {
                    if (ch->in_room->direction[dir]
                        && !IS_SET(ch->in_room->direction[dir]->exit_info, EX_SECRET)) {
                        sprintf(buffer, "%c", UPPER(*dirs[dir]));
                        strcat(exits, buffer);
                    }
                }

                tmpob = find_exitable_wagon_for_room(ch->in_room);
                if (tmpob) {
                    if (*exits)
                        strcat(exits, " ");
                    strcat(exits, "Leave");
                }

		if (!IS_SET(ch->specials.brief, BRIEF_NOVICE)) {
		    if (IS_SET(ch->in_room->room_flags, RFL_SAFE)) {
                        if (*exits)
                            strcat(exits, " ");
                        strcat(exits, "Quit");
		    }

		    if (is_save_zone(ch->in_room->zone)) {
                        if (*exits)
                            strcat(exits, " ");
                        strcat(exits, "Save");
		    }
		}

                if (*exits) {
                    sprintf(buffer, " [%s]", exits);
                    strcat(tmp, buffer);
                }

                strcat(tmp, "\n\r");
                send_to_char(tmp, ch);

                //                send_to_char(bdesc_list[ch->in_room->zone]
                //                             [ch->in_room->bdesc_index].desc, ch);


                tmp_desc = find_ex_description("[DET_ETH_DESC]", ch->in_room->ex_description, TRUE);
                if (tmp_desc && (affected_by_spell(ch, SPELL_DETECT_ETHEREAL)
                                 || (IS_SET(ch->specials.affected_by, CHAR_AFF_DETECT_ETHEREAL))))
                    send_to_char(tmp_desc, ch);
                tmp_desc2 =
                    find_ex_description("[DET_INVIS_DESC]", ch->in_room->ex_description, TRUE);
                if (tmp_desc2 && (affected_by_spell(ch, SPELL_DETECT_INVISIBLE)
                                  || (IS_SET(ch->specials.affected_by, CHAR_AFF_DETECT_INVISIBLE))))
                    send_to_char(tmp_desc2, ch);
                else {
                    if (weather_info.sunlight == SUN_DARK || use_global_darkness) {
                        tmp_desc = find_ex_description("night", ch->in_room->ex_description, FALSE);
                        if (tmp_desc)
                            send_to_char(tmp_desc, ch);
                        else {
                            send_to_char(bdesc_list[ch->in_room->zone]
                                         [ch->in_room->bdesc_index].desc, ch);
                            send_to_char(ch->in_room->description, ch);
                        }
                    } else {
                        send_to_char(bdesc_list[ch->in_room->zone]
                                     [ch->in_room->bdesc_index].desc, ch);
                        send_to_char(ch->in_room->description, ch);
                    }
                }

                //                send_to_char(ch->in_room->description, ch);

                if (IS_SET(ch->in_room->room_flags, RFL_RESTFUL_SHADE))
                    send_to_char("A deep patch of shadow covers the area, "
                                 "driving off the uncomfortable heat.\n\r", ch);
                if (IS_SET(ch->in_room->room_flags, RFL_ASH))
                    send_to_char("A layer of grey ash lies over everything" ".\n\r", ch);
                for (ex = 0; ex < 6; ex++) {
                    if (ch->in_room->direction[ex]
                        && IS_SET(ch->in_room->direction[ex]->exit_info, EX_SPL_SAND_WALL)) {
                        sprintf(buffer, "A giant wall of sand blocks " "the %s exit.\n\r",
                                dirs[ex]);
                        send_to_char(buffer, ch);
                    }
                }

                for (ex = 0; ex < 6; ex++) {
                    if (ch->in_room->direction[ex]
                        && IS_SET(ch->in_room->direction[ex]->exit_info, EX_SPL_FIRE_WALL)) {
                        sprintf(buffer, "A giant wall of fire blocks " "the %s exit.\n\r",
                                dirs[ex]);
                        send_to_char(buffer, ch);
                    }
                }

                for (ex = 0; ex < 6; ex++) {
                    if (ch->in_room->direction[ex]
                        && IS_SET(ch->in_room->direction[ex]->exit_info, EX_SPL_BLADE_BARRIER)) {
                        sprintf(buffer, "A whirling mass of blades block " "the %s exit.\n\r",
                                dirs[ex]);
                        send_to_char(buffer, ch);
                    }
                }

                for (ex = 0; ex < 6; ex++) {
                    if (ch->in_room->direction[ex]
                        && IS_SET(ch->in_room->direction[ex]->exit_info, EX_SPL_WIND_WALL)) {
                        sprintf(buffer, "Gusting winds block " "the %s exit.\n\r", dirs[ex]);
                        send_to_char(buffer, ch);
                    }
                }

            }
            break;
        case 12:               /* look 'on'        */
            {
                if (*arg2) {
                    int findBits = FIND_CHAR_ROOM | FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP;

                    /* Item carried */
                    bits = generic_find(arg2, findBits, ch, &tmp_char, &tmp_object);

                    if (bits == FIND_CHAR_ROOM && tmp_char) {
                        cprintf(ch, "You can only look on furniture," " %s doesn't qualify.\n\r",
                                PERS(ch, tmp_char));
                        return;
                    }

                    if (bits) { /* Found something */
                        if (GET_ITEM_TYPE(tmp_object) == ITEM_FURNITURE
                            && IS_SET(tmp_object->obj_flags.value[1], FURN_PUT)) {
                            if (*preHow || *postHow) {
                                char obj_keyword[MAX_INPUT_LENGTH];

                                find_obj_keyword(tmp_object, findBits, ch, obj_keyword,
                                                 sizeof(obj_keyword));

                                sprintf(to_char, "you look at ~%s", obj_keyword);
                                sprintf(to_room, "@ looks at ~%s", obj_keyword);

                                if (!send_to_char_and_room_parsed
                                    (ch, preHow, postHow, to_char, to_room, to_room,
                                     MONITOR_OTHER)) {
                                    return;
                                }
                            }

                            show_list_of_obj_to_char( tmp_object, tmp_object->contains, ch, "On", bits );
                        } else {
                            send_to_char("There's nothing on it.\n\r", ch);
                        }
                    } else {    /* wrong argument */
                        send_to_char("You do not see that item here.\n\r", ch);
                    }
                } else {
                    /* no argument */
                    send_to_char("Look on what?!\n\r", ch);
                }
            }
            break;
        case -1:
            /* wrong arg */
            send_to_char("Sorry, I didn't understand that!\n\r", ch);
            break;
        }
    }
}

/* end of look */




void
cmd_read(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH];
    char my_temp[MAX_STRING_LENGTH];
    OBJ_DATA *found_object;
    CHAR_DATA *tmp_char;
    struct extra_descr_data *tmp_desc;

    if (!ch->desc)
        return;

    if (GET_POS(ch) < POSITION_SLEEPING) {
        cprintf(ch, "You can't see anything but stars!\n\r");
        return;
    }

    if (GET_POS(ch) == POSITION_SLEEPING) {
        cprintf(ch, "You can't see anything; you're sleeping!\n\r");
        return;
    }

    if (IS_AFFECTED(ch, CHAR_AFF_BLIND)) {
        cprintf(ch, "You can't see a thing; you're blinded!\n\r");
        return;
    }

    if (affected_by_spell(ch, SPELL_ELEMENTAL_FOG) && has_sight_spells_resident(ch)) {
        cprintf(ch, "You can't see a thing; A fog covers your eyes!\n\r");
        return;
    }

    if (IS_DARK(ch->in_room) && !IS_AFFECTED(ch, CHAR_AFF_INFRAVISION) && !IS_IMMORTAL(ch)
        && (GET_RACE(ch) != RACE_HALFLING) && (GET_RACE(ch) != RACE_MANTIS)
        && (GET_RACE(ch) != RACE_GHOUL) && (GET_RACE(ch) != RACE_VAMPIRE)) {
        cprintf(ch, "You can't read in the dark.\n\r");
        return;
    }

    argument = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

    if (*arg1) {
        generic_find(arg1, FIND_OBJ_INV, ch, &tmp_char, &found_object);

        if (!found_object) {
            cprintf(ch, "You aren't carrying any '%s'.\n\r", arg1);
            return;
        }

        if (GET_ITEM_TYPE(found_object) == ITEM_NOTE) {
            int pageNum = 1;

            if (IS_SET(found_object->obj_flags.value[3], NOTE_BROKEN))
                cprintf(ch, "%s has a broken seal.\n\r",
                        capitalize(format_obj_to_char(found_object, ch, 1)));
            else if (IS_SET(found_object->obj_flags.value[3], NOTE_SEALED)) {
                cprintf(ch, "%s is sealed and cannot be read until it is broken.\n\r",
                        capitalize(format_obj_to_char(found_object, ch, 1)));
                return;
            }

            /* if they specified a page */
            if (*arg2) {
                /* if it's a numeric argument */
                if (is_number(arg2)) {
                    /* convert to an int */
                    pageNum = atoi(arg2);

                    /* prepend the page_ as we expect the edesc to be labeled */
                    sprintf(arg2, "page_%d", pageNum);
                }
                /* check if they specified page_# */
                else if (strlen(arg2) > 5 && is_number(arg2 + 5)) {
                    pageNum = atoi(arg2 + 5);
                }
            } else {
                sprintf(arg2, "page_%d", pageNum);
                if (found_object->obj_flags.value[0] > 1)
                    cprintf(ch, "You read page %d.\n\r", pageNum);
            }

            /* page count vs. page requested */
            if (found_object->obj_flags.value[0] < pageNum) {
                cprintf(ch, "%s only has %d pages.\n\r",
                        capitalize(format_obj_to_char(found_object, ch, 1)),
                        found_object->obj_flags.value[0]);
                return;
            }

            for (tmp_desc = found_object->ex_description; tmp_desc; tmp_desc = tmp_desc->next)
                if (isname(arg2, tmp_desc->keyword)
                    && isnameprefix("page_", tmp_desc->keyword))
                    break;

            if (tmp_desc) {
                if (read_item(found_object, ch, my_temp)) {
                    cprintf(ch, "%s", my_temp);
                    page_string(ch->desc, tmp_desc->description, 1);
		    if (found_object->obj_flags.value[4] != 0) {
		      int bookSkill = found_object->obj_flags.value[4];
		      int bookSkillMaxPerc = found_object->obj_flags.value[5];
		      char learnString[MAX_STRING_LENGTH];
		      int oldSkillLearnedValue;
		      if (has_skill(ch, bookSkill)) {
			if (ch->skills[bookSkill]->learned < 
			    bookSkillMaxPerc) {
			  if (ch->skills[bookSkill]->learned > 
			      (bookSkillMaxPerc - 30)) {
			    oldSkillLearnedValue = ch->skills[bookSkill]->learned;
			    sprintf(learnString, "You study about %s.\n\r", skill_name[bookSkill]);
			    send_to_char(learnString, ch);
			    gain_skill(ch, bookSkill, 1);
			    if (ch->skills[bookSkill]->learned != 
				oldSkillLearnedValue) {
			      sprintf(learnString, "You learn something about %s.\n\r", skill_name[bookSkill]);
			      send_to_char(learnString, ch);
			    }
			  }
			  else {
			    sprintf(learnString, "You don't know enough about %s, yet, to learn from this book.\n\r", skill_name[bookSkill]);
			    send_to_char(learnString, ch); 
			  }
			}
			else {
			  sprintf(learnString, "You know more about %s than this book does, it can't teach you anything new.\n\r", skill_name[bookSkill]);
			  send_to_char(learnString, ch); 
			}
		      }
		      else {
			sprintf(learnString, "You don't know anything about %s, this book won't be of use to you until you learn the basics.\n\r", skill_name[bookSkill]);
			send_to_char(learnString, ch); 
		      }
		    }
                } else {
                    cprintf(ch, "%s", my_temp);
                }

                return;
            } else {
                cprintf(ch, "It's blank.\n\r");
                return;
            }
        } else {
            cprintf(ch, "You can't read that.\n\r");
            return;
        }
    } else {
        cprintf(ch, "What do you want to read?\n\r");
        return;
    }
}



void
cmd_examine(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char name[100], buf[100];
    int bits;
    OBJ_DATA *obj;
    CHAR_DATA *tch;

    sprintf(buf, "at %s", argument);
    cmd_look(ch, buf, 15, 0);

    argument = one_argument(argument, name, sizeof(name));

    if (!*name) {
        send_to_char("Examine what?\n\r", ch);
        return;
    }

    bits = generic_find(name, FIND_OBJ_INV | FIND_OBJ_ROOM | FIND_OBJ_EQUIP, ch, &tch, &obj);

    if (obj) {
        if ((GET_ITEM_TYPE(obj) == ITEM_DRINKCON)
            || (GET_ITEM_TYPE(obj) == ITEM_CONTAINER)
            || (GET_ITEM_TYPE(obj) == ITEM_LIGHT && IS_CAMPFIRE(obj))) {
            sprintf(buf, "in %s", name);
            cmd_look(ch, buf, 15, 0);
        }
    }
    /* if you're wondering why there's not a not found message here, it's
     * handled by the cmd_look() call at the beginning of this cmd
     */
}

void
cmd_exits(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int door;
    char buf[MAX_STRING_LENGTH], buf2[256];
    OBJ_DATA *tmpob;
    CHAR_DATA *staff = SWITCHED_PLAYER(ch);

    int skell_room;             /* Room to diasplay on Skellebain */
    
    int display_skell;
 
    char tmp[256];

    int sector_select;

    static char *exits[] = {
        "North",
        "East ",
        "South",
        "West ",
        "Up   ",
        "Down "
    };

    *buf = '\0';

    if ((IS_DARK(ch->in_room) || (visibility(ch) <= 0))
        && ((!IS_IMMORTAL(staff)) && (!IS_AFFECTED(ch, CHAR_AFF_INFRAVISION)))) {
        send_to_char("You stumble around, looking for the exits.\n\r", ch);
        adjust_move(ch, -1);
    }

    for (door = 0; door <= 5; door++)
        if (EXIT(ch, door))
            if (EXIT(ch, door)->to_room && !((door == DIR_UP) && !IS_IMMORTAL(staff)
                                             && IS_SET(EXIT(ch, door)->to_room->room_flags,
                                                       RFL_FALLING))
                && !IS_SET(EXIT(ch, door)->exit_info, EX_CLOSED)) {
                if (((IS_DARK(EXIT(ch, door)->to_room)) && !IS_IMMORTAL(ch))
                    || (IS_AFFECTED(ch, CHAR_AFF_BLIND))
                    || (affected_by_spell(ch, SPELL_ELEMENTAL_FOG)
                        && has_sight_spells_resident(ch)))
                    sprintf(buf + strlen(buf), "%s - Darkness", exits[door]);
                else if ((EXIT(ch, door)->to_room->sector_type == SECT_SILT) && !IS_IMMORTAL(ch))
                    sprintf(buf + strlen(buf), "%s - Silt", exits[door]);
                else if ((visibility(ch) <= 0)
                         && (!IS_SET(EXIT(ch, door)->to_room->room_flags, RFL_INDOORS))
                         && !IS_IMMORTAL(ch))
                    sprintf(buf + strlen(buf), "%s - Faint", exits[door]);
                else {
                     // Verify if exit should be displayed 
                     if (affected_by_spell(ch, POISON_SKELLEBAIN)) {
                         display_skell = number(0, 1);  /* Displays skell name 50%*/
                     } else if (affected_by_spell(ch, POISON_SKELLEBAIN_2)) {
                         display_skell = number(0, 3);  /* Displays skell name 25%*/
                     } else  if (affected_by_spell(ch, POISON_SKELLEBAIN_3)) {
                         display_skell = number(0, 6);  /* Displays skell name 15%*/
                     }

                     if (affected_by_spell(ch, POISON_SKELLEBAIN) && display_skell > 0) {
                        if (number(1, 100) < 25) {         // 25% of the time it will show a rand sector
                            sector_select = number(0, 26); // Choose random sector to display
                        } else {
                            sector_select = ch->in_room->sector_type;
                        }
                        skell_room = number(0,4);   /* Select room on each exit while on Skellebain */
                        get_skellebain_room_name(ch, tmp, skell_room, sector_select);
                        sprintf(buf + strlen(buf), "%s - %s", exits[door], tmp);
                    } else if (affected_by_spell(ch, POISON_SKELLEBAIN_2) && display_skell > 0) {
                        if (number(1, 100) < 25) {         // 25% of the time it will show a rand sector
                            sector_select = number(0, 26); // Choose random sector to display
                        } else {
                            sector_select = ch->in_room->sector_type;
                        }
                        skell_room = number(0,4);   /* Select room on each exit while on Skellebain */
                        get_skellebain_room_name(ch, tmp, skell_room, sector_select);
                        sprintf(buf + strlen(buf), "%s - %s", exits[door], tmp);
                    } else if (affected_by_spell(ch, POISON_SKELLEBAIN_3) && display_skell > 0) {
                        if (number(1, 100) < 25) {         // 25% of the time it will show a rand sector
                            sector_select = number(0, 26); // Choose random sector to display
                        } else {
                            sector_select = ch->in_room->sector_type;
                        }
                        skell_room = number(0,4);   /* Select room on each exit while on Skellebain */
                        get_skellebain_room_name(ch, tmp, skell_room, sector_select);
                        sprintf(buf + strlen(buf), "%s - %s", exits[door], tmp);
                    } else
                        sprintf(buf + strlen(buf), "%s - %s", exits[door],
                            EXIT(ch, door)->to_room->name);
                }

                // allow switched staff to see the truth & darkness
                if (ch != staff && IS_IMMORTAL(staff)
                    && (IS_DARK(EXIT(ch, door)->to_room)
                        || IS_AFFECTED(ch, CHAR_AFF_BLIND)
                        || (affected_by_spell(ch, SPELL_ELEMENTAL_FOG)
                            && has_sight_spells_resident(ch))
                        || EXIT(ch, door)->to_room->sector_type == SECT_SILT
                        || visibility(ch) <= 0))
                    sprintf(buf + strlen(buf), " (%s)", EXIT(ch, door)->to_room->name);

                if (!IS_IMMORTAL(staff))
                    strcat(buf, "\n\r");
                else {
                    sprintf(buf2, " [%d]\n\r", EXIT(ch, door)->to_room->number);
                    strcat(buf, buf2);
                }
            }

    tmpob = find_exitable_wagon_for_room(ch->in_room);

    if (tmpob) {
        sprintf(buf2, "Leave - %s", tmpob->in_room->name);
        strcat(buf, buf2);

        if (!IS_IMMORTAL(staff))
            strcat(buf, "\n\r");
        else {
            sprintf(buf2, " [%d]\n\r", tmpob->in_room->number);
            strcat(buf, buf2);
        }
    }

    cprintf(ch, "Visible exits:\n\r");

    if (*buf)
        cprintf(ch, "%s", buf);
    else
        cprintf(ch, "None.\n\r");
}

extern int get_normal_condition(CHAR_DATA * ch, int cond);

void
cmd_score(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    struct time_info_data playing_time, player_age;
    char privsstring[4096];
    struct zone_priv_data *priv;
    char no_ldesc[40];
    int zone, g, none, j;
    extern char *age_names[];

    cprintf(ch, "You are %s", IS_NPC(ch) ? MSTR(ch, short_descr) : GET_NAME(ch));

    if (ch->clan) {
        if (!ch->clan->next && ch->clan->clan < MAX_CLAN && !IS_NPC(ch)) {
            int clan = ch->clan->clan;
            int rank = ch->clan->rank;
            char *rname = get_clan_rank_name(clan, rank);

            cprintf(ch, ", %s%sof the %s", rname ? rname : "", rname ? " " : "",
                    smash_article(clan_table[clan].name));
        } else if (ch->clan->next) {
            cprintf(ch, ", of many people. (type 'tribes' to see your tribes)");
        }
    }
    cprintf(ch, ".\n\r");

    if (!IS_NPC(ch)) {
        cprintf(ch, "Keywords: %s\n\r", ch->player.extkwds);
    }

    cprintf(ch, "Sdesc: %s\n\r", MSTR(ch, short_descr));

    if (ch->player.info[1]) {
        cprintf(ch, "Objective: %s\n\r", ch->player.info[1]);
    }

    cprintf(ch, "Long Description:\n\r");
    strcpy(no_ldesc, "Code Generated Long Description.");
    if (!IS_NPC(ch) || (IS_NPC(ch) && ch->long_descr)) {
        if ((strncmp(MSTR(ch, long_descr), "A ", 2)
             && strncmp(MSTR(ch, long_descr), "An ", 3)
             && strncmp(MSTR(ch, long_descr), "The ", 4)) || !IS_NPC(ch)) {
            cprintf(ch, "%s%s%s\n\r", ch->long_descr ? capitalize(PERS(ch, ch)) : "",
                    ch->long_descr ? " " : "", ch->long_descr ? ch->long_descr : no_ldesc);

        } else {
            cprintf(ch, "%s\n\r", capitalize(ch->long_descr));
        }
    } else {
        cprintf(ch, "%s\n\r", capitalize(MSTR(ch, long_descr)));
    }

    player_age = age(ch);
    cprintf(ch, "You are %d years, %d months, and %d days old,\n\r", player_age.year,
            player_age.month, player_age.day);

    g = age_class(get_virtual_age(player_age.year, get_apparent_race(ch)));
    g = MAX(0, g);
    g = MIN(5, g);
    cprintf(ch, " which by your race and appearance is %s.\n\r", age_names[g]);
    cprintf(ch, "You are %d inches tall, and weigh %d ten-stone.\n\r", GET_HEIGHT(ch),
            GET_WEIGHT(ch));

    cprintf(ch, "Your strength is %s, your agility is %s,\n\r", STAT_STRING(ch, ATT_STR),
            STAT_STRING(ch, ATT_AGL));
    cprintf(ch, "  your wisdom is %s, and your endurance is %s.\n\r", STAT_STRING(ch, ATT_WIS),
            STAT_STRING(ch, ATT_END));

    if (!IS_IMMORTAL(ch)) {
        int hunger_percent = PERCENTAGE(GET_COND(ch, FULL),
         get_normal_condition(ch, FULL));
        int thirst_percent = PERCENTAGE(GET_COND(ch, THIRST),
         get_normal_condition(ch, THIRST));

         /* either hungry or thirsty, so build it up properly */
         cprintf(ch, "You are ");

         /* based on echoes from limits.c, if you adjust these, adjust
          * limits.c echoes as well
          */
         if (hunger_percent < 4)
             cprintf(ch, "starving and ");
         else if (hunger_percent >= 4 && hunger_percent < 8)
             cprintf(ch, "famished and ");
         else if (hunger_percent >= 8 && hunger_percent < 16)
             cprintf(ch, "very hungry and ");
         else if (hunger_percent >= 16 && hunger_percent < 22)
             cprintf(ch, "hungry and ");
         else if (hunger_percent >= 22 && hunger_percent < 40)
             cprintf(ch, "a little hungry and ");
         else if (hunger_percent >= 40 && hunger_percent < 55)
             cprintf(ch, "peckish and ");
         else if (hunger_percent >= 55 && hunger_percent < 73)
             cprintf(ch, "satisfied and ");
         else if (hunger_percent >= 73 && hunger_percent < 100)
             cprintf(ch, "full and ");
         else if (hunger_percent >= 100)
             cprintf(ch, "stuffed and ");

         if (thirst_percent >= 40)
             cprintf(ch, "not thirsty.");
         if (thirst_percent < 40 && thirst_percent >= 22)
             cprintf(ch, "a little thirsty.");
         if (thirst_percent < 22 && thirst_percent >= 16)
             cprintf(ch, "thirsty.");
         if (thirst_percent < 16 && thirst_percent >= 8)
             cprintf(ch, "very thirsty.");
         if (thirst_percent < 8 && thirst_percent >= 4)
             cprintf(ch, "parched.");
         if (thirst_percent < 4)
             cprintf(ch, "dehydrated!");

         cprintf(ch, "\n\r");
    }
    switch (is_char_drunk(ch)) {
    case DRUNK_LIGHT:
        cprintf(ch, "You are light-headed.\n\r");
        break;
    case DRUNK_MEDIUM:
        cprintf(ch, "You are semi-intoxicated.\n\r");
        break;
    case DRUNK_HEAVY:
        cprintf(ch, "You are intoxicated.\n\r");
        break;
    case DRUNK_SMASHED:
        cprintf(ch, "You are extremely intoxicated.\n\r");
        break;
    case DRUNK_DEAD:
        cprintf(ch, "You are totally plastered.\n\r");
        break;
    default:
        break;
    }

    if (is_magicker(ch))
        cprintf(ch,
                "Your health is %d(%d), you have %d(%d) mana, %d(%d) "
                "stamina,\n\rand %d(%d) stun.\n\r", GET_HIT(ch), GET_MAX_HIT(ch), GET_MANA(ch),
                GET_MAX_MANA(ch), GET_MOVE(ch), GET_MAX_MOVE(ch), GET_STUN(ch), GET_MAX_STUN(ch));
    else
        cprintf(ch, "Your health is %d(%d), you have %d(%d) stamina, and" " %d(%d) stun.\n\r",
                GET_HIT(ch), GET_MAX_HIT(ch), GET_MOVE(ch), GET_MAX_MOVE(ch), GET_STUN(ch),
                GET_MAX_STUN(ch));

    if (IS_IMMORTAL(ch)) {
        if (GET_LEVEL(ch) > LEGEND) {
            cprintf(ch, "\n\rYou own the following zones: \n\r");

            for (zone = 0, none = FALSE; zone <= top_of_zone_table; zone++) {
                if (zone_table[zone].uid == ch->specials.uid) {
                    cprintf(ch, "     Zone %d: %s.\n\r", zone, zone_table[zone].name);
                    none = TRUE;
                }
            }
            for (zone = 0, none = FALSE; zone <= top_of_zone_table; zone++) {
                for (priv = zone_table[zone].privs; priv; priv = priv->next)
                    if (priv->uid == ch->specials.uid) {
                        sprint_flag(priv->type, grp_priv, privsstring);
                        cprintf(ch, "    Zone %d: %s: %s\n\r", zone, zone_table[zone].name,
                                privsstring);
                    }
            }

            if (!none)
                cprintf(ch, "     None.\n\r");
            cprintf(ch, "\n\r");

            cprintf(ch, "You have group rights in the following zones: \n\r");

            for (g = 0, none = FALSE; g < MAX_GROUPS; g++) {
                for (zone = 0; zone <= top_of_zone_table; zone++) {
                    if (IS_SET(ch->specials.group, grp[g].bitv)
                        && IS_SET(zone_table[zone].groups, grp[g].bitv)) {
                        cprintf(ch, "     Zone %d: %s [%s].\n\r", zone, zone_table[zone].name,
                                grp[g].name);
                        none = TRUE;
                    }
                }
            }

            if (!none)
                cprintf(ch, "     None.\n\r");
            cprintf(ch, "\n\r");

        }

        if (ch->specials.il) {
            cprintf(ch, "Your invisibility level is %d.\n\r", ch->specials.il);
        }
    }

    playing_time = real_time_passed((time(0) - ch->player.time.logon) + ch->player.time.played, 0);
    cprintf(ch, "\n\rYou have been playing for %d days and %d hours.\n\r", playing_time.day,
            playing_time.hours);

    switch (GET_POS(ch)) {
    case POSITION_DEAD:
        cprintf(ch, "You are DEAD!\n\r");
        break;
    case POSITION_MORTALLYW:
        cprintf(ch, "You are mortally wounded, and will probably die soon " "if not aided.\n\r");
        break;
    case POSITION_STUNNED:
        cprintf(ch, "You are stunned, unable to move.\n\r");
        break;
    case POSITION_SLEEPING:
        cprintf(ch, "You are asleep");
        if (ch->on_obj) {
            cprintf(ch, " on %s", format_obj_to_char(ch->on_obj, ch, 1));
            if (ch->on_obj->table) {
                cprintf(ch, ", at %s", format_obj_to_char(ch->on_obj->table, ch, 1));
            }
        }
        cprintf(ch, ".\n\r");
        break;
    case POSITION_RESTING:
        cprintf(ch, "You are resting");
        if (ch->on_obj) {
            cprintf(ch, " on %s", format_obj_to_char(ch->on_obj, ch, 1));
            if (ch->on_obj->table) {
                cprintf(ch, ", at %s", format_obj_to_char(ch->on_obj->table, ch, 1));
            }
        }
        cprintf(ch, ".\n\r");
        break;
    case POSITION_SITTING:
        cprintf(ch, "You are sitting");
        if (ch->on_obj) {
            cprintf(ch, " on %s", format_obj_to_char(ch->on_obj, ch, 1));
            if (ch->on_obj->table) {
                cprintf(ch, ", at %s", format_obj_to_char(ch->on_obj->table, ch, 1));
            }
        }
        cprintf(ch, ".\n\r");
        break;
    case POSITION_FIGHTING:
        if (ch->specials.fighting)
            if( trying_to_disengage(ch)) {
                cprintf(ch, "You are defending against %s.\n\r",
                 PERS(ch, ch->specials.fighting));
            } else {
                cprintf(ch, "You are fighting %s.\n\r", 
                 PERS(ch, ch->specials.fighting));
            }
        else
            cprintf(ch, "You are standing.\n\r");
        break;
    case POSITION_STANDING:
        if (!ch->specials.riding) {
            cprintf(ch, "You are standing");
            if (ch->on_obj) {
                if (IS_SET(ch->on_obj->obj_flags.value[1], FURN_SKIMMER)
                 || IS_SET(ch->on_obj->obj_flags.value[1], FURN_WAGON))
                    cprintf(ch, " on %s", format_obj_to_char(ch->on_obj, ch, 1));
                else
                    cprintf(ch, " at %s", format_obj_to_char(ch->on_obj, ch, 1));
                if (ch->on_obj->table) {
                    cprintf(ch, ", at %s", format_obj_to_char(ch->on_obj->table, ch, 1));
                }
            }
            cprintf(ch, ".\n\r");
        } else
            cprintf(ch, "You are riding %s.\n\r", PERS(ch, ch->specials.riding));
        break;
    default:
        cprintf(ch, "You are floating.\n\r");
        break;
    }

    if (GET_SPOKEN_LANGUAGE(ch) < 0 || GET_SPOKEN_LANGUAGE(ch) >= MAX_SKILLS) {
        cprintf(ch, "An error occurred while attempting to determine your spoken language, please wish up.\n\r");
        return;
    }

    g = GET_SPOKEN_LANGUAGE(ch);
    for (j = 0; accent_table[j].skillnum != LANG_NO_ACCENT; j++)
        if (accent_table[j].skillnum == GET_ACCENT(ch))
            break;
    if (accent_table[j].skillnum != LANG_NO_ACCENT) {
        cprintf(ch, "You are currently speaking %s with %s%s accent.\n\r", skill_name[g],
                indefinite_article(accent_table[j].name), accent_table[j].name);
    } else {
        cprintf(ch, "You are currently speaking %s with an unknown accent.\n\r", skill_name[g]);
    }
}

void
cmd_time(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int day, age, year, yr1, yr2, dotw;
    char buf[265], tmp[256];
    char *suf;
    char buffer[MAX_STRING_LENGTH];

    if (!ch || !ch->in_room)
        return;

    if (get_room_extra_desc_value(ch->in_room, "[time_failure]", buffer, MAX_STRING_LENGTH)) {
        cprintf(ch, "%s", buffer);    
        return;
    }

    /* Added so people underground lose their sense of time. -Sanvean */
    if (has_special_on_cmd(ch->in_room->specials, "cave_emulator", -1)
        || ch->in_room->sector_type == SECT_SOUTHERN_CAVERN
        || ch->in_room->sector_type == SECT_CAVERN 
        || ch->in_room->sector_type == SECT_SEWER) {
        cprintf(ch, 
         "Deep underground, you find you have lost all sense of time.\n\r");
        return;
    }

    /* You can't tell the time in the elemental planes. -Raesanos */
    if (ch->in_room->zone == 34) {
        cprintf(ch, "You are unable to determine the time here.\n\r");
        return;
    }

    if (IS_AFFECTED(ch, CHAR_AFF_SLEEP)) {
        send_to_char("Lost in sleepy dreams, you have no idea how much time has passed.\n\r", ch);
        return;
    }

    day = time_info.day + 1;
    age = (time_info.year / 77) + 1;
    year = (time_info.year % 77) + 1;
    yr1 = time_info.year % 11;
    yr2 = time_info.year % 7;

    switch (time_info.hours) {
    case NIGHT_EARLY:
        strcpy(tmp, "before dawn");
        break;
    case DAWN:
        strcpy(tmp, "dawn");
        break;
    case RISING_SUN:
        strcpy(tmp, "early morning");
        break;
    case MORNING:
        strcpy(tmp, "late morning");
        break;
    case HIGH_SUN:
        strcpy(tmp, "high sun");
        break;
    case AFTERNOON:
        strcpy(tmp, "early afternoon");
        break;
    case SETTING_SUN:
        strcpy(tmp, "late afternoon");
        break;
    case DUSK:
        strcpy(tmp, "dusk");
        break;
    case NIGHT_LATE:
        strcpy(tmp, "late at night");
        break;
    default:
        gamelog("Illegal hour in time_info.");
        time_info.hours = DAWN;
        break;
    }

    suf = number_suffix(day);

#ifdef DOTW_SUPPORT
    dotw = day % MAX_DOTW;
    sprintf(buf, "It is %s on %s, the %d%s day of the %s,\n\r", tmp, weekday_names[dotw], day, suf,
            sun_phase[(int) time_info.month]);
#else
    sprintf(buf, "It is %s on the %d%s day of the %s,\n\r", tmp, day, suf,
            sun_phase[(int) time_info.month]);
#endif
    send_to_char(buf, ch);

    suf = number_suffix(age);

    sprintf(buf, "In the Year of %s's %s, year %d of the %d%s Age.\n\r", year_one[yr1],
            year_two[yr2], year, age, suf);
    send_to_char(buf, ch);

}


void
cmd_weather(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int i, dir, wn_heat, nb_heat, cnt, day;
    char buf1[256], buf[256];
    struct weather_node *wn;
    struct weather_node *nb;
    float condition = 0.0;
    int velocity = 0;
    ROOM_DATA *wroom;
    char jphase[30], lphase[30];
    char jpos[15], lpos[15], epos[15];

    // By defalt, use the room the character is in
    wroom = ch->in_room;
    if (!wroom)
        return;

    argument = one_argument(argument, buf, sizeof(buf));
    if (strlen(buf) > 0) {
        int dir = get_direction_from_name(buf);
        wroom = get_room_distance(ch, 1, dir);
        if (!wroom) {
            cprintf(ch, "There is no room to the '%s'.\n\r", buf);
            return;
        }
        send_to_char("Peering into the next room...\n\r", ch);
    }

    if (!(IS_IMMORTAL(ch)) && IS_SET(wroom->room_flags, RFL_INDOORS)) {
        send_to_char("You have no feeling about the weather indoors.\n\r", ch);
        return;
    }

    dir = weather_info.wind_direction;
    nb_heat = 0;
    cnt = 0;

    for (wn = wn_list; wn; wn = wn->next) {
        if (wn->zone == wroom->zone)
            break;
    }

    if (wn) {
        if (wn->neighbors[rev_dir[dir]]) {
            for (i = 0; i < wn->neighbors[rev_dir[dir]]; i++) {
                nb = wn->near_node[rev_dir[dir]][i];
                if (nb) {
                    nb_heat += nb->temp;
                    cnt++;
                };
            }
        }
        if (!nb_heat)
            nb_heat = wn->temp;
        else
            nb_heat = (nb_heat / cnt);
        wn_heat = wn->temp;

        nb_heat = MIN((int) (nb_heat / 15), 9);
        wn_heat = MIN((int) (wn_heat / 15), 9);
        nb_heat = MAX(nb_heat, 0);
        wn_heat = MAX(wn_heat, 0);

        char tide_name[15];
        if ((weather_info.tides >= 0) && (weather_info.tides <= 19))        sprintf(tide_name, "very low");
        else if ((weather_info.tides >= 20) && (weather_info.tides <= 39))  sprintf(tide_name, "low");
        else if ((weather_info.tides >= 40) && (weather_info.tides <= 59))  sprintf(tide_name, "normal");
        else if ((weather_info.tides >= 60) && (weather_info.tides <= 79))  sprintf(tide_name, "high");
        else if ((weather_info.tides >= 80) && (weather_info.tides <= 100)) sprintf(tide_name, "very high");


        if (IS_IMMORTAL(ch)) {
            sprintf(buf, "%% Wind(all zones): %s at %d\n\r",
                    dir_name[(int) weather_info.wind_direction], weather_info.wind_velocity);
            send_to_char(buf, ch);
            sprintf(buf, "%% Front(all zones): %d\n\r", weather_info.pressure);
            send_to_char(buf, ch);
            sprintf(buf, "%% Temp(zone): %d min:(%d) max:(%d)\n\r", wn->temp, wn->min_temp,
                    wn->max_temp);
            send_to_char(buf, ch);
            sprintf(buf, "%% Condition(zone): %.1f min:(%.1f) max:(%.1f) median (%.1f)\n\r", wn->condition,
              wn->min_condition, wn->max_condition, wn->median_condition);
            send_to_char(buf, ch);
            sprintf(buf, "%% Life(zone): %d max:(%d)\n\r", wn->life, wn->max_life);
            send_to_char(buf, ch);
            sprintf(buf, "%% Tides(zone): %d (%s)\n\r\n\r", weather_info.tides, tide_name);
            send_to_char(buf, ch);

            sprintf(buf, "%% Weather > 8.5 (Tremendous sandstorm) Wind >= 60 (Mighty gale)\n\r");
            send_to_char(buf, ch);
            sprintf(buf,
                    "%% Weather > 7.0 (Fierce storm)         Wind >= 40 (Fierce [temperature] wind)\n\r");
            send_to_char(buf, ch);
            sprintf(buf,
                    "%% Weather > 5.0 (Harsh storm)          Wind >= 25 (Strong [temperature] wind)\n\r");
            send_to_char(buf, ch);
            sprintf(buf,
                    "%% Weather > 2.5 (Gritty sand storm)    Wind >= 15 ([temperature] Wind)\n\r");
            send_to_char(buf, ch);
            sprintf(buf,
                    "%% Weather (Sky is clear)               Wind >= 5  ([temperature] Breeze)\n\r");
            send_to_char(buf, ch);
            sprintf(buf, "%% Wind (Air is silent)\n\r");
            send_to_char(buf, ch);
            sprintf(buf, "%% Air Temp is determined by average of neighbors temp.\n\r");
            send_to_char(buf, ch);
            sprintf(buf, "%% Every hour:\n\r");
            send_to_char(buf, ch);

            sprintf(buf, "%%   1 in 4 chance of temp + rand(-5 to 5)\n\r");
            send_to_char(buf, ch);
            sprintf(buf, "%%   front + rand(-50 to 150)\n\r");
            send_to_char(buf, ch);
            sprintf(buf, "%%   when front > 1000 wind changes directon\n\r");
            send_to_char(buf, ch);
        }

        if (ch->in_room->sector_type == SECT_SILT || ch->in_room->sector_type == SECT_SHALLOWS) {
        sprintf(buf, "It is %s%s %s.\n\rThe level of the silt appears to be %s.\n\r", indefinite_article(temperature_name[wn_heat]),
                temperature_name[wn_heat], (weather_info.sunlight == SUN_DARK) ? "night" : "day", tide_name);
        } else {
        sprintf(buf, "It is %s%s %s.\n\r", indefinite_article(temperature_name[wn_heat]),
                temperature_name[wn_heat], (weather_info.sunlight == SUN_DARK) ? "night" : "day");
        }
        send_to_char(buf, ch);

/* modified this so the amount of sand message also checks  */
/* the wind condition and doesn't return a storm message if */
/* wind velocity is less than 5    5/4/00  -Sanvean         */

        condition = 0.0;
        velocity = 0;

        if (wn) {
            condition = wn->condition;
            velocity = weather_info.wind_velocity;
        }
        if (IS_SET(wroom->room_flags, RFL_SANDSTORM)) {
            condition += 5.0;
        }
        if (IS_SET(wroom->room_flags, RFL_INDOORS)) {
            condition -= 10.0;
        }

        sprintf(buf1, "%s", temperature_name[nb_heat]);

        if (condition >= 8.50)
            cprintf(ch,
             "A tremendous sandstorm whips stinging dust from %s across the land.\n\r",
             rev_dir_name[dir]);
        else if (condition >= 7.00)
            cprintf(ch,
             "A fierce storm whirls sand and dust from %s, obscuring the sky.\n\r",
             rev_dir_name[dir]);
        else if (condition >= 5.00)
            cprintf(ch,
             "A harsh sandstorm from %s fills the air with whirling sand and dust.\n\r",
             rev_dir_name[dir]);
        else if (condition >= 2.50)
            cprintf(ch,
             "Gritty sand blows in from %s, piling in small dunes.\n\r",
             rev_dir_name[dir]);

        else { // Only show wind message if there isn't a noticable sandstorm
            if (velocity >= 60)
                cprintf(ch, "A mighty gale wind screams out of %s.\n\r", rev_dir_name[dir]);
            else if (velocity >= 40)
                cprintf(ch, "A fierce %s wind howls out of %s.\n\r", buf1, rev_dir_name[dir]);
            else if (velocity >= 25)
                cprintf(ch, "A strong %s wind blows from %s.\n\r", buf1, rev_dir_name[dir]);
            else if (velocity >= 15)
                cprintf(ch, "%s %s wind blows from %s.\n\r", (nb_heat >= 8) ? "An" : "A", buf1,
                        rev_dir_name[dir]);
            else if (velocity >= 5)
                cprintf(ch, "%s %s breeze blows from %s.\n\r", (nb_heat >= 8) ? "An" : "A", buf1,
                        rev_dir_name[dir]);
            else
                cprintf(ch, "The air is as silent as the sand.\n\r");
        }

	if (!use_global_darkness) 
	{

            // Get waxing/waning char
            if (weather_info.moon[JIHAE] < 4)
                strcpy(jpos, "eastern");
            else if (weather_info.moon[JIHAE] > 3)
                strcpy(jpos, "western");
            
            if (weather_info.moon[LIRATHU] < 6)
                strcpy(lpos, "eastern");
            else if (weather_info.moon[LIRATHU] > 5)
                strcpy(lpos, "western");
            
            if (weather_info.moon[ECHRI] < 15)
                strcpy(epos, "eastern");
            else if (weather_info.moon[ECHRI] > 15)
                strcpy(epos, "western");
            else if (weather_info.moon[ECHRI] == 15)
                strcpy(epos, "center of the");
            
            // Phase logic here
            day = time_info.day + 1;
            if ((day > 0 && day < 3) || (day > 229 && day < 232)) { // New Moon
                 strcpy(lphase, "Almost imperceptible,");
            } else if (day > 2 && day < 31) { // waxing, new crescent
                 strcpy(lphase, "The thin, waxing crescent of");
            } else if (day > 30 && day < 60) { // waxing, crescent
                 strcpy(lphase, "The thick, waxing crescent of");
            } else if (day > 59 && day < 87) { // half-moon waxing
                 strcpy(lphase, "Half illuminated and waxing,");
            } else if (day > 146 && day < 174) { // half-moon waning
                 strcpy(lphase, "Half illuminated and waning,");
            } else if (day > 86 && day < 115) { // waxing, gibbous 
                 strcpy(lphase, "The thick, waxing gibbous of");
            } else if (day > 114 && day < 119) { // full moon
                 strcpy(lphase, "Bright and full,");
            } else if (day > 118 && day < 147) { // waning, gibbous
                 strcpy(lphase, "The thick, waning gibbous of");
            } else if (day > 173 && day < 202) { // waning crescent
                 strcpy(lphase, "The thick, waning crescent of");
            } else if (day > 201 && day < 230) { // waning, new crescent
                 strcpy(lphase, "The thin, waning crescent of");
            }

            if ((day > 48 && day < 52) || (day > 163 && day < 168)) { // new moon
                 strcpy(jphase, "Almost imperceptible,");
            } else if ((day > 51 && day < 65) || (day > 167 && day < 181)) { //  waxing, new crescent
                 strcpy(jphase, "The thin, waxing crescent of");
            } else if ((day > 64 && day < 78) || (day > 180 && day < 194)) { // waxing, crescent
                 strcpy(jphase, "The thick, waxing crescent of");
            } else if ((day > 77  && day < 92 ) || (day > 193 && day < 208)) { // half-moon
                 strcpy(jphase, "Half illuminated and waxing,");
            } else if ((day > 123 && day < 138) || (day > 8   && day < 23 )) {
                 strcpy(jphase, "Half illuminated and waning,");
            } else if ((day > 91 && day < 106) || (day > 207 && day < 222)) { // waxing, gibbous
                 strcpy(jphase, "The thick, waxing gibbous of");
            } else if ((day > 105 && day < 110) || (day > 221 && day < 226)) { // full moon
                 strcpy(jphase, "Bright and full,");
            } else if ((day > 109 && day < 124) || (day > 225 && day < 232) || (day > 0 && day < 9)) { // waning, gibbous
                 strcpy(jphase, "The thick, waning gibbous of");
            } else if ((day > 137 && day < 151) || (day > 22 && day < 36)) { // waning crescent
                 strcpy(jphase, "The thick, waning crescent of");
            } else if ((day > 150 && day < 164) || (day > 35 && day < 49)) { // waning, new crescent
                 strcpy(jphase, "The thin, waning crescent of");
            }

            if ((weather_info.moon[JIHAE] == JIHAE_RISE) || (weather_info.moon[JIHAE] == JIHAE_SET)) {
                if (GET_RACE(ch) == RACE_HALFLING)
                    cprintf(ch, "%s the red moon hangs low in the %s sky.\n\r", jphase, jpos);
                else if (IS_TRIBE(ch, 24))
                    cprintf(ch, "%s the red moon, Ganin, hangs low in the %s sky.\n\r", jphase, jpos);
                else
                    cprintf(ch, "%s the red moon hangs low in the %s sky.\n\r", jphase, jpos);
            } else if ((weather_info.moon[JIHAE] > JIHAE_RISE)
                       && (weather_info.moon[JIHAE] < JIHAE_SET)) {
                if (GET_RACE(ch) == RACE_HALFLING)
                    cprintf(ch, "%s the red moon is high in the %s sky.\n\r", jphase, jpos);
                else if (IS_TRIBE(ch, 24))
                    cprintf(ch, "%s the red moon, Ganin, is high in the %s sky.\n\r", jphase, jpos);
                else
                    cprintf(ch, "%s the red moon is high in the %s sky.\n\r", jphase, jpos);
            }
            if ((weather_info.moon[LIRATHU] == LIRATHU_RISE)
                || (weather_info.moon[LIRATHU] == LIRATHU_SET)) {
                if (GET_RACE(ch) == RACE_HALFLING)
                     cprintf(ch, "%s the white moon hangs low in the %s sky.\n\r", lphase, lpos);
                else if (IS_TRIBE(ch, 24))
                     cprintf(ch, "%s the white moon, Alkahar, hangs low in the %s sky.\n\r", lphase, lpos);
                else
                     cprintf(ch, "%s the white moon, hangs low in the %s sky.\n\r", lphase, lpos);
            } else if ((weather_info.moon[LIRATHU] > LIRATHU_RISE)
                       && (weather_info.moon[LIRATHU] < LIRATHU_SET)) {
                if (GET_RACE(ch) == RACE_HALFLING)
                    cprintf(ch, "%s the white moon is high in the %s sky.\n\r", lphase, lpos);
                else if (IS_TRIBE(ch, 24))
                    cprintf(ch, "%s the white moon, Alkahar, is high in the %s sky.\n\r", lphase, lpos);
                else
                    cprintf(ch, "%s the white moon is high in the %s sky.\n\r", lphase, lpos);
            }
    	    if (use_shadow_moon && time_info.hours >= DAWN && time_info.hours <= DUSK) {
                if ((weather_info.moon[ECHRI] == ECHRI_RISE) || (weather_info.moon[ECHRI] == ECHRI_SET)) {
                    if (GET_RACE(ch) == RACE_HALFLING)
                        cprintf(ch, "The black moon hangs low in the %s sky.\n\r", epos);
                    else if (IS_TRIBE(ch, 24))
                        cprintf(ch, "The black moon, Kuato, hangs low in the %s sky.\n\r", epos);
                    else
                        cprintf(ch, "Hanging low in the %s sky is the black moon.\n\r", epos);
                } else if ((weather_info.moon[ECHRI] > ECHRI_RISE)
                       && (weather_info.moon[ECHRI] < ECHRI_SET)) {
                    if (GET_RACE(ch) == RACE_HALFLING)
                        cprintf(ch, "The black moon is high in the %s sky.\n\r", epos); 
                        //send_to_char("The black moon is high in the sky.\n\r", ch);
                    else if (IS_TRIBE(ch, 24))
                        cprintf(ch, "The black moon, Kuato, is high in the %s sky.\n\r", epos);
                        //send_to_char("The black moon, Kuato, is high in the sky.\n\r", ch);
                    else
                        cprintf(ch, "High in the %s sky is the black moon.\n\r", epos);
                        //send_to_char("High in the sky is the black moon.\n\r", ch);
                }
            } // Daylight check to see Echri
	} // End global darkness

        if (affected_by_spell(ch, POISON_SKELLEBAIN) && number(0, 3) > 0)
            send_to_char("An eerie, blue-colored moon sits low in the sky.\n\r", ch);
        if (affected_by_spell(ch, POISON_SKELLEBAIN_2) && number(0, 3) > 0)
            send_to_char("An eerie, purple-colored moon warbles low in the sky.\n\r", ch);
        if (affected_by_spell(ch, POISON_SKELLEBAIN_3) && number(0, 3) > 0)
            send_to_char("An eerie, green-colored moon pulses faintly low in the sky.\n\r", ch);



    } else
        send_to_char("You have no feeling about the weather at all.\n\r", ch);
}

struct weather_node *
get_weather_node_room(ROOM_DATA * rm)
{
    struct weather_node *wn;

    for (wn = wn_list; wn; wn = wn->next) {
        if (wn->zone == rm->zone)
            break;
    }
    return (wn);
}



#define NEW_HELP_DIR "data_files/help_files"


void
cmd_help(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
  FILE *fp;
  int i;
  char buf[MAX_STRING_LENGTH], fname[256];
  char *output = NULL;
  char *new_argument = 0;


  if (!ch->desc)
    return;
  
    if (!argument) {
      file_to_string(HELP_PAGE_FILE, &help);
      page_string(ch->desc, help, 1);
      return;
    }

    while (*argument == ' ')
        argument++;

    /* look at everything they typed */
    strcpy(buf, argument);

    if (buf[0] == '\0') {
        file_to_string(HELP_PAGE_FILE, &help);
        page_string(ch->desc, help, 1);
        return;
    }

    for (i = 0; buf[i] != '\0'; i++) {
        /* uppercase the help name */
        buf[i] = UPPER(buf[i]);

        /* replace blanks with underscores */
        if (buf[i] == ' ')
            buf[i] = '_';
    }

#define HELP_SKILL_CHECKS
#ifdef HELP_SKILL_CHECKS
    /* check to see if they have the associated power if PSI_ or SPELL_ */
    if (!str_prefix("PSI_", buf) || !str_prefix("SPELL_", buf)) {
        int sk;
        char name[MAX_STRING_LENGTH];
        char *pname;

        /* find the first underscore in the name */
        pname = strchr(buf, '_');

        /* move past the underscore */
        pname++;

        /* copy into our own buffer */
        strcpy(name, pname);

        pname = name;

        /* convert the help name into a skill name */
        for (; *pname; pname++) {
            /* lowercase the help name */
            *pname = LOWER(*pname);

            /* replace _ with ' ' for skill name */
            if (*pname == '_')
                *pname = ' ';
	}

/*
     sprintf( buf2, "Looking for skill '%s'.\n\r", name );
     send_to_char( buf2, ch );
*/

        for (sk = 1; sk < MAX_SKILLS; sk++)
            if (!strnicmp(skill_name[sk], name, strlen(name)))
                break;

        if (sk != MAX_SKILLS) {
            if (!has_skill(ch, sk)) {
	      cprintf(ch, "You don't have the skill '%s'.\n\r", name);
                return;
            }
        }

        if (sk == MAX_SKILLS) {
            cprintf(ch, "There is no such skill or spell, '%s'.\n\r", name);
            return;
        }
    }
#endif

    sprintf(fname, "%s/%s", NEW_HELP_DIR, buf);

    if (!(fp = fopen(fname, "r"))) {
        if ((fp = fopen(HELP_FILE, "a"))) {
            fprintf(fp, "%s\n", buf);
            fclose(fp);
        }
        cprintf(ch, "There is no help on topic '%s'.\n\r", argument);
        return;
    }
    fclose(fp);

    file_to_string(fname, &output);
    if (!output) {
        gamelogf("file_to_string failed in cmd_help for %s", fname);
        return;
    }
    page_string(ch->desc, output, 1);
    free(output);
}


#define WIZ_HELP_DIR "data_files/wizhelp_files"

void
cmd_wizhelp(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    FILE *fp;
    int i;
    char buf[256], fname[256];
    char *output = NULL;

    if (!ch->desc)
        return;

    while (*argument == ' ')
        argument++;

#define BLANKS_IN_HELPS
#ifdef BLANKS_IN_HELPS
    /* look at everything they typed */
    strcpy(buf, argument);
#else
    /* removed this so that we can look at everything they typed */
    argument = one_argument(argument, buf, sizeof(buf));
#endif

    if (buf[0] == '\0') {
        page_string(ch->desc, godhelp, 0);
        return;
    }

    for (i = 0; buf[i] != '\0'; i++) {
        buf[i] = UPPER(buf[i]);
#ifdef BLANKS_IN_HELPS
        /* replace blanks with underscores */
        if (buf[i] == ' ')
            buf[i] = '_';
#endif
    }

    sprintf(fname, "%s/%s", WIZ_HELP_DIR, buf);

    if (!(fp = fopen(fname, "r"))) {
        send_to_char("There is no help on that topic.\n\r", ch);
        return;
    }
    fclose(fp);

    file_to_string(fname, &output);
    if (!output) {
        gamelogf("file_to_string failed in cmd_wizhelp for %s", fname);
        return;
    }
    page_string(ch->desc, output, 1);
    free(output);
}

void
cmd_who(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    DESCRIPTOR_DATA *d;
    CHAR_DATA *view;
    int lev = -1, plrs;
    char buf[256], buf2[256], tmp[256], show_list[MAX_STRING_LENGTH];
    char levstr[256], arg[256];
    //  char *message=0;
    int clan = 0, show_clan = 0;
    bool show_biographies = FALSE;
    bool show_objective = FALSE;
    bool show_feedback = FALSE;
    bool show_sdesc = FALSE;
    bool show_newbie = FALSE;
    bool show_guild = FALSE;
    bool show_sub_guild = FALSE;
    bool show_city = FALSE;
    bool show_vitals = FALSE;
    bool show_pvitals = FALSE;
    bool show_room = FALSE;
    int show_skill = -1;
    int ct = 0;
    int numbios = 0;
    int numchars = 5;
    int show_clan_flag, tmp_clan = 0;
    bool show_zone = FALSE;
    int zone_num = -1;
    int guild_num = -1, sub_guild_num = -1;
    char old_arg[MAX_STRING_LENGTH];
    CHAR_DATA *viewer = SWITCHED_PLAYER(ch);

    strcpy(show_list, "");

    argument = one_argument(argument, tmp, sizeof(tmp));

    if ((*tmp == 'k') && IS_IMMORTAL(viewer)) {
        strcpy(old_arg, argument);
        argument = one_argument(argument, arg, sizeof(arg));

        if (arg[0] == '\0') {
            send_to_char( "You must name a skill.\n\r", ch );
            return;
        }

        for (show_skill = 1; show_skill < MAX_SKILLS; show_skill++) {
            if (!str_prefix(old_arg, skill_name[show_skill]))
                break;
        }

        if (show_skill == MAX_SKILLS) {
            cprintf(ch, "Unable to find skill '%s'.\n\r", old_arg);
            return;
        }
    }

    // Options are only available to immortals
    if (IS_IMMORTAL(viewer)) {

    char tmpvital[2];

    if (*tmp == 'v') {
        show_vitals = TRUE;
    } else if (*tmp == 'p') {
        show_pvitals = TRUE;
    } else if (*tmp == 'c') {
        show_clan_flag = TRUE;

        strcpy(old_arg, argument);
        argument = one_argument(argument, arg, sizeof(arg));

        strncpy(tmpvital, old_arg, 1);
        char* tmp_old_arg = old_arg + 2;

        if (arg[0] == '\0') {
            clan = -1;
        } else if (!is_number(arg)) {
            if (*tmpvital == 'v') {
                if (strlen(tmp_old_arg) < 3) {
                    clan = atoi(tmp_old_arg);
                    if (clan < 0 || clan >= MAX_CLAN) {
                        send_to_char("No such clan.\n\r", ch);
                        return;
                    }
                    show_vitals = TRUE;
                } else {
                    clan = lookup_clan_by_name(tmp_old_arg);
                    if (clan == -1) {
                        send_to_char("No such clan.\n\r", ch);
                        return;
                    }
                    show_vitals = TRUE;
                }
            } else if (*tmpvital == 'p') {
                if (strlen(tmp_old_arg) < 3) {
                    clan = atoi(tmp_old_arg);
                    if (clan < 0 || clan >= MAX_CLAN) {
                        send_to_char("No such clan.\n\r", ch);
                        return;
                    }
                    show_pvitals = TRUE;
                } else {
                    clan = lookup_clan_by_name(tmp_old_arg);

                    if (clan == -1) {
                        send_to_char("No such clan.\n\r", ch);
                        return;
                    }
                show_pvitals = TRUE;
                }
            } else {
                clan = lookup_clan_by_name(old_arg);

                if (clan == -1) {
                    send_to_char("No such clan.\n\r", ch);
                    return;
                }
            }
        } else {
            clan = atoi(arg);

            if (clan < 0 || clan >= MAX_CLAN) {
                send_to_char("No such clan.\n\r", ch);
                return;
            }
        }
    } else if (*tmp == 'r') {
        if (*argument == 'v') {
            show_vitals = TRUE;
        } else if (*argument == 'p') {
            show_pvitals = TRUE;
        }
        show_room = TRUE;
    } else if (*tmp == 'f') {
        show_feedback = TRUE;
    } else if (*tmp == 'd') {
        show_sdesc = TRUE;
    } else if (*tmp == 'o') {
        show_objective = TRUE;
    } else if (*tmp == 'n') {
        strcpy(old_arg, argument);
        argument = one_argument(argument, arg, sizeof(arg));

        if ((arg[0] != '\0') && (is_number(arg))) {
            numchars = atoi(arg);
        }

        show_newbie = TRUE;
    } else if (*tmp == 'b') {
        strcpy(old_arg, argument);
        argument = one_argument(argument, arg, sizeof(arg));

        if ((arg[0] != '\0') && (is_number(arg))) {
            numbios = atoi(arg);
        }

        show_biographies = TRUE;
    } else if (*tmp == 'z') {
        strcpy(old_arg, argument);
        argument = one_argument(argument, arg, sizeof(arg));

        strncpy(tmpvital, old_arg, 1);
        char* tmp_old_arg = old_arg + 2;

        if (*tmpvital == 'v') {
            if (strlen(tmp_old_arg) < 3) {
                zone_num = atoi(tmp_old_arg);
                show_zone = TRUE;
            }
        show_vitals = TRUE;
        } else if (*tmpvital == 'p') {
            if (strlen(tmp_old_arg) < 3) {
                zone_num = atoi(tmp_old_arg);
                show_zone = TRUE;
            }
        show_pvitals = TRUE;
        } else {
            if ((arg[0] != '\0') && (is_number(arg))) {
                zone_num = atoi(arg);
            }
        show_zone = TRUE;
        }
    } else if (*tmp == 'g') {
        strcpy(old_arg, argument);
        argument = one_argument(argument, arg, sizeof(arg));
        for (guild_num = 1; guild_num < MAX_GUILDS; guild_num++)
            if (!str_prefix(old_arg, guild[guild_num].name))
                break;

        if (guild_num == MAX_GUILDS) {
            send_to_char("No such guild.\n\r", ch);
            return;
        }
        show_guild = TRUE;
    } else if (*tmp == 's') {
        strcpy(old_arg, argument);
        argument = one_argument(argument, arg, sizeof(arg));
        for (sub_guild_num = 1; sub_guild_num < MAX_SUB_GUILDS; sub_guild_num++)
            if (!str_prefix(old_arg, sub_guild[sub_guild_num].name))
                break;

        if (sub_guild_num == MAX_SUB_GUILDS) {
            send_to_char("No such subguild.\n\r", ch);
            return;
        }
        show_sub_guild = TRUE;
    } else if (*tmp == 'l') {
        strcpy(old_arg, argument);
        argument = one_argument(argument, arg, sizeof(arg));
	if (arg[0] != '\0') {
	    if (is_number(arg))
		ct = atoi(arg);
	    else
		for (ct = 0; ct < MAX_CITIES; ct++)
		    if (!str_prefix(old_arg, city[ct].name))
			break;
	    if (ct <= 0 || ct >= MAX_CITIES) {
		send_to_char("No such city.\n\r", ch);
		return;
	    }
	}
        show_city = TRUE;
    } else if (is_number(tmp))
        lev = atoi(tmp);
    } // End IS_IMMORTAL(viewer)


    plrs = 0;

    if ((IS_IMMORTAL(viewer) && lev < 1 && GET_LEVEL(viewer) > BUILDER) || clan) {
        if( show_skill != -1 ) {
            cprintf(ch, "Players with skill %s\n\r------------------------\n\r",
             skill_name[show_skill] );
        } else {
            send_to_char("Players\n\r-------\n\r", ch);
        }
    }
    else
        send_to_char("Immortals\n\r---------\n\r", ch);

    for (d = descriptor_list; d; d = d->next) {
        if (!d->connected && d->character) {
            view = 0;
            int charbios = 0;

            // Don't show people without a character, if there is a level limit
            if (lev != -1 && !d->original && !d->character)
                continue;

            if (d->original) {
                if (CAN_SEE(viewer, d->original)
                    && (lev == 0 ? GET_LEVEL(d->character) == 0 : GET_LEVEL(d->original) >= lev)
                    && ((IS_IMMORTAL(viewer) && GET_LEVEL(ch) > BUILDER)
                        || IS_IMMORTAL(d->original) || clan)) {
                    view = d->original;
                }
            } else if (CAN_SEE(viewer, d->character)
                       && (lev == 0 ? GET_LEVEL(d->character) == 0 : GET_LEVEL(d->character) >= lev)
                       && ((IS_IMMORTAL(viewer) && GET_LEVEL(viewer) > BUILDER)
                           || IS_IMMORTAL(d->character) || clan)) {
                view = d->character;
            }

            if (!view)
                continue;

            if( show_skill != -1
             && ( !has_skill(view, show_skill) ||IS_IMMORTAL(view) ))
                continue;

            if (clan > 0 && !IS_TRIBE(view, clan))
                continue;

            if (clan == -1) {
                if (IS_IMMORTAL(viewer) && !IS_IN_SAME_TRIBE(ch, view))
                    continue;
                else if (!IS_IMMORTAL(viewer)) {
                    CLAN_DATA *pC, *pC2;

                    /* this is essentially is_in_same_clan code, with
                     * the complication of checking for specific tribes */
                    /* could write a procedure to do this, but this would
                     * be quicker than calling and looping for each tribe */
                    show_clan = 0;
                    for (pC = ch->clan; pC; pC = pC->next)
                        for (pC2 = view->clan; pC2; pC2 = pC2->next)
                            if (pC->clan == pC2->clan
                                && (pC->clan == 6 || pC->clan == 7 || pC->clan == 8 || pC->clan == 9
                                    || pC->clan == 11 || pC->clan == 12 || pC->clan == 14
                                    || pC->clan == 24 || pC->clan == 25 || pC->clan == 30
                                    || pC->clan == 48 || pC->clan == 62 || pC->clan == 64)) {
                                show_clan = pC->clan;
                                break;
                            }

                    if (show_clan == 0)
                        continue;
                }
            }

            if (show_city && (char_in_city(view) != ct || IS_IMMORTAL(view)))
                continue;

            if (show_guild && GET_GUILD(view) != guild_num)
                continue;

            if (show_sub_guild && GET_SUB_GUILD(view) != sub_guild_num)
                continue;

            if (show_objective && IS_IMMORTAL(view))
                continue;

            /* if immortal is looking for those who want feedback */
            if (show_feedback
                /* and the person to view is an immortal */
                && (IS_IMMORTAL(view)
                    /* or not an immortal and doesn't want feedback */
                    || (!IS_IMMORTAL(view)
                        && !IS_SET(d->player_info->flags, PINFO_WANTS_RP_FEEDBACK))))
                /* skip them */
                continue;

            if (show_newbie && (IS_IMMORTAL(view)
                                || (!IS_IMMORTAL(view)
                                    && (d->player_info->num_chars > numchars))))
                /* skip them */
                continue;

  //          if (show_biographies) {
  //              charbios = get_num_biographies(view);

  //              if (IS_IMMORTAL(view)
  //               || (!IS_IMMORTAL(view) && numbios > charbios))
                    /* skip them */
  //                  continue;
  //          }

            if (show_zone && view->in_room && (view->in_room->zone != zone_num))
                continue;

            if (show_room && (view->in_room->number != ch->in_room->number))
                continue;

            plrs++;

            if (GET_LEVEL(viewer) < STORYTELLER && !clan) {
                if (!strcmp(GET_NAME(view), "Edward"))
                    strcpy(buf,
                           "The constellation of Edward is in the far "
                           "west, gleaming majestically.");

		else if (!strcmp(GET_NAME(view), "Adhira"))
		  strcpy(buf,
			 "Gracing the southern horizon, a distant cluster of "
			 "stars marks the presence of Adhira.");

                else if (!strcmp(GET_NAME(view), "Kelvik"))
                    strcpy(buf,
                           "The twin blue stars of Kelvik's Eyes " "circle languidly overhead.");

                else if (!strcmp(GET_NAME(view), "Nessalin"))
                    strcpy(buf, "The glowing Nessalin Nebula flickers " "eternally overhead.");

                else if (!strcmp(GET_NAME(view), "Ur"))
                    strcpy(buf, "The seven-star cluster of Ur burns in the " "southern sky.");
                else if (!strcmp(GET_NAME(view), "Tlaloc"))
                    strcpy(buf, "The lone star of Tlaloc burns independently in the sky.");
                else if (!strcmp(GET_NAME(view), "Halaster"))
                    strcpy(buf,
                           "A murky shroud obscures the stars, marking "
                           "the appearance of Halaster.");

                else if (!strcmp(GET_NAME(view), "Azroen"))
                    strcpy(buf, "The brilliant comet of Azroen moves swiftly " "across the sky.");

                else if (!strcmp(GET_NAME(view), "Stehle"))
                    strcpy(buf, "The crimson star of Stehle smoulders on the " "horizon.");

                else if (!strcmp(GET_NAME(view), "Tenebrius"))
                    strcpy(buf, "The Tenebrius cloud of stars looms in the " "west.");

                else if (!strcmp(GET_NAME(view), "Blaylock"))
                    strcpy(buf, "The twinkling stars of Blaylock shine in " "the southern sky.");

                else if (!strcmp(GET_NAME(view), "Thanas"))
                    strcpy(buf,
                           "The elven cluster of Thanas runs silently " "across the night sky.");

                else if (!strcmp(GET_NAME(view), "Sanvean"))
                    strcpy(buf,
                           "A glimmer of starlight marks the presence " "of Sanvean in the west.");

                else if (!strcmp(GET_NAME(view), "Bhagharva"))
                    strcpy(buf, "The black sphere of Bhagharva blots out " "part of the sky.");

                else if (!strcmp(GET_NAME(view), "Bremen"))
                    strcpy(buf, "The fiery meteor of Bremen lightens the " "northern sky.");

                else if (!strcmp(GET_NAME(view), "Morgenes"))
                    strcpy(buf,
                           "The solitary white star of Morgenes burns"
                           " brightly on the northern horizon.");

                else if (!strcmp(GET_NAME(view), "Savak"))
                    strcpy(buf,
                           "The nebulous mist of Savak looms in an " "ominous cloud overhead.");

                else if (!strcmp(GET_NAME(view), "Tiernan"))
                    strcpy(buf,
                           "The emerald planet of Tiernan shimmers "
                           "clearly in the southern sky.");

                else if (!strcmp(GET_NAME(view), "Becklee"))
                    strcpy(buf,
                           "Red-orange flashes of light streak across "
                           "across the sky in Becklee's wake.");

                else if (!strcmp(GET_NAME(view), "Eris"))
                    strcpy(buf,
                           "The translucent bands of the Eris aurora "
                           "dance erratically above the horizon.");

                else if (!strcmp(GET_NAME(view), "Krrx"))
                    strcpy(buf,
                           "A pale white nimbus in the eastern sky " "marks the presence of Krrx.");
                else if (!strcmp(GET_NAME(view), "Raesanos"))
                    strcpy(buf,
                           "The violet-ringed planet of Raesanos "
                           "hangs over the northern mountains.");
                else
                    sprintf(buf, "The constellation of %s circles about " "the center of the sky.",
                            GET_NAME(view));
            } else if (show_vitals) {            
                if (GET_LEVEL(view) < LEGEND)
                    strcpy(levstr, "Mrtl");
                else if (GET_LEVEL(view) < BUILDER)
                    strcpy(levstr, "Lgnd");
                else if (GET_LEVEL(view) < STORYTELLER)
                    strcpy(levstr, "Blrd");
                else if (GET_LEVEL(view) < HIGHLORD)
                    strcpy(levstr, "Srtl");
                else if (GET_LEVEL(view) < OVERLORD)
                    strcpy(levstr, "Hghl");
                else
                    strcpy(levstr, "Ovrl");
              
              char hpmaxpad[4], hppad[4];
              char mamaxpad[4], mapad[4];
              char stmaxpad[4], stpad[4];
              char mvmaxpad[4], mvpad[4];
              if (GET_HIT(view) < 10)  strcpy(hppad, "  ");
              else if (GET_HIT(view) < 100)  strcpy(hppad, " ");
              else strcpy(hppad, "");
              if (GET_MAX_HIT(view) < 10)  strcpy(hpmaxpad, "  ");
              else if (GET_MAX_HIT(view) < 100)  strcpy(hpmaxpad, " ");
              else strcpy(hpmaxpad, "");

              if (GET_MANA(view) < 10)  strcpy(mapad, "  ");
              else if (GET_MANA(view) < 100)  strcpy(mapad, " ");
              else strcpy(mapad, "");
              if (GET_MAX_MANA(view) < 10)  strcpy(mamaxpad, "  ");
              else if (GET_MAX_MANA(view) < 100)  strcpy(mamaxpad, " ");
              else strcpy(mamaxpad, "");

              if (GET_STUN(view) < 10)  strcpy(stpad, "  ");
              else if (GET_STUN(view) < 100)  strcpy(stpad, " ");
              else strcpy(stpad, "");
              if (GET_MAX_STUN(view) < 10)  strcpy(stmaxpad, "  ");
              else if (GET_MAX_STUN(view) < 100)  strcpy(stmaxpad, " ");
              else strcpy(stmaxpad, "");

              if (GET_MOVE(view) < 10)  strcpy(mvpad, "  ");
              else if (GET_MOVE(view) < 100)  strcpy(mvpad, " ");
              else strcpy(mvpad, "");
              if (GET_MAX_MOVE(view) < 10)  strcpy(mvmaxpad, "  ");
              else if (GET_MAX_MOVE(view) < 100)  strcpy(mvmaxpad, " ");
              else strcpy(mvmaxpad, "");

   
              sprintf(buf, "[%s] [%s%d/%d%shp %s%d/%d%sma %s%d/%d%sst %s%d/%d%smv] %s (%s)",
                  levstr,
                  hppad, GET_HIT(view), GET_MAX_HIT(view), hpmaxpad, 
                  mapad, GET_MANA(view), GET_MAX_MANA(view), mamaxpad,
                  stpad, GET_STUN(view), GET_MAX_STUN(view), stmaxpad,
                  mvpad, GET_MOVE(view), GET_MAX_MOVE(view), mvmaxpad,
                  MSTR(view, name), d->player_info->name);

            } else if (show_pvitals) {
                if (GET_LEVEL(view) < LEGEND)
                    strcpy(levstr, "Mrtl");
                else if (GET_LEVEL(view) < BUILDER)
                    strcpy(levstr, "Lgnd");
                else if (GET_LEVEL(view) < STORYTELLER)
                    strcpy(levstr, "Bldr");
                else if (GET_LEVEL(view) < HIGHLORD)
                    strcpy(levstr, "Srtl");
                else if (GET_LEVEL(view) < OVERLORD)
                    strcpy(levstr, "Hghl");
                else
                    strcpy(levstr, "Ovrl");
                
                char hppad[4];
                char mapad[4];
                char stpad[4];
                char mvpad[4];
                if ((int)floor((double) GET_HIT(view)/GET_MAX_HIT(view) * 100) < 10 )
                     strcpy(hppad, "  ");
                else if ((int)floor((double) GET_HIT(view)/GET_MAX_HIT(view) * 100) < 100 )
                     strcpy(hppad, " ");
                else
                     strcpy(hppad, "");
                
                if ((int)floor((double) GET_MANA(view)/GET_MAX_MANA(view) * 100) < 10 )
                     strcpy(mapad, "  ");
                else if ((int)floor((double) GET_MANA(view)/GET_MAX_MANA(view) * 100) < 100 )
                     strcpy(mapad, " ");
                else
                     strcpy(mapad, "");                
                
                if ((int)floor((double) GET_STUN(view)/GET_MAX_STUN(view) * 100) < 10 )
                     strcpy(stpad, "  ");
                else if ((int)floor((double) GET_STUN(view)/GET_MAX_STUN(view) * 100) < 100 )
                     strcpy(stpad, " ");
                else
                     strcpy(stpad, "");                
                
                if ((int)floor((double) GET_MOVE(view)/GET_MAX_MOVE(view) * 100) < 10 )
                     strcpy(mvpad, "  ");
                else if ((int)floor((double) GET_MOVE(view)/GET_MAX_MOVE(view) * 100) < 100 )
                     strcpy(mvpad, " ");
                else
                     strcpy(mvpad, "");                
                 
                  sprintf(buf, "[%s] [%s%d%%hp %s%d%%ma %s%d%%st %s%d%%mv] %s (%s)",
                      levstr,
                      hppad, (int)floor((double) GET_HIT(view)/GET_MAX_HIT(view) * 100),
                      mapad, (int)floor((double) GET_MANA(view)/GET_MAX_MANA(view) * 100),
                      stpad, (int)floor((double) GET_STUN(view)/GET_MAX_STUN(view) * 100),
                      mvpad, (int)floor((double) GET_MOVE(view)/GET_MAX_MOVE(view) * 100),
                      MSTR(view, name),  d->player_info->name);
            
            } else if (show_skill != -1) {
                int raw, learned;

                sprintf(tmp, "(%s)", d->player_info->name);
                sprintf(buf, "%-12s %-12s", MSTR(view, name), tmp);

                strcat(buf, " - ");

                learned = get_skill_percentage(view, show_skill);
                raw = get_raw_skill(view, show_skill);
                if( learned == raw ) {
                    sprintf(tmp, "%3d%%    ", learned);
                } else {
                    sprintf(tmp, "%3d/%3d%%", learned, raw);
                }
                strcat(buf, tmp);

                if( skill[show_skill].sk_class == CLASS_MAGICK ) {
                    sprintf(tmp, " [%-4s]", Power[(int) view->skills[show_skill]->rel_lev]);
                    strcat( buf, tmp);
                }

                if (d->str)
                    strcat(buf, " [C]");

                if (view->specials.gone)
                    strcat(buf, " [G]");

                if (IS_SET(view->specials.act, CFL_NOWISH))
                    strcat(buf, " [N]");

                if (IS_SET(view->specials.act, CFL_FROZEN))
                    strcat(buf, " [F]");

                if (IS_SET(d->player_info->flags, PINFO_WANTS_RP_FEEDBACK))
                    strcat(buf, " [R]");
            } else if (show_objective) {
                sprintf(tmp, "(%s)", d->player_info->name);
                sprintf(buf, "%-12s %-12s", MSTR(view, name), tmp);

                strcat(buf, " - ");

                if (view->player.info[1])
                    strcat(buf, view->player.info[1]);

                if (d->str)
                    strcat(buf, " [C]");

                if (view->specials.gone)
                    strcat(buf, " [G]");

                if (IS_SET(view->specials.act, CFL_NOWISH))
                    strcat(buf, " [N]");

                if (IS_SET(view->specials.act, CFL_FROZEN))
                    strcat(buf, " [F]");

                if (IS_SET(d->player_info->flags, PINFO_WANTS_RP_FEEDBACK))
                    strcat(buf, " [R]");
            } else if (show_sdesc) {
                sprintf(tmp, "(%s)", d->player_info->name);
                sprintf(buf, "%-12s %-12s", MSTR(view, name), tmp);

                strcat(buf, " - ");

                strcat(buf, MSTR(view, short_descr));

                if (d->str)
                    strcat(buf, " [C]");

                if (view->specials.gone)
                    strcat(buf, " [G]");

                if (IS_SET(view->specials.act, CFL_NOWISH))
                    strcat(buf, " [N]");

                if (IS_SET(view->specials.act, CFL_FROZEN))
                    strcat(buf, " [F]");

                if (IS_SET(d->player_info->flags, PINFO_WANTS_RP_FEEDBACK))
                    strcat(buf, " [R]");

            } else {
                switch (GET_LEVEL(view)) {
                case MORTAL:
                    strcpy(levstr, "Mortal");
                    break;
                case LEGEND:
                    strcpy(levstr, "Legend");
                    break;
                case BUILDER:
                    strcpy(levstr, "Builder");
                    break;
                case STORYTELLER:
                    strcpy(levstr, "Storyteller");
                    break;
                case HIGHLORD:
                    strcpy(levstr, "Highlord");
                    break;
                case OVERLORD:
                    strcpy(levstr, "Overlord");
                    break;
                }

                /* User-specific titles */
                if (!strcmp(GET_NAME(view), "Sanvean"))
                    strcpy(levstr, "Overchick");

                if (!strcmp(GET_NAME(view), "Laeris"))
                    strcpy(levstr, "Cynic");

                if (!strcmp(GET_NAME(view), "Morgenes"))
                    strcpy(levstr, "Codelord");

                if (!strcmp(GET_NAME(view), "Tiernan"))
                    strcpy(levstr, "Timelord");

                if (!strcmp(GET_NAME(view), "Halaster"))
                    strcpy(levstr, "Deathlord");

                if (!strcmp(GET_NAME(view), "Tlaloc"))
                    strcpy(levstr, "Texan");

                if (!strcmp(GET_NAME(view), "Savak"))
                    strcpy(levstr, "Dancin' Fool");

                /* if( !strcmp( GET_NAME( view ), "Eris" ) )
                 * strcpy (levstr, " Stowytewwer "); */

                if (!strcmp(GET_NAME(view), "Saikun"))
                    strcpy(levstr, "FunkLord");

                if (!strcmp(GET_NAME(view), "Xygax"))
                    strcpy(levstr, "Shadowlord");

                if (!strcmp(GET_NAME(view), "Mekeda"))
                    strcpy(levstr, "SpiceLord");

                if (has_extra_cmd(view->granted_commands, CMD_CSET,
					HIGHLORD, TRUE))
                    get_char_extra_desc_value(view, "[RANK_TEXT]", levstr, sizeof(levstr));

                if (GET_LEVEL(view) >= HIGHLORD)
                    get_char_extra_desc_value(view, "[RANK_TEXT]", levstr, sizeof(levstr));

                levstr[13] = '\0';

                /* only show simple clan format to morts & low imms */
                if (clan && GET_LEVEL(viewer) < STORYTELLER) {
                    if (can_use_psionics(ch) && can_use_psionics(view)) {
                        /* Allanaki Militia, Conclave, Kuraci Special Ops */
                        if (show_clan == 12 || show_clan == 14 || show_clan == 62)
                            sprintf(buf, "%s", MSTR(view, short_descr));
                        else
                            sprintf(buf, "%s", MSTR(view, name));
                    }
                } else {
                    int pad = (14 - strlen(levstr)) / 2;

                    /* For who z listing, show the room # also */
                    if ((show_zone || show_city) && view->in_room) {
                        sprintf(tmp, "%s] [%5d", levstr, view->in_room->number);
                        strcpy(levstr, tmp);
                    }

                    sprintf(buf, "[%*s%-*s] [%s/%s] [%s] %s (%s)", pad, " ", 14 - pad, levstr,
                            GET_GUILD(view) ? guild[(int) GET_GUILD(view)].abbrev : "---",
                            GET_SUB_GUILD(view) ? sub_guild[(int) GET_SUB_GUILD(view)].
                            abbrev : "---", race[(int) GET_RACE(view)].abbrev, MSTR(view, name),
                            d->player_info->name);

                    /* For who n listing, show the # chars on acct */
                    if (show_newbie) {
                        sprintf(tmp, " [%d]", d->player_info->num_chars);
                        strcat(buf, tmp);
                    }

                    /* For who b listing, show the # biographies on char */
                    if (show_biographies) {
                        sprintf(tmp, " [%d]", charbios);
                        strcat(buf, tmp);
                    }
                }

                /* immortals can see all clans -Tenebrius 
                 * if( clan == 0 && IS_IMMORTAL( ch ) )
                 */
                if (IS_IMMORTAL(viewer)) {
                    tmp_clan = show_clan;
                    show_clan = GET_TRIBE(view);
                }

                if ((show_clan > 0) && (show_clan < MAX_CLAN)) {
                    if (IS_TRIBE_LEADER(view, show_clan))
                        sprintf(buf2, " [*%s*]", clan_table[show_clan].name);
                    else
                        sprintf(buf2, " [%s]", clan_table[show_clan].name);
                    strcat(buf, buf2);
                }

                /* immortals can see all clans -Tenebrius  */
                if (IS_IMMORTAL(viewer)) {
                    show_clan = tmp_clan;

                    if (view->specials.il) {
                        sprintf(buf2, " [I %d]", view->specials.il);
                        strcat(buf, buf2);
                    }
                }

                if (d->str)
                    strcat(buf, " [C]");

                if (view->specials.gone)
                    strcat(buf, " [G]");

                if (IS_IMMORTAL(viewer)) {
                    if (view->specials.quiet_level != 0) {
                        strcat(buf, " [");
                        sprint_flag(view->specials.quiet_level, quiet_flag, buf2);
                        strcat(buf, buf2);
                        strcat(buf, " ]");
                    }

                    if (d->original == view)
                        sprintf(buf + strlen(buf), " [S %d]", npc_index[d->character->nr].vnum);

                    if (IS_SET(view->specials.act, CFL_NOWISH))
                        strcat(buf, " [N]");

                    if (IS_SET(view->specials.act, CFL_FROZEN))
                        strcat(buf, " [F]");

                    /* does the account want feedback? */
                    if (IS_SET(d->player_info->flags, PINFO_WANTS_RP_FEEDBACK))
                        strcat(buf, " [R]");

                    if (!IS_IMMORTAL(view)) {
                        /* d->player_info below was view->desc->player_info
                         * but it is possible that view->desc is null
                         * change back if this is wrong -Morg */
                        if (d->player_info->karma > 0) {
                            sprintf(buf2, " [%d k]", d->player_info->karma);
                            strcat(buf, buf2);
                        }
                    }
                }
            }

            strcat(buf, "\n\r");
            if (!clan || (clan && (can_use_psionics(view) || IS_IMMORTAL(viewer)))
                || show_objective)
                strcat(show_list, buf);
        }
    }

    sprintf(buf, "\n\rThere are %d visible %s currently in %s.\n\r", plrs,
            ((IS_IMMORTAL(viewer) && lev < 1)
             || clan) ? "players" : "Immortals",
	    (IS_IMMORTAL(viewer) && show_city) ?
	    (ct ? city[ct].name : "the wilderness") : "the world");
    strcat(show_list, buf);

    if (!IS_IMMORTAL(viewer) && !clan) {
        for (d = descriptor_list, plrs = 0; d; d = d->next)
            if (d->character && !IS_IMMORTAL(d->character)
                && !IS_NPC(d->character)
                /* && CAN_SEE (ch, d->character) -- plyrs get full count -Morg */
                && d->character != ch)
                plrs++;

        sprintf(buf, "\n\rThere are %d players currently in the world, " "other than yourself.\n\r",
                plrs);
        strcat(show_list, buf);
    }

    page_string(ch->desc, show_list, 1);
}

void
gather_who_stats()
{
    DESCRIPTOR_DATA *d;
    char buf[MAX_STRING_LENGTH];
    char clanname[MAX_STRING_LENGTH];
    char aname[MAX_STRING_LENGTH];

    extern void who_log(char *);
    extern void who_log_line(char *);

    who_log("Who");
    for (d = descriptor_list; d; d = d->next)
        if (d->character && !IS_NPC(d->character)) {
            sprintf(aname, "(%s)", d->player_info->name);

            if (d->character->clan)
                if (d->character->clan->clan <= MAX_CLAN)
                    sprintf (clanname, "%s", clan_table[d->character->clan->clan].name);
                else
                    sprintf (clanname, "%s", "Unknown");
            else
                sprintf (clanname, "%s", " ");

            sprintf(buf, "%-12s %-14s [%s] [%s] [%s]", MSTR(d->character, name), aname,
                    guild[(int) GET_GUILD(d->character)].abbrev,
                    race[(int) GET_RACE(d->character)].abbrev,
                    clanname);
            who_log_line(buf);
        }

    who_log("Where");
    for (d = descriptor_list; d; d = d->next)
        if (d->character && !IS_NPC(d->character) && d->character->in_room) {
            sprintf(aname, "(%s)", d->player_info->name);
            sprintf(buf, "%-12s %-14s [%5d] %s", MSTR(d->character, name), aname,
                    d->character->in_room->number, d->character->in_room->name);
            who_log_line(buf);
        }
}

void
gather_population_stats()
{
    DESCRIPTOR_DATA *d;
    int players = 0, immortals = 0;
    char buf[MAX_STRING_LENGTH];
    struct timeval start;

    extern void stat_log(char *);

    perf_enter(&start, "gather_population_stats()");
    for (d = descriptor_list; d; d = d->next)
        if (d->character && !IS_NPC(d->character)) {
            if (IS_IMMORTAL(d->character))
                immortals++;
            else
                players++;
        }

    global_game_stats.player_count = players;
    global_game_stats.staff_count = immortals;
    sprintf(buf, "P=%3d I=%3d", players, immortals);
    stat_log(buf);
    perf_exit("gather_population_stats()", start);
}

void
cmd_dwho(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    struct descriptor_data *d;
    CHAR_DATA *view;
    int lev, plrs;
    char buf[256], tmp[256], show_list[MAX_STRING_LENGTH];
    char levstr[256];
    char dbuf[256];
    sh_int switched_imm = IS_SWITCHED_IMMORTAL(ch);

    strcpy(show_list, "");

    argument = one_argument(argument, tmp, sizeof(tmp));

    lev = atoi(tmp);
    plrs = 0;

    if (switched_imm)
        send_to_char("Players\n\r-------\n\r", ch);
    else
        send_to_char("Immortals\n\r----------\n\r", ch);

    for (d = descriptor_list; d; d = d->next) {
        if (!d->connected) {
            view = 0;
            if (d->original) {
                if (CAN_SEE(ch, d->original) && (GET_LEVEL(d->original) >= lev)
                    && (switched_imm || IS_IMMORTAL(d->original)))
                    view = d->original;
            } else if (CAN_SEE(ch, d->character)
                       && (GET_LEVEL(d->character) >= lev)
                       && (switched_imm || IS_IMMORTAL(d->character)))
                view = d->character;

            if (view) {
                plrs++;
                if (GET_LEVEL(view) < LEGEND)
                    strcpy(levstr, "Mrtl");
                else if (GET_LEVEL(view) < BUILDER)
                    strcpy(levstr, "Lgnd");
                else if (GET_LEVEL(view) < STORYTELLER)
                    strcpy(levstr, "Bldr");
                else if (GET_LEVEL(view) < HIGHLORD)
                    strcpy(levstr, "Srtl");
                else if (GET_LEVEL(view) < OVERLORD)
                    strcpy(levstr, "Hghl");
                else
                    strcpy(levstr, "Ovrl");

                if (*tmp == 'v') {
                     sprintf(buf, "[%s] [%d/%dhp %d/%dst %d/%dma %d/%dmv] %s (%s)\n\r",
                             levstr, 
                             GET_HIT(view), GET_MAX_HIT(view), GET_STUN(view), GET_MAX_STUN(view),
                             GET_MANA(view), GET_MAX_MANA(view), GET_MOVE(view), GET_MAX_MOVE(view), 
                             MSTR(view, name), d->player_info->name);
                     strcpy(dbuf, buf);
                } else if (*tmp == 'p'){
                             sprintf(buf, "[%s] [%d%%hp %d%%st %d%%ma %d%%mv] %s (%s)\n\r",
                             levstr,
                             (int)floor((double) GET_HIT(view)/GET_MAX_HIT(view) * 100),
                             (int)floor((double) GET_STUN(view)/GET_MAX_STUN(view) * 100),
                             (int)floor((double) GET_MANA(view)/GET_MAX_MANA(view) * 100),
                             (int)floor((double) GET_MOVE(view)/GET_MAX_MOVE(view) * 100),
                             MSTR(view, name),  d->player_info->name);
                     strcpy(dbuf, buf);


                } else { 
                    sprintf(buf, "[%s] [%s] [%s] %s", levstr, guild[(int) GET_GUILD(view)].abbrev,
                            race[(int) GET_RACE(view)].abbrev, MSTR(view, name));
                    sprintf(dbuf, "%-38s", buf);
                    if (!(plrs % 2))
                        strcat(dbuf, "\n\r");
                }
                strcat(show_list, dbuf);
            }
        }
    }

    if (plrs % 2)
        strcat(show_list, "\n\r");
    sprintf(buf, "\n\rThere are %d visible %s currently in the world.\n\r", plrs,
            IS_IMMORTAL(ch) ? "players" : "Immortals");
    strcat(show_list, buf);

    if (!IS_IMMORTAL(ch)) {
        for (d = descriptor_list, plrs = 0; d; d = d->next)
            if (d->character && !IS_IMMORTAL(d->character) && !IS_NPC(d->character)
                && CAN_SEE(ch, d->character) && (d->character != ch))
                plrs++;
        sprintf(buf, "\n\rThere are %d players currently in the world, " "other than yourself.\n\r",
                plrs);
        strcat(show_list, buf);
    }

    page_string(ch->desc, show_list, 1);
}

void
cmd_users(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_STRING_LENGTH], line[200], host_format[80];
    char tmp[MAX_STRING_LENGTH];
    struct descriptor_data *d;
    int cnt = 0, len_types;
    int level_limit = 0;

    while (isspace(*argument))
        argument++;             // Skip spaces
    if (isdigit(*argument)) {
        level_limit = atoi(argument);
        argument = one_argument(argument, tmp, sizeof(tmp));
    }

    argument = one_argument(argument, host_format, sizeof(host_format));

    for (len_types = 0; *connected_types[len_types] != '\n'; len_types++) {     /* do nothing count len_types up to the max */
    };

    strcpy(buf, "Connections\n\r-----------\n\r");

    for (d = descriptor_list; d; d = d->next) {
        if (level_limit > 0) {
            if (!d->original && !d->character)
                continue;

            // Don't show people below the level limit
            if ((d->original && GET_LEVEL(d->original) < level_limit)
                || (d->character && GET_LEVEL(d->character) < level_limit))
                continue;
        }

        if (d->character && MSTR(d->character, name)) {
            if (d->original)
                sprintf(line, "%-15s ", MSTR(d->original, name));
            else
                sprintf(line, "%-15s ", MSTR(d->character, name));
        } else
            sprintf(line, "%-15s ", "(Unknown)");

        if (d->player_info && d->player_info->name) {
            sprintf(tmp, "(%s)", d->player_info->name);
            sprintf(line + strlen(line), "%-17s ", tmp);
        } else
            sprintf(line + strlen(line), "%-17s ", "(Unknown)");

        sprintf(line + strlen(line), "[%2d] ", d->descriptor);

        if ((d->connected < len_types) && (d->connected >= 0))
            sprintf(line + strlen(line), "[%s] ", connected_types[d->connected]);
        else
            sprintf(line + strlen(line), "[--Invalid Type %3d--] ", d->connected);

        if (d->host) {
            if ((strcmp(host_format, "site") == 0) || (strcmp(host_format, "-s") == 0)) {
                sprintf(line + strlen(line), "[%-10s]\n\r", d->hostname);
            } else
                sprintf(line + strlen(line), "[%s]\n\r", d->host);
        } else
            strcat(line, "[Unknown Host]\n\r");


        if (GET_LEVEL(ch) > HIGHLORD) { /* patch so overlords do not have to make any
                                         * kind of check */
            cnt++;
            strcat(buf, line);
        } else {
            if (!d->character || CAN_SEE(ch, d->character)) {
                cnt++;
                strcat(buf, line);
            }
        }
    }
    send_to_char(buf, ch);

    sprintf(buf, "\n\rThere are %d visible users connected to the game.\n\r", cnt);
    send_to_char(buf, ch);
}


void
cmd_inventory(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    OBJ_DATA *i, *ii;
    bool found;
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char *tmpstr;
    int shown_count;
    int index;

    found = FALSE;

#ifdef NESS_MONEY
#else
    /* quick search for money--if found recombine it with total cash */
    for (i = ch->carrying; i; i = ii) {
        ii = i->next_content;
        if (i->obj_flags.type == ITEM_MONEY)
            money_mgr(MM_OBJ2CH, 0, (void *) i, (void *) ch);
    }
#endif

    for (i = ch->carrying; i; i = i->next_content) {
        if (CAN_SEE_OBJ(ch, i)) {
            found = TRUE;
            break;
        }
    }

    buf2[0] = '\0';
    sprintf(buf2, "You are carrying");

    if (IS_SET(ch->specials.brief, BRIEF_EQUIP)) {
        string_safe_cat(buf2, " ", MAX_STRING_LENGTH);
    } else {
        string_safe_cat(buf2, ":\n\r", MAX_STRING_LENGTH);
    }

    if (GET_OBSIDIAN(ch)) {
        sprintf(buf, "%d obsidian pieces", GET_OBSIDIAN(ch));
        found = TRUE;
        string_safe_cat(buf2, buf, MAX_STRING_LENGTH);
        if (IS_SET(ch->specials.brief, BRIEF_EQUIP) && ch->carrying) {
            string_safe_cat(buf2, ", ", MAX_STRING_LENGTH);

        } else if (!IS_SET(ch->specials.brief, BRIEF_EQUIP)) {
            string_safe_cat(buf2, "\n\r", MAX_STRING_LENGTH);
        }
    }

    sprintf(buf, "%s", list_obj_to_char(ch->carrying, ch, 2, FALSE));

    for (index = 0, shown_count = 1; index < strlen(buf); index++)
        if (buf[index] == ',' )
            shown_count++;

    if (IS_SET(ch->specials.brief, BRIEF_EQUIP)
     && GET_OBSIDIAN(ch)
     && strlen(buf) > 0 && shown_count < 2) {
        string_safe_cat(buf2, "and ", MAX_STRING_LENGTH);
    }

    string_safe_cat(buf2, buf, MAX_STRING_LENGTH);

    if (IS_SET(ch->specials.brief, BRIEF_EQUIP)) {
        string_safe_cat(buf2, ".", MAX_STRING_LENGTH);
        tmpstr = strdup(buf2);
        tmpstr = format_string(tmpstr);
        cprintf(ch, "%s", tmpstr);
        free(tmpstr);
    } else {
        cprintf(ch, "%s\n\r", buf2);
    }

    if (!found) {
        sprintf(buf, "nothing.\n\r");
        send_to_char(buf, ch);
    }
}


void
cmd_equipment(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int j;
    bool found;
    /*
     * static int sorted_eq[MAX_WEAR] =
     * {
     * 6, 20, 21, 3, 4, 5, 10, 14, 15, 9, 16, 17, 19, 1, 2, 0, 11, 18, 12, 13, 7,
     * 8, 22, 23, 24, 25
     * };
     */

    send_to_char("You are using:\n\r", ch);
    found = FALSE;
    for (j = 0; j < MAX_WEAR; j++) {
        if (ch->equipment[sorted_eq[j]]) {
            if (CAN_SEE_OBJ(ch, ch->equipment[sorted_eq[j]])) {
                send_to_char(where[sorted_eq[j]], ch);
                show_obj_to_char(ch->equipment[sorted_eq[j]], ch, 1);
                found = TRUE;
            } else {
                send_to_char(where[sorted_eq[j]], ch);
                send_to_char("Something.\n\r", ch);
                found = TRUE;
            }
        }
    }
    if (!found) {
        send_to_char(" Nothing.\n\r", ch);
    }
}


void
cmd_credits(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    page_string(ch->desc, credits, 0);
}

void
cmd_areas(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    page_string(ch->desc, areas, 0);
}

void
cmd_news(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    page_string(ch->desc, news, 0);
}

void
cmd_motd(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    page_string(ch->desc, motd, 0);
}

void
cmd_wizlist(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    page_string(ch->desc, wizlist, 0);
}

bool check_char_name(CHAR_DATA * ch, CHAR_DATA * tar_ch, const char *name);

void
cmd_where(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char name[MAX_INPUT_LENGTH], buf[MAX_STRING_LENGTH];
    char show_list[MAX_STRING_LENGTH], next_tmp_name[MAX_STRING_LENGTH];
    char rname_buf[256], real_name_buf[512];
    char old_args[MAX_INPUT_LENGTH];
    int len, rnumber;
    char zname[256];
    int pcount, ocount;

    /* stuff for the special/dmpl searcher */
    CHAR_DATA *tmp_char;
    OBJ_DATA *tmp_obj;

    int cindex = 0;
    char dmplname[MAX_STRING_LENGTH], tmp_arg[MAX_STRING_LENGTH] = "";
    char fn[MAX_STRING_LENGTH] = "";
    char tmp[MAX_STRING_LENGTH];
    FILE *fp;

    register CHAR_DATA *i;
    register OBJ_DATA *k;
    struct descriptor_data *d;
    int c = 0, zone = 0, vnum;
    CLAN_DATA *clan;

    /* Pointer to allow recursion for objects */
    OBJ_DATA *z;

    argument = one_argument(argument, name, sizeof(name));
    strcpy(old_args, argument);
    argument = one_argument(argument, zname, sizeof(zname));

    strcpy(show_list, "");
    len = 0;
    if (!*name) {
        strcat(show_list, "Players\n\r-------\n\r");

        for (d = descriptor_list; d; d = d->next) {
            CHAR_DATA *to_show = d->original ? d->original : d->character;

            if (to_show && (d->connected == CON_PLYNG) && CAN_SEE(ch, to_show)) {
                c++;
                sprintf(tmp, "(%s)", d->player_info->name);
                if (d->original)
                    sprintf(buf, "%-15s %-17s [%5d] %s\n\r", MSTR(d->original, name), tmp,
                            d->original->in_room->number, d->original->in_room->name);
                else
                    sprintf(buf, "%-15s %-17s [%5d] %s\n\r", MSTR(d->character, name), tmp,
                            d->character->in_room->number, d->character->in_room->name);
                strcat(show_list, buf);
            }
        }
        sprintf(buf, "\n\rThere are %d players currently in " "the game.\n\r", c);
        strcat(show_list, buf);
        page_string(ch->desc, show_list, 1);
        return;
    }

    /* SPECIAL/DMPL searcher hack -Tenebrius */
    if (!strcmp(name, "*")) {
        if (!zname[0]) {
            send_to_char("where * <coded or dmplname>\n\r", ch);
        } else if (-1 == (cindex = coded_name(zname))) {
            strcpy(dmplname, zname);
            if (*dmplname == '\0') {
                send_to_char("where * <coded or dmplname>\n\r", ch);
                return;
            };

            sprintf(fn, "/home/mud/armag/lib/dmpl/%s", dmplname);

            if (!(fp = fopen(fn, "r"))) {
                send_to_char("That is not a special or a dmpl\n\r", ch);
                send_to_char(fn, ch);
                send_to_char("\n\r", ch);
                return;
            };
            fclose(fp);
        };

        strcpy(show_list, "Ok, running the test program\n\r----------------\n\r");
        for (c = 0; c < top_of_obj_t; c++) {
            tmp_obj = read_object(c, REAL);
            if (((cindex != -1)
                 && (coded_specials_list[cindex].function(0, 0, tmp_arg, 0, 0, tmp_obj)))
                || ((cindex == -1) && (start_obj_prog(0, tmp_obj, 0, tmp_arg, dmplname)))) {
                sprintf(buf, "object %d\n\r", obj_index[c].vnum);
                if ((len + strlen(buf)) > MAX_STRING_LENGTH) {
                    send_to_char("list too long", ch);
                    return;
                }
                len += strlen(buf);
                strcat(show_list, buf);
            }
            extract_obj(tmp_obj);
        };

        for (c = 0; c < top_of_npc_t; c++) {
            tmp_char = read_mobile(c, REAL);
            if (((cindex != -1)
                 && (coded_specials_list[cindex].function(0, 0, tmp_arg, tmp_char, 0, 0)))
                || ((cindex == -1) && (start_npc_prog(0, tmp_char, 0, tmp_arg, dmplname)))) {
                sprintf(buf, "character %d\n\r", npc_index[c].vnum);
                if ((len + strlen(buf)) > MAX_STRING_LENGTH) {
                    send_to_char("list too long", ch);
                    return;
                }
                len += strlen(buf);
                strcat(show_list, buf);
            }
            extract_char(tmp_char);
        };
        if (!*show_list)
            send_to_char("Couldn't find any such thing.\n\r", ch);
        else
            page_string(ch->desc, show_list, 1);

        return;
    }

    *buf = '\0';
    zone = -1;
    if (!strcmp(name, "o") || !strcmp(name, "O")) {
        if (is_number(zname))
            vnum = atoi(zname);
        else {
            vnum = -1;
            strcpy(zname, old_args);
        }

        for (k = object_list; k; k = k->next) {
            if (vnum == obj_index[k->nr].vnum
                || (vnum == -1 && isallnamebracketsok(zname, OSTR(k, name)))) {

                if (k->in_room) {
                    strcpy(rname_buf, k->in_room->name);
                    rnumber = k->in_room->number;
                    sprintf(buf, "%-20s - %s [%d]\n\r", OSTR(k, short_descr), rname_buf, rnumber);
                } else {
                    char held_by[MAX_STRING_LENGTH];
                    char buf2[MAX_STRING_LENGTH];

                    strcpy(rname_buf, "");
                    rnumber = -1;
                    if (k->carried_by) {
                        strcpy(held_by, REAL_NAME(k->carried_by));
                        if (k->carried_by->in_room) {
                            strcpy(rname_buf, k->carried_by->in_room->name);
                            rnumber = k->carried_by->in_room->number;
                        }
                    } else if (k->equipped_by) {
                        strcpy(held_by, REAL_NAME(k->equipped_by));
                        if (k->equipped_by->in_room) {
                            strcpy(rname_buf, k->equipped_by->in_room->name);
                            rnumber = k->equipped_by->in_room->number;
                        }
                    } else if (k->in_obj) {
                        strcpy(held_by, OSTR(k->in_obj, short_descr));
                        // Begin "recursive" search
                        z = (k->in_obj);
                        rname_buf[0] = 0;

                        while (z) {
                            if (z->in_room) {
                                strcat(rname_buf, z->in_room->name);
                                rnumber = z->in_room->number;
                            } else if (z->carried_by) {
                                sprintf(rname_buf, "%s", REAL_NAME(z->carried_by));
                                if (z->carried_by->in_room) {
                                    sprintf(buf2, " - %s", z->carried_by->in_room->name);
                                    strcat(rname_buf, buf2);
                                    rnumber = z->carried_by->in_room->number;
                                }
                            } else if (z->equipped_by) {
                                sprintf(rname_buf, "%s", REAL_NAME(z->equipped_by));
                                if (z->equipped_by->in_room) {
                                    sprintf(buf2, " - %s", z->equipped_by->in_room->name);
                                    strcat(rname_buf, buf2);
                                    rnumber = z->equipped_by->in_room->number;
                                }
                            } else if (z->in_obj) {
                                sprintf(rname_buf, "%s - ", OSTR(z->in_obj, short_descr));
                            }
                            z = z->in_obj;
                        }
                    } else if (k->table) {
                        strcpy(held_by, OSTR(k->table, short_descr));

                        if (k->table->in_room) {
                            strcpy(rname_buf, k->table->in_room->name);
                            rnumber = k->table->in_room->number;
                        }
                    } else
                        strcpy(held_by, "Somewhere");

                    sprintf(buf, "%-20s - %s", OSTR(k, short_descr), held_by);

                    if (rname_buf[0] != '\0')
                        sprintf(held_by, " - %s [%d]\n\r", rname_buf, rnumber);
                    else
                        sprintf(held_by, "\n\r");

                    strcat(buf, held_by);
                }

                if ((len + strlen(buf)) > MAX_STRING_LENGTH) {
                    send_to_char("list too long", ch);
                    return;
                }
                len += strlen(buf);
                strcat(show_list, buf);
            }                   /*  object match */
        }                       /*  k = obj; k ; k = obj->next */

        if (!*show_list)
            send_to_char("Couldn't find any such thing.\n\r", ch);
        else
            page_string(ch->desc, show_list, 1);
        return;

    }
    /* if name[0] == 'o' */
    if (!strcmp(name, "n") || !strcmp(name, "N")) {
        if (is_number(zname))
            vnum = atoi(zname);
        else {
            vnum = -1;
            strcpy(zname, old_args);
        }

        for (i = character_list; i; i = i->next) {
            if (vnum == npc_index[i->nr].vnum
                || (vnum == -1 && isallnamebracketsok(zname, MSTR(i, name)))) {
                {
                    if (IS_NPC(i))
                        sprintf(buf, "%-22s - %s[%d]\n\r", MSTR(i, short_descr),
                                (i->in_room ? i->in_room->name : "ERROR - NO ROOM"),
                                (i->in_room ? i->in_room->number : -1));
                    /* if they're not an npc, make sure they're visible -Morg */
                    else if (char_can_see_char(ch, i))
                        sprintf(buf, "%-22s - %s[%d]\n\r", MSTR(i, name),
                                (i->in_room ? i->in_room->name : "ERROR - NO ROOM"),
                                (i->in_room ? i->in_room->number : -1));
                }

                if ((len + strlen(buf)) > MAX_STRING_LENGTH) {
                    send_to_char("list too long", ch);
                    return;
                }
                len += strlen(buf);
                strcat(show_list, buf);
            }                   /*  vnum == npc number */
        }                       /*  i = npc; i ; i = i->next */

        if (!*show_list)
            send_to_char("Couldn't find any such thing.\n\r", ch);
        else
            page_string(ch->desc, show_list, 1);
        return;
    }
    /* if name[0] == 'n' */
    if (!strcmp(name, "i") || !strcmp(name, "I")) {
        vnum = 0;
        if (!(*zname))
            strcpy(zname, "1");
        if (is_number(zname))
            vnum = atoi(zname);
        else
            vnum = 1;

        if ((vnum < 1) || (vnum > OVERLORD)) {
            gamelog("Invalid level search.  Defaulting to level 1.");
            vnum = 1;
        }

        sprintf(buf, "*** Searching for immortals of level %d(%s) ***\n\r", vnum, level_name[vnum]);
        send_to_char(buf, ch);

#define SCREWY_WHERE 1
#ifdef SCREWY_WHERE
        for (i = character_list; i; i = i->next) {
            if (!IS_NPC(i) && (GET_LEVEL(i) >= vnum)) {
                strcpy(buf, "");
                {
                    if (IS_NPC(i))
                        sprintf(buf, "%-22s - %s[%d]\n\r", MSTR(i, short_descr), i->in_room->name,
                                i->in_room->number);
                    /* if they're not an npc, make sure they're visible -Morg */
                    else if (char_can_see_char(ch, i) && i->desc)
                        sprintf(buf, "%-22s - %s[%d]\n\r", MSTR(i, name), i->in_room->name,
                                i->in_room->number);
                }

                if ((len + strlen(buf)) > MAX_STRING_LENGTH) {
                    send_to_char("list too long", ch);
                    return;
                }
                len += strlen(buf);
                strcat(show_list, buf);
            }                   /*  vnum == npc number */
        }                       /*  i = npc; i ; i = i->next */
#else
        for (i = character_list; i; i = i->next) {
            if (!IS_NPC(i) && (GET_LEVEL(i) >= vnum)) {
                {
                    if (IS_NPC(i))
                        sprintf(buf, "%-22s - %s[%d]\n\r", MSTR(i, short_descr), i->in_room->name,
                                i->in_room->number);
                    /* if they're not an npc, make sure they're visible -Morg */
                    else if (char_can_see_char(ch, i) && i->desc)
                        sprintf(buf, "%-22s - %s[%d]\n\r", MSTR(i, name), i->in_room->name,
                                i->in_room->number);
                }

                if ((len + strlen(buf)) > MAX_STRING_LENGTH) {
                    send_to_char("list too long", ch);
                    return;
                }
                len += strlen(buf);
                strcat(show_list, buf);
            }                   /*  vnum == npc number */
        }                       /*  i = npc; i ; i = i->next */
#endif
        if (!*show_list)
            send_to_char("Couldn't find any immortals of that level.\n\r", ch);
        else
            page_string(ch->desc, show_list, 1);
        return;
    }
    /* if name[0] == 'i' */
#define AZROEN_WHERE_C 1
#ifdef AZROEN_WHERE_C
    if (!strcmp(name, "c") || !strcmp(name, "C")) {
        pcount = 0;

        if (!*zname) {
            send_to_char("Syntax: 'where c <-p> (clan # OR clan name)'\n\r", ch);
            return;
        }
        if (!strcmp(zname, "-p") || !strcmp(zname, "-P")) {
            pcount = 1;
            strcpy(buf, argument);      /* remove -p from old_args */
            argument = one_argument(argument, zname, sizeof(zname));
        } else if (!strcmp(zname, "-n") || !strcmp(zname, "-N")) {
            pcount = 2;
            strcpy(buf, argument);      /* remove -n from old_args */
            argument = one_argument(argument, zname, sizeof(zname));
        }
        if (!*zname) {
            send_to_char("Syntax: 'where c <-p> (clan # OR clan name)'\n\r", ch);
            return;
        }

        if (is_number(zname))
            vnum = atoi(zname);
        else 
            vnum = lookup_clan_by_name(old_args);

        sprintf(buf, "*** Searching for Tribe %d(%s)%s ***\n\r", vnum, 
         ((vnum > 0) && (vnum < MAX_CLAN)) ? clan_table[vnum].name : old_args,
         (pcount == 1) ? " -- Players only" : ((pcount == 2)
          ? " -- NPC's only" : " -- Players and NPC's"));
        send_to_char(buf, ch);

        if ((vnum <= 0) || vnum >= MAX_CLAN ) {
            send_to_char("That tribe does not exist.\n\r", ch);
            return;
        }

        for (i = character_list; i; i = i->next) {
            if (!((!IS_NPC(i) && (pcount == 2))
                  || (IS_NPC(i) && (pcount == 1)))) {
                for (clan = i->clan; clan; clan = clan->next) {
                    int cn = clan->clan;
                    if (vnum == cn) {
                        {
                            if (IS_NPC(i))
                                sprintf(buf, "%-22s - %s[%d]\n\r", MSTR(i, short_descr),
                                        i->in_room->name, i->in_room->number);
                            /* if they're not an npc, make sure they're visible -Morg */
                            else if (char_can_see_char(ch, i))
                                sprintf(buf, "%-22s - %s[%d]\n\r", MSTR(i, name), i->in_room->name,
                                        i->in_room->number);
                        }

                        if ((len + strlen(buf)) > MAX_STRING_LENGTH) {
                            send_to_char("list too long", ch);
                            return;
                        }
                        len += strlen(buf);
                        strcat(show_list, buf);
                    }           /*  vnum == npc number */
                }               /*  for clan list */
            }                   /*  ! (is_npc(i) && pcount) */
        }                       /*  i = npc; i ; i = i->next */

        if (!*show_list)
            send_to_char("Couldn't find anyone in that tribe.\n\r", ch);
        else
            page_string(ch->desc, show_list, 1);
        return;
    }                           /* if name[0] == 'c' */
#endif

    *buf = '\0';
    zone = -1;
    if (*zname)
        zone = atoi(zname);
    pcount = 1;
    /* while (sprintf (next_tmp_name, "%d.%s", pcount, name) &&
     * (i = get_char_vis (ch, next_tmp_name))) */
    /* look for chars in same room, count them first */
    for (i = ch->in_room->people; i; i = i->next_in_room) {
        if (check_char_name(ch, i, name)) {
            sprintf(next_tmp_name, "%d.%s", pcount, name);
            if ((zone == -1) || (zone == i->in_room->zone)) {
                if (IS_NPC(i))
                    sprintf(buf, "(%s) %-22s - %s[%d]\n\r", next_tmp_name, MSTR(i, short_descr),
                            i->in_room->name, i->in_room->number);
                else
                    sprintf(buf, "(%s) %-22s - %s[%d]\n\r", next_tmp_name, MSTR(i, name),
                            i->in_room->name, i->in_room->number);
                if ((len + strlen(buf)) > MAX_STRING_LENGTH) {
                    send_to_char("list too long", ch);
                    return;
                }
                len += strlen(buf);
                strcat(show_list, buf);
            }
            ++pcount;
        }
    }

    /* now search all characters, and drop those that are in the same room */
    for (i = character_list; i; i = i->next) {
        if (check_char_name(ch, i, name) && i->in_room != ch->in_room) {
            sprintf(next_tmp_name, "%d.%s", pcount, name);

#define TENE_FIX_WHERE
#ifdef TENE_FIX_WHERE
            /* delete this #ifdef if is still working after 3/9/2002 */
            if ((zone == -1) || (i->in_room && (zone == i->in_room->zone)))
#else
            if ((zone == -1) || (zone == i->in_room->zone))
#endif
            {
                if (IS_NPC(i) && i->in_room)
                    sprintf(buf, "(%s) %-22s - %s[%d]\n\r", next_tmp_name, MSTR(i, short_descr),
                            i->in_room->name, i->in_room->number);
                else if (i->in_room)
                    sprintf(buf, "(%s) %-22s - %s[%d]\n\r", next_tmp_name, MSTR(i, name),
                            i->in_room->name, i->in_room->number);
                if ((len + strlen(buf)) > MAX_STRING_LENGTH) {
                    send_to_char("list too long", ch);
                    return;
                }
                len += strlen(buf);
                strcat(show_list, buf);
            }
            ++pcount;
        }
    }

    ocount = 1;
    /* while (sprintf (next_tmp_name, "%d.%s", ocount, name) &&
     * (k = get_obj_vis (ch, next_tmp_name))) */

    /* search the character for the object */
    for (k = ch->carrying; k; k = k->next_content) {
        if (isname(name, OSTR(k, name)) && CAN_SEE_OBJ(ch, k)) {
            sprintf(next_tmp_name, "%d.%s", ocount, name);
            if ((zone == -1) || (k->in_room && (zone == k->in_room->zone))) {
                if (k->in_room) {
                    strcpy(rname_buf, k->in_room->name);
                    rnumber = k->in_room->number;
                    sprintf(buf, "(%s)%-20s- %s [%d]\n\r", next_tmp_name, OSTR(k, short_descr),
                            rname_buf, rnumber);
                } else {
                    char held_by[MAX_STRING_LENGTH];
                    char buf2[MAX_STRING_LENGTH];

                    strcpy(rname_buf, "");
                    rnumber = -1;
                    if (k->carried_by) {
                        strcpy(held_by, REAL_NAME(k->carried_by));
                        if (k->carried_by->in_room) {
                            strcpy(rname_buf, k->carried_by->in_room->name);
                            rnumber = k->carried_by->in_room->number;
                        }
                    } else if (k->equipped_by) {
                        strcpy(held_by, REAL_NAME(k->equipped_by));
                        if (k->equipped_by->in_room) {
                            strcpy(rname_buf, k->equipped_by->in_room->name);
                            rnumber = k->equipped_by->in_room->number;
                        }
                    } else if (k->in_obj) {
                        strcpy(held_by, OSTR(k->in_obj, short_descr));

                        if (k->in_obj->in_room) {
                            strcpy(rname_buf, k->in_obj->in_room->name);
                            rnumber = k->in_obj->in_room->number;
                        } else if (k->in_obj->carried_by) {
                            sprintf(rname_buf, "%s", REAL_NAME(k->in_obj->carried_by));
                            if (k->in_obj->carried_by->in_room) {
                                sprintf(buf2, " - %s", k->in_obj->carried_by->in_room->name);
                                strcat(rname_buf, buf2);
                                rnumber = k->in_obj->carried_by->in_room->number;
                            }
                        } else if (k->in_obj->equipped_by) {
                            sprintf(rname_buf, "%s", REAL_NAME(k->in_obj->equipped_by));
                            if (k->in_obj->equipped_by->in_room) {
                                sprintf(buf2, " - %s", k->in_obj->equipped_by->in_room->name);
                                strcat(rname_buf, buf2);
                                rnumber = k->in_obj->equipped_by->in_room->number;
                            }
                        }
                    } else if (k->table) {
                        strcpy(held_by, OSTR(k->table, short_descr));

                        if (k->table->in_room) {
                            strcpy(rname_buf, k->table->in_room->name);
                            rnumber = k->table->in_room->number;
                        }
                    } else
                        strcpy(held_by, "Somewhere");

                    sprintf(buf, "(%s)%-20s- %s", next_tmp_name, OSTR(k, short_descr), held_by);

                    if (rname_buf[0] != '\0')
                        sprintf(held_by, " - %s [%d]\n\r", rname_buf, rnumber);
                    else
                        sprintf(held_by, "\n\r");

                    strcat(buf, held_by);
                }
                if ((len + strlen(buf)) > MAX_STRING_LENGTH) {
                    send_to_char("list too long", ch);
                    return;
                }
                len += strlen(buf);
                strcat(show_list, buf);
            }
            ++ocount;
        }
    }

    /* search the room */
    for (k = ch->in_room->contents; k; k = k->next_content) {
        if (isname(name, OSTR(k, name)) && CAN_SEE_OBJ(ch, k)) {
            sprintf(next_tmp_name, "%d.%s", ocount, name);
            if ((zone == -1) || (k->in_room && (zone == k->in_room->zone))) {

                if (k->in_room) {
                    strcpy(rname_buf, k->in_room->name);
                    rnumber = k->in_room->number;
                    sprintf(buf, "(%s)%-20s- %s [%d]\n\r", next_tmp_name, OSTR(k, short_descr),
                            rname_buf, rnumber);
                } else {
                    char held_by[MAX_STRING_LENGTH];
                    char buf2[MAX_STRING_LENGTH];

                    strcpy(rname_buf, "");
                    rnumber = -1;
                    if (k->carried_by) {
                        strcpy(held_by, REAL_NAME(k->carried_by));
                        if (k->carried_by->in_room) {
                            strcpy(rname_buf, k->carried_by->in_room->name);
                            rnumber = k->carried_by->in_room->number;
                        }
                    } else if (k->equipped_by) {
                        strcpy(held_by, REAL_NAME(k->equipped_by));
                        if (k->equipped_by->in_room) {
                            strcpy(rname_buf, k->equipped_by->in_room->name);
                            rnumber = k->equipped_by->in_room->number;
                        }
                    } else if (k->in_obj) {
                        strcpy(held_by, OSTR(k->in_obj, short_descr));

                        if (k->in_obj->in_room) {
                            strcpy(rname_buf, k->in_obj->in_room->name);
                            rnumber = k->in_obj->in_room->number;
                        } else if (k->in_obj->carried_by) {
                            sprintf(rname_buf, "%s", REAL_NAME(k->in_obj->carried_by));
                            if (k->in_obj->carried_by->in_room) {
                                sprintf(buf2, " - %s", k->in_obj->carried_by->in_room->name);
                                strcat(rname_buf, buf2);
                                rnumber = k->in_obj->carried_by->in_room->number;
                            }
                        } else if (k->in_obj->equipped_by) {
                            sprintf(rname_buf, "%s", REAL_NAME(k->in_obj->equipped_by));
                            if (k->in_obj->equipped_by->in_room) {
                                sprintf(buf2, " - %s", k->in_obj->equipped_by->in_room->name);
                                strcat(rname_buf, buf2);
                                rnumber = k->in_obj->equipped_by->in_room->number;
                            }
                        }
                    } else if (k->table) {
                        strcpy(held_by, OSTR(k->table, short_descr));

                        if (k->table->in_room) {
                            strcpy(rname_buf, k->table->in_room->name);
                            rnumber = k->table->in_room->number;
                        }
                    } else
                        strcpy(held_by, "Somewhere");

                    sprintf(buf, "(%s)%-20s- %s", next_tmp_name, OSTR(k, short_descr), held_by);

                    if (rname_buf[0] != '\0')
                        sprintf(held_by, " - %s [%d]\n\r", rname_buf, rnumber);
                    else
                        sprintf(held_by, "\n\r");

                    strcat(buf, held_by);
                }
                if ((len + strlen(buf)) > MAX_STRING_LENGTH) {
                    send_to_char("list too long", ch);
                    return;
                }
                len += strlen(buf);
                strcat(show_list, buf);
            }
            ++ocount;
        }
    }

    /* search the world list, skipping those in same room and carried
     * by ch */
    for (k = object_list; k; k = k->next) {
        if (isname(name, OSTR(k, name))
            && CAN_SEE_OBJ(ch, k) && k->in_room != ch->in_room && k->carried_by != ch) {
            sprintf(next_tmp_name, "%d.%s", ocount, name);
            if ((zone == -1) || (k->in_room && (zone == k->in_room->zone))) {

                if (k->in_room) {
                    strcpy(rname_buf, k->in_room->name);
                    rnumber = k->in_room->number;
                    sprintf(buf, "(%s)%-20s- %s [%d]\n\r", next_tmp_name, OSTR(k, short_descr),
                            rname_buf, rnumber);
                } else {
                    char held_by[MAX_STRING_LENGTH];
                    char buf2[MAX_STRING_LENGTH];

                    strcpy(rname_buf, "");
                    rnumber = -1;
                    if (k->carried_by) {
                        strcpy(held_by, REAL_NAME(k->carried_by));
                        if (k->carried_by->in_room) {
                            strcpy(rname_buf, k->carried_by->in_room->name);
                            rnumber = k->carried_by->in_room->number;
                        }
                    } else if (k->equipped_by) {
                        strcpy(held_by, REAL_NAME(k->equipped_by));
                        if (k->equipped_by->in_room) {
                            strcpy(rname_buf, k->equipped_by->in_room->name);
                            rnumber = k->equipped_by->in_room->number;
                        }
                    } else if (k->in_obj) {
                        strcpy(held_by, OSTR(k->in_obj, short_descr));

                        if (k->in_obj->in_room) {
                            strcpy(rname_buf, k->in_obj->in_room->name);
                            rnumber = k->in_obj->in_room->number;
                        } else if (k->in_obj->carried_by) {
                            sprintf(rname_buf, "%s", REAL_NAME(k->in_obj->carried_by));
                            if (k->in_obj->carried_by->in_room) {
                                sprintf(buf2, " - %s", k->in_obj->carried_by->in_room->name);
                                strcat(rname_buf, buf2);
                                rnumber = k->in_obj->carried_by->in_room->number;
                            }
                        } else if (k->in_obj->equipped_by) {
                            sprintf(rname_buf, "%s", REAL_NAME(k->in_obj->equipped_by));
                            if (k->in_obj->equipped_by->in_room) {
                                sprintf(buf2, " - %s", k->in_obj->equipped_by->in_room->name);
                                strcat(rname_buf, buf2);
                                rnumber = k->in_obj->equipped_by->in_room->number;
                            }
                        }
                    } else if (k->table) {
                        strcpy(held_by, OSTR(k->table, short_descr));

                        if (k->table->in_room) {
                            strcpy(rname_buf, k->table->in_room->name);
                            rnumber = k->table->in_room->number;
                        }
                    } else
                        strcpy(held_by, "Somewhere");

                    sprintf(buf, "(%s)%-20s- %s", next_tmp_name, OSTR(k, short_descr), held_by);

                    if (rname_buf[0] != '\0')
                        sprintf(held_by, " - %s [%d]\n\r", rname_buf, rnumber);
                    else
                        sprintf(held_by, "\n\r");

                    strcat(buf, held_by);
                }
                if ((len + strlen(buf)) > MAX_STRING_LENGTH) {
                    send_to_char("list too long", ch);
                    return;
                }
                len += strlen(buf);
                strcat(show_list, buf);
            }
            ++ocount;
        }
    }
    if (!*show_list)
        send_to_char("Couldn't find any such thing.\n\r", ch);
    else
        page_string(ch->desc, show_list, 1);
}


void calc_offense_defense(CHAR_DATA *ch, CHAR_DATA *victim, int type, OFF_DEF_DATA *data );

void
cmd_consider(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *target;
    CHAR_DATA *victim;
    char name[256], buf[256];
    int diff;
    struct off_def_data od_data1, od_data2;

    argument = one_argument(argument, name, sizeof(name));

    if (!(victim = get_char_room_vis(ch, name))) {
        send_to_char("Consider killing who?\n\r", ch);
        return;
    }

    argument = one_argument(argument, name, sizeof(name));

    if( name[0] != '\0' ) {
        target = victim;
        if (!(victim = get_char_room_vis(ch, name))) {
            send_to_char("Consider who killing who?\n\r", ch);
            return;
        }
    } else {
        target = ch;
    }

    if (victim == ch) {
        send_to_char("Easy! Very easy indeed!\n\r", ch);
        return;
    }

    diff = (get_char_size(victim) - get_char_size(target));

    if( target != ch ) {
        strcpy( buf, PERS(ch, victim));
        cprintf(ch, "You compare %s and %s.\n\r", PERS(ch, target), buf);
    }
    else {
        act("$n eyes you morbidly.", TRUE, ch, 0, victim, TO_VICT);
        act("$n eyes $N morbidly.", TRUE, ch, 0, victim, TO_NOTVICT);

        if (diff > 5)
            act("$E's a lot bigger than you are...", FALSE, ch, 0, victim, TO_CHAR);
        else if (diff > 0)
            act("$E's a little bigger than you, careful.", FALSE, ch, 0, victim, TO_CHAR);
        else if (diff < -5)
            act("Looks like you could rip $M limb from limb--the wimp!", FALSE, ch, 0, victim, TO_CHAR);
        else if (diff < 0)
            act("You have the size advantage...$E shouldn't be a problem.", FALSE, ch, 0, victim,
                TO_CHAR);
        else {                      /* same 'size' so check height/weight */
            if (ch->player.height > victim->player.height) {
                sprintf(buf, "You are taller than $E is, ");
                if (ch->player.weight > victim->player.weight)
                    strcat(buf, "and you have the weight advantage.");
                else
                    strcat(buf, "but $E looks heavier.");
            } else if (ch->player.height < victim->player.height) {
                sprintf(buf, "$E's taller than you are, ");
                if (victim->player.weight > ch->player.weight)
                    strcat(buf, "and heavier, too.");
                else
                    strcat(buf, "but looks lighter than you.");
            } else
                /* same height */
            {
                sprintf(buf, "You're the same height, ");
                if (ch->player.weight > victim->player.weight)
                    strcat(buf, "but $E looks lighter than you.");
                else if (ch->player.weight < victim->player.weight)
                    strcat(buf, "but $E looks heavier than you.");
                else
                    strcat(buf, "and about the same weight.");
            }
            act(buf, FALSE, ch, 0, victim, TO_CHAR);
        }
    }
    
    calc_offense_defense(target, victim, TYPE_UNDEFINED, &od_data1);
    calc_offense_defense(victim, target, TYPE_UNDEFINED, &od_data2);

    strcpy( buf, PERS(ch, target ));
    strcpy( name, PERS(ch, victim ));
    cprintf( ch, 
     "%s: offense: %3d defense: %3d\n\r%s: defense: %3d offense: %3d\n\r",
     buf, od_data1.offense, od_data2.defense,
     name, od_data1.defense, od_data2.offense);
}


void
cmd_races(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
  int r;
  int found_race = 0;
  char buf[256];
  char rtypestring[256];
  char look_for[256];
  char *show_list;
  struct skin_data *skin;
  
  CREATE(show_list, char, 256 * 2 * MAX_RACES);
  
  argument = one_argument(argument, look_for, sizeof(look_for));
  if (strlen(argument) > 0) {
    strcat(look_for, " ");
    strcat(look_for, argument);
  }
  
  if (strlen(look_for) > 0) {
    for (r = 1; r < MAX_RACES; r++) {
      if (!(stricmp(race[r].name, look_for))) {
	sprintf(buf,
		"%s - Stats: [%dd%d+%d, %dd%d+%d, %dd%d+%d, %dd%d+%d], "
		"Dam: [%dd%d+%d], Life Spn: [%d]\n\r", race[r].name, race[r].strdice,
		race[r].strsize, race[r].strplus, race[r].agldice, race[r].aglsize,
		race[r].aglplus, race[r].wisdice, race[r].wissize, race[r].wisplus,
		race[r].enddice, race[r].endsize, race[r].endplus, race[r].attack->damdice,
		race[r].attack->damsize, race[r].attack->damplus, race[r].max_age);
	strcat(show_list, buf);
	sprintf(buf, "   Off base: [%d], Def base: [%d], Nat Armor: [%d]\n\r",
		race[r].off_base, race[r].def_base, race[r].natural_armor);
	strcat(show_list, buf);
	
	switch (race[r].race_type) {
	case 0:
	  sprintf (rtypestring, "Other");
	  break;
	case 1:
	  sprintf (rtypestring, "Humanoid");
	  break;
	case 2:
	  sprintf (rtypestring, "Mammalian");
	  break;
	case 3:
	  sprintf (rtypestring, "Insectine");
	  break;
	case 4:
	  sprintf (rtypestring, "Avian - Flightless");
	  break;
	case 5:
	  sprintf (rtypestring, "Ophidian");
	  break;
	case 6:
	  sprintf (rtypestring, "Reptilian");
	  break;
	case 7:
	  sprintf (rtypestring, "Plant");
	  break;
	case 8:
	  sprintf (rtypestring, "Arachnid");
	  break;
	case 9:
	  sprintf (rtypestring, "Avian - Flying");
	  break;
	case 10:
	  sprintf (rtypestring, "Silt");
	  break;
	default:
	  sprintf (rtypestring, "UNKNOWN");
	  break;
	}
	
	sprintf(buf, "   Race Type: %s\n\r", rtypestring);
	strcat(show_list, buf);
	
	skin = race[r].skin;
	while (skin) {
	  sprintf(buf, "   SkinObj: %d,  SkinMod: %d\n\r   %s\n\r   %s\n\r", skin->item,
		  skin->bonus, skin->text[0], skin->text[1]);
	  strcat(show_list, buf);
	  skin = skin->next;
	}
	
	found_race = 1;
      }
    }
    if (!found_race)
      strcat(show_list, "Race not found.\n\r");
  }
  if (!found_race) {
    strcpy(show_list,
	   "Following is a list of all implemented races that you may give mobiles.\n\r");
    
    for (r = 1; r < MAX_RACES; r++) {
      sprintf(buf,
      	      "%s - Stats: [%dd%d+%d, %dd%d+%d, %dd%d+%d, %dd%d+%d], "
      	      "Dam: [%dd%d+%d], Life Spn: [%d]\n\r", race[r].name, race[r].strdice,
      	      race[r].strsize, race[r].strplus, race[r].agldice, race[r].aglsize,
      	      race[r].aglplus, race[r].wisdice, race[r].wissize, race[r].wisplus,
      	      race[r].enddice, race[r].endsize, race[r].endplus, race[r].attack->damdice,
      	      race[r].attack->damsize, race[r].attack->damplus, race[r].max_age);
      strcat(show_list, buf);
      sprintf(buf, "   Off base: [%d], Def base: [%d], Nat Armor: [%d]\n\r",
      	      race[r].off_base, race[r].def_base, race[r].natural_armor);
      strcat(show_list, buf);
    }
  }
  
  
  page_string(ch->desc, show_list, 1);
  free((char *) show_list);
}


enum traversal_flags
{
    CAN_SEE = (1 << 1),
    IN_CLAN = (1 << 2),
    WANTS_FEEDBACK = (1 << 3),
};

void
who_list_traverse(CHAR_DATA * ch)
{
}


void
cmd_idle(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_STRING_LENGTH], line[200], idle[80], tag[30];
    char tmp[MAX_STRING_LENGTH];
    struct descriptor_data *d;
    char arg1[MAX_INPUT_LENGTH];
    char target_name[MAX_INPUT_LENGTH];
    int level_limit = 0, len_types;
    bool show_conn_state = FALSE;

    for (len_types = 0; *connected_types[len_types] != '\n'; len_types++) {     /* do nothing count len_types up to the max */
    };


    argument = one_argument(argument, arg1, sizeof(arg1));

    target_name[0] = '\0';

    if (!strcmp(arg1, "-c")) {
        argument = one_argument(argument, arg1, sizeof(arg1));
        show_conn_state = TRUE;
    }

    if (is_number(arg1)) {
        level_limit = atoi(arg1);
    } else if (strlen(arg1) > 0) {
        strcpy(target_name, arg1);
    }


    strcpy(buf, "Armageddon Idle Statistics\n\r--------------------------" "\n\r");

    for (d = descriptor_list; d; d = d->next) {
        strcpy(line, "");

        // Don't show people without a character, if there is a level limit
        if (level_limit != 0 && !d->original && !d->character)
            continue;

        // Don't show OLs to anyone with less than HL privs
        if (((d->original && GET_LEVEL(d->original) == OVERLORD)
             || (d->character && GET_LEVEL(d->character) == OVERLORD))
            && (GET_LEVEL(ch) < HIGHLORD))
            continue;

        // Don't show people below the level limit
        if ((d->original && GET_LEVEL(d->original) < level_limit)
            || (d->character && GET_LEVEL(d->character) < level_limit))
            continue;

        /* if they have an account name */
        if( d->player_info && d->player_info->name ) {
            /* does the target name match the account name */
            if (strlen(target_name) > 0 
             && strcasecmp(target_name, d->player_info->name))
                continue;
        } else {
            /* no account, are they looking for 'unknown' */
            if( strlen(target_name) > 0 && strcasecmp(target_name, "unknown"))
                continue;
        }

        if (d->original)
            sprintf(line, "%-15s ", MSTR(d->original, name));
        else if (d->character)
            sprintf(line, "%-15s ", MSTR(d->character, name));
        else
            sprintf(line, "%-15s ", "Unknown");

        if (d->player_info && d->player_info->name) {
            sprintf(tmp, "(%s)", d->player_info->name);
            sprintf(line + strlen(line), "%-17s ", tmp);
        } else
            sprintf(line + strlen(line), "%-17s ", "(Unknown)");

        sprintf(line + strlen(line), "[%2d] ", d->descriptor);


        if (show_conn_state) {
            if ((d->connected < len_types) && (d->connected >= 0))
                sprintf(idle, "[%s] ", connected_types[d->connected]);
            else
                sprintf(idle, "[--Invalid Type %3d--] ", d->connected);
        } else {
            strcpy(idle, "Interacting Nicely");
            if ((d->idle / (WAIT_SEC * 20)) > 0)
                strcpy(idle, "Thinking?");
            if ((d->idle / (WAIT_SEC * 20)) > 1)
                strcpy(idle, "Lagging?");
            if ((d->idle / (WAIT_SEC * 20)) > 2)
                strcpy(idle, "Confused?");
            if ((d->idle / (WAIT_SEC * 20)) > 4)
                strcpy(idle, "Taking a dump?");
            if ((d->idle / (WAIT_SEC * 20)) > 7)
                strcpy(idle, "Fell in the toilet?");
            if ((d->idle / (WAIT_SEC * 20)) > 9)
                strcpy(idle, "Falling asleep?");
            if ((d->idle / (WAIT_SEC * 20)) > 14)
                strcpy(idle, "Sleeping?");
            if ((d->idle / (WAIT_SEC * 20)) > 19)
                strcpy(idle, "Rotting?");
            if ((d->idle / (WAIT_SEC * 20)) > 24)
                strcpy(idle, "Dead?");
            if ((d->idle / (WAIT_SEC * 20)) > 46)
                strcpy(idle, "Losing Player.. *OVER IDLE*");
        }

        sprintf(tag, "[%02d:%02d:%02d]  ", d->idle / (WAIT_SEC * 1200),
                d->idle % (WAIT_SEC * 1200) / (WAIT_SEC * 20),
                d->idle % (WAIT_SEC * 1200) % (WAIT_SEC * 20) / (WAIT_SEC / 3));
        strcat(line, tag);

        strcat(line, idle);
        strcat(line, "\r\n");

        if (d->original) {
            if (CAN_SEE(ch, d->original))
                strcat(buf, line);
        } else if (d->character) {
            if (CAN_SEE(ch, d->character))
                strcat(buf, line);
        } else {
            strcat(buf, line);
        }
    }

    send_to_char(buf, ch);
}



void
cmd_immcmds(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_STRING_LENGTH];
    int no, i;
    if (IS_NPC(ch))
        return;

    send_to_char("The following privileged commands are available:\n\r\n\r", ch);

    *buf = '\0';

    for (no = 1, i = 1; *command_name[i] != '\n'; i++)
        if ((GET_LEVEL(ch) >= cmd_info[i].minimum_level) && (cmd_info[i].minimum_level > MORTAL)) {

            sprintf(buf + strlen(buf), "%-12s", command_name[i]);
            if (!(no % 7))
                strcat(buf, "\n\r");
            no++;
        }
    strcat(buf, "\n\r");
    page_string(ch->desc, buf, 1);
}



bool
is_covered(CHAR_DATA * ch, CHAR_DATA * vict, int pos)
{
    switch (pos) {
    case WEAR_FINGER_R:
    case WEAR_FINGER_L:
    case WEAR_FINGER_R2:
    case WEAR_FINGER_L2:
    case WEAR_FINGER_R3:
    case WEAR_FINGER_L3:
    case WEAR_FINGER_R4:
    case WEAR_FINGER_L4:
    case WEAR_FINGER_R5:
    case WEAR_FINGER_L5:
        if (!vict->equipment[WEAR_HANDS])
            return FALSE;
        if (!CAN_SEE_OBJ(ch, vict->equipment[WEAR_HANDS]))
            return FALSE;
        return TRUE;
    case WEAR_BODY:
    case WEAR_BELT:
    case WEAR_ON_BELT_1:
    case WEAR_ON_BELT_2:
    case WEAR_WAIST:           /*   - Kelvik ...   */
        if (!vict->equipment[WEAR_ABOUT])
            return FALSE;
        if (!CAN_SEE_OBJ(ch, vict->equipment[WEAR_ABOUT]))
            return FALSE;
        if (is_cloak_open(ch, vict->equipment[WEAR_ABOUT]))
            return FALSE;
        if (is_cloak(vict, vict->equipment[WEAR_ABOUT]))
            return TRUE;
        return FALSE;
    case WEAR_SHOULDER_R:
        if (!vict->equipment[WEAR_OVER_SHOULDER_R])
            return FALSE;
        if (!CAN_SEE_OBJ(ch, vict->equipment[WEAR_OVER_SHOULDER_R]))
            return FALSE;
        return TRUE;
    case WEAR_SHOULDER_L:
        if (!vict->equipment[WEAR_OVER_SHOULDER_L])
            return FALSE;
        if (!CAN_SEE_OBJ(ch, vict->equipment[WEAR_OVER_SHOULDER_L]))
            return FALSE;
        return TRUE;
    default:
        return FALSE;
    }
}

bool
is_desc_covered(CHAR_DATA * ch, CHAR_DATA * vict, int pos)
{
    
    switch (pos) {
    case WEAR_BACK:
        // Nothing is being worn on back, or "about", then the slot isn't covered
        if ((!vict->equipment[WEAR_BODY]) && (!vict->equipment[WEAR_ABOUT]))
            return FALSE;

        // If there is gear in the location, but it is invisible for some reason, to the viewer,
        // then the slow isn't covered.
        if ((vict->equipment[WEAR_BODY] && !CAN_SEE_OBJ(ch, vict->equipment[WEAR_BODY]))
            && (vict->equipment[WEAR_ABOUT] && !CAN_SEE_OBJ(ch, vict->equipment[WEAR_ABOUT])))
            return FALSE;
        return TRUE;
    default:
        return is_covered(ch, vict, pos);
    }
}



void
send_spell_symbol_to_char(CHAR_DATA * ch, int sp, char *buf, int buf_len)
{
    char tmp[MAX_STRING_LENGTH];

    if ((skill[sp].element >= 1)
        && (skill[sp].element <= LAST_ELEMENT) && (skill[sp].sphere >= 1)
        && (skill[sp].sphere <= LAST_SPHERE) && (skill[sp].mood >= 1)
        && (skill[sp].mood <= LAST_MOOD)) {

        sprintf(tmp, "%-25s %s %s %s\n\r", skill_name[sp], Element[skill[sp].element - 1],
                Sphere[skill[sp].sphere - 1], Mood[skill[sp].mood - 1]);
    } else {
        sprintf(tmp, "%-25s No symbols\n\r", skill_name[sp]);
    }
    string_safe_cat(buf, tmp, buf_len);
}

void
cmd_symbol(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char buf[MAX_STRING_LENGTH];
    char spname[256];
    bool all = FALSE;
    bool found = FALSE;
    int element = -1;
    int mood = -1;
    int sphere = -1;
    int sp;

    if (!IS_NPC(ch) && !IS_IMMORTAL(ch)) {
        send_to_char("What?\n\r", ch);
        return;
    }

    for (; *argument == ' '; argument++);

    if (!*argument) {
        send_to_char("Usage: symbol [-a] [-e element] [-s sphere] [-m mood] [spell name]\n\r", ch);
        return;
    }

    strcpy(spname, argument);

    argument = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

    if (!str_prefix(arg1, "-all")) {
        all = TRUE;
    } else if (!str_prefix(arg1, "-element")) {
        for (element = 0; element < LAST_ELEMENT; element++)
            if (!str_prefix(arg2, Element[element]))
                break;

        if (arg2[0] == '\0' || element == LAST_ELEMENT) {
            cprintf(ch, "Element not found, valid elements are:\n\r");
            for (element = 0; element < LAST_ELEMENT; element++)
                cprintf(ch, "   %s\n\r", Element[element]);
            return;
        }
    } else if (!str_prefix(arg1, "-mood")) {
        for (mood = 0; mood < LAST_MOOD; mood++)
            if (!str_prefix(arg2, Mood[mood]))
                break;

        if (arg2[0] == '\0' || mood == LAST_MOOD) {
            cprintf(ch, "Mood not found, valid moods are:\n\r");
            for (mood = 0; mood < LAST_MOOD; mood++)
                cprintf(ch, "   %s\n\r", Mood[mood]);
            return;
        }
    } else if (!str_prefix(arg1, "-sphere")) {
        for (sphere = 0; sphere < LAST_SPHERE; sphere++)
            if (!str_prefix(arg2, Sphere[sphere]))
                break;

        if (arg2[0] == '\0' || sphere == LAST_SPHERE) {
            cprintf(ch, "Sphere not found, valid spheres are:\n\r");
            for (sphere = 0; sphere < LAST_SPHERE; sphere++)
                cprintf(ch, "   %s\n\r", Sphere[sphere]);
            return;
        }
    }

    buf[0] = '\0';
    for (sp = 1; *skill_name[sp] != '\n'; sp++) {
        if (skill[sp].sk_class != CLASS_MAGICK)
            continue;

        if (all || (element != -1 && skill[sp].element - 1 == element)
            || (mood != -1 && skill[sp].mood - 1 == mood)
            || (sphere != -1 && skill[sp].sphere - 1 == sphere)
            || !strnicmp(spname, skill_name[sp], strlen(spname))) {
            send_spell_symbol_to_char(ch, sp, buf, sizeof(buf));
            found = TRUE;
        }
    }

    if (!found && !all && element == -1 && mood == -1 && sphere == -1)
        cprintf(ch, "That spell does not exist.\n\r");
    else
        page_string(ch->desc, buf, TRUE);
    return;
}


void
cmd_memory(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256];
    int om = 0, nm = 0;
/* 

   sprintf(buf, "Armageddon is currently using %d bytes.\n\r", consumption());
   send_to_char(buf, ch);
 */

    sprintf(buf, "Average amount of memory used in objs: %d bytes.\n\r", om);
    send_to_char(buf, ch);
    sprintf(buf, "Average amount of memory used in npcs: %d bytes.\n\r", nm);
    send_to_char(buf, ch);
    sprintf(buf, "Memory used in existing objs: %d bytes.\n\r", ob_mem);
    send_to_char(buf, ch);
    sprintf(buf, "Memory used in existing npcs: %d bytes.\n\r", ch_mem);
    send_to_char(buf, ch);
}


void
cmd_count(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_STRING_LENGTH];
    char arg2[MAX_INPUT_LENGTH];
    char where[MAX_STRING_LENGTH];
    char what[MAX_STRING_LENGTH];
    OBJ_DATA *obj;
    int cnt = 0;

    argument = one_argument(argument, what, sizeof(what));
    argument = one_argument(argument, arg2, sizeof(arg2));

    if (what[0] == '\0') {
        send_to_char("Count what?\n\r", ch);
        return;
    }

    if (arg2[0] == '\0' || !strncmp(arg2, "inventory", strlen(arg2))
        || !strncmp(arg2, "held", strlen(arg2))) {
        obj = ch->carrying;
        strcpy(where, "in your inventory");
    } else if (!strncmp(arg2, "room", strlen(arg2))) {
        obj = ch->in_room->contents;
        strcpy(where, "in the room");
    } else if ((obj = get_obj_room_vis(ch, arg2)) == NULL) {
        sprintf(buf, "Count the %s in what?\n\r", what);
        send_to_char(buf, ch);
        return;
    } else if (obj->obj_flags.type == ITEM_CONTAINER) {
        if (is_closed_container(ch, obj)) {
            sprintf(buf, "%s is closed.\n\r", format_obj_to_char(obj, ch, 2));
            send_to_char(buf, ch);
            return;
        }
        sprintf(where, "in %s", format_obj_to_char(obj, ch, 2));
        obj = obj->contains;
    } else if (obj->obj_flags.type == ITEM_FURNITURE) {
        sprintf(where, "in %s", format_obj_to_char(obj, ch, 2));
        obj = obj->contains;
    } else if (!(obj->obj_flags.type == ITEM_FURNITURE || obj->obj_flags.type == ITEM_CONTAINER)) {
        act("There's nothing in $p to count.", FALSE, ch, obj, NULL, TO_CHAR);
        return;
    }

    if (!obj) {
        sprintf(buf, "What do you want to count %s in?", what);
        act(buf, TRUE, ch, NULL, NULL, TO_CHAR);
        return;
    }
#ifdef NESS_MONEY
    for (; obj; obj = obj->next_content)
        if (CAN_SEE_OBJ(ch, obj) && isname(what, OSTR(obj, name)))
            if (obj->obj_flags.type == ITEM_MONEY) {
                cnt += obj->obj_flags.value[0];
                sprintf(buf, "There are %d of %ss.\n\r", obj->obj_flags.value[0],
                        OSTR(obj, short_descr));
                send_to_char(buf, ch);
            } else
                cnt++;
#else
    for (; obj; obj = obj->next_content)
        if (CAN_SEE_OBJ(ch, obj) && isname(what, OSTR(obj, name))) {
            if (obj->obj_flags.type == ITEM_MONEY)
                cnt += obj->obj_flags.value[0];
            else
                cnt++;
        }
#endif

    // depluralize 'coins'
    if( !stricmp(what, "coins") ) {
       strcpy(what, "coin");
    }

    if (cnt) {
        sprintf(buf, "There %s %d %s %s.\n\r", cnt > 1 ? "are" : "is", cnt,
                cnt > 1 ? pluralize(what) : what, where);
        send_to_char(buf, ch);
    } else {
        sprintf(buf, "There isn't a single %s %s.\n\r", what, where);
        send_to_char(buf, ch);
    }
    return;
}


/* show_tables - part of Morg's dynamic tables */
void
show_tables(CHAR_DATA * ch)
{
    ROOM_DATA *in_room;
    OBJ_DATA *chair;
    OBJ_DATA *table;
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    int counter;
    int count_down;
    PLYR_LIST *sCh;

    in_room = ch->in_room;

    buf2[0] = '\0';
    counter = 1;
    for (table = in_room->contents; table; table = table->next_content) {
        if (table->obj_flags.type == ITEM_FURNITURE
            && (IS_SET(table->obj_flags.value[1], FURN_TABLE)
                || (IS_SET(table->obj_flags.value[1], FURN_SIT)
                    && table->obj_flags.value[2] > 1))
            && CAN_SEE_OBJ(ch, table)) {
            int chair_count;
            int people_count;
            int line_length;

            sprintf(buf, "At %d) %s are:\n\r      ", counter, format_obj_to_char(table, ch, 1));
            strcat(buf2, buf);

            chair_count = 0;
            people_count = 0;
            if (IS_SET(table->obj_flags.value[1], FURN_TABLE)) {
                for (chair = table->around; chair; chair = chair->next_content) {
                    chair_count += MAX(chair->obj_flags.value[2], 1);
                    people_count += count_plyr_list(chair->occupants, ch);
                }

                /* handle spaces for 'standing room' */
                if (IS_SET(table->obj_flags.value[1], FURN_STAND)
                    || IS_SET(table->obj_flags.value[1], FURN_SIT)
                    || IS_SET(table->obj_flags.value[1], FURN_REST)
                    || IS_SET(table->obj_flags.value[1], FURN_SLEEP)) {
                    /* allow extra seats for any open spots */
                    chair_count += MAX(table->obj_flags.value[2], 1)
                        - chair_count;
                    people_count += count_plyr_list(table->occupants, ch);
                }
            } else {
                chair_count = MAX(table->obj_flags.value[2], 1);
                people_count += count_plyr_list(table->occupants, ch);
            }


            line_length = 0;
            count_down = people_count;

            if (IS_SET(table->obj_flags.value[1], FURN_TABLE)) {
                for (chair = table->around; chair; chair = chair->next_content) {
                    for (sCh = chair->occupants; sCh; sCh = sCh->next) {
                        if (!CAN_SEE(ch, sCh->ch))
                            continue;

                        sprintf(buf, "%s%s%s", count_down == 1 && people_count != 1
                                && chair_count == people_count ? "and " : "", PERS(ch, sCh->ch),
                                count_down > 1 || (count_down == 1
                                                   && people_count < chair_count) ? ", " : "");

                        if (line_length + strlen(buf) > 73) {
                            strcat(buf2, "\n\r      ");
                            line_length = 0;
                        }

                        line_length += strlen(PERS(ch, sCh->ch));
                        strcat(buf2, buf);
                        count_down--;
                    }
                }
            }

            for (sCh = table->occupants; sCh; sCh = sCh->next) {
                if (!CAN_SEE(ch, sCh->ch))
                    continue;

                sprintf(buf, "%s%s%s", count_down == 1 && people_count != 1
                        && chair_count == people_count ? "and " : "", PERS(ch, sCh->ch),
                        count_down > 1 || (count_down == 1 && people_count < chair_count)
                        ? ", " : "");

                if (line_length + strlen(buf) > 73) {
                    strcat(buf2, "\n\r      ");
                    line_length = 0;
                }

                line_length += strlen(PERS(ch, sCh->ch));
                strcat(buf2, buf);
                count_down--;
            }

            /* empty seats left? */
            if (people_count < chair_count) {
                sprintf(buf, "%s%s empty seat%s.\n\r", people_count ? "and " : "",
                        numberize(chair_count - people_count),
                        chair_count - people_count == 1 ? "" : "s");

                if (line_length + strlen(buf) > 73) {
                    strcat(buf2, "\n\r      ");
                    line_length = 0;
                }
                strcat(buf2, buf);
            } else if (chair_count == 0) {
                strcat(buf2, "no seats at all.\n\r");
            } else {
                strcat(buf2, ".\n\r");
            }
            counter++;
        }
    }

    if (counter == 1) {
        send_to_char("You can't find any tables in the area.\n\r", ch);
        return;
    }

    page_string(ch->desc, buf2, 1);
}


void
show_obj_properties(OBJ_DATA * obj, CHAR_DATA * ch, int bits)
{
    char buf[MAX_STRING_LENGTH];
    char tmpBuf[MAX_STRING_LENGTH];
    char color[20];
    int single = FALSE;
    PLYR_LIST *sCh;

    strcpy(color, "");

    if (!obj || !ch)
        return;

    if (IS_OBJ_STAT(obj, OFL_MAGIC)
        && (IS_AFFECTED(ch, CHAR_AFF_DETECT_MAGICK) || affected_by_spell(ch, PSI_MAGICKSENSE)))
        send_to_char("It has a faint blue glow.\n\r", ch);

    if (IS_OBJ_STAT(obj, OFL_ETHEREAL_OK)
        && (IS_AFFECTED(ch, CHAR_AFF_DETECT_MAGICK) || affected_by_spell(ch, PSI_MAGICKSENSE)
            || IS_AFFECTED(ch, CHAR_AFF_DETECT_ETHEREAL)))
        send_to_char("It has a strange shadowy hue to it.\n\r", ch);

    if (IS_OBJ_STAT(obj, OFL_GLOW))
        send_to_char("It is surrounded by a soft glowing aura.\n\r", ch);

    if (IS_OBJ_STAT(obj, OFL_HUM))
        send_to_char("It emits a faint humming sound.\n\r", ch);

    if ((find_ex_description("[SPELL_MARK]", obj->ex_description, TRUE))
        || (find_ex_description("[SAND_STATUE_MDESC]", obj->ex_description, TRUE)))
        send_to_char("A small image glows softly upon its surface.\n\r", ch);

    if (find_ex_description("[SPELL_VAMPIRIC_BLADE]", obj->ex_description, TRUE))
        send_to_char("It is in a state of rapid decay.\n\r", ch);

    if (IS_SET(obj->obj_flags.state_flags, OST_BLOODY))
        send_to_char("It is stained with blood.\n\r", ch);

    if (IS_SET(obj->obj_flags.state_flags, OST_TORN))
        send_to_char("The material is torn in many places.\n\r", ch);

    if (IS_SET(obj->obj_flags.state_flags, OST_TATTERED))
        send_to_char("The material is tattered, almost ruined.\n\r", ch);

    if (IS_SET(obj->obj_flags.state_flags, OST_LACED))
        send_to_char("The edges of this garmet have been stitched with lace.\n\r", ch);

    if (IS_SET(obj->obj_flags.state_flags, OST_EMBROIDERED))
        send_to_char("This garment has been embroidered.\n\r", ch);

    if (IS_SET(obj->obj_flags.state_flags, OST_FRINGED))
        send_to_char("Fringes has been sewn onto the edges of this garment.\n\r", ch);

    if (IS_SET(obj->obj_flags.state_flags, OST_MUDCAKED))
        send_to_char("Dried mud cakes the surface of this item.\n\r", ch);

    if (IS_SET(obj->obj_flags.state_flags, OST_DUSTY))
        send_to_char("It is covered with dust and sand.\n\r", ch);

    if (IS_SET(obj->obj_flags.state_flags, OST_STAINED))
        send_to_char("Various stains cover its surface.\n\r", ch);

    if (IS_SET(obj->obj_flags.state_flags, OST_ASH))
        send_to_char("It is covered with a fine layer of ash.\n\r", ch);

    if (IS_SET(obj->obj_flags.state_flags, OST_REPAIRED))
        send_to_char("This item seems to have been repaired in the past.\n\r", ch);

    if (IS_SET(obj->obj_flags.state_flags, OST_GITH))
        send_to_char("A repugnant odor clings to this item.\n\r", ch);

    if (IS_SET(obj->obj_flags.state_flags, OST_BURNED))
        send_to_char("It seems to have been burned.\n\r", ch);

    if (IS_SET(obj->obj_flags.state_flags, OST_SEWER))
        send_to_char("A heavy stench clings to this item.\n\r", ch);

    if (IS_SET(obj->obj_flags.state_flags, OST_OLD))
        send_to_char("It appears to be quite old.\n\r", ch);

    if (IS_SET(obj->obj_flags.state_flags, OST_SWEATY))
        send_to_char("Sweat stains are visible in many places.\n\r", ch);

    switch (GET_ITEM_TYPE(obj)) {
    case ITEM_CONTAINER: 
      {
	if (IS_SET(obj->obj_flags.value[1], CONT_SEAL_BROKEN)) {
	  cprintf(ch, "A seal has been placed upon it, but is now broken.\n\r");
	} else if (IS_SET(obj->obj_flags.value[1], CONT_SEALED)) {
	  cprintf(ch, "A seal has been placed upon it.\n\r");
	}
	break;
      }
    case ITEM_LIGHT:
        show_light_properties(obj, ch);
        break;
    case ITEM_NOTE:
        {
            int i;
            int pages_used = 0;
            int perc_full = 0;
            char page[256];
            char *title;

            if (IS_SET(obj->obj_flags.value[3], NOTE_BROKEN)) {
                cprintf(ch, "A seal has been placed upon it, but is now broken.\n\r");
            } else if (IS_SET(obj->obj_flags.value[3], NOTE_SEALED)) {
                cprintf(ch, "A seal has been placed upon it.\n\r");
            }

            /* get the book title. */
            if ((title = find_ex_description("[BOOK_TITLE]", obj->ex_description, TRUE))) {
                if (has_skill(ch, obj->obj_flags.value[2])
                    && ch->skills[obj->obj_flags.value[2]]->learned > 40) {
                    cprintf(ch, "It is titled \"%s\"\n\r", title);
                }
            }

            cprintf(ch, "It has %d pages in it.\n\r", obj->obj_flags.value[0]);

            for (i = 0; i < obj->obj_flags.value[0]; i++) {
                sprintf(page, "page_%d", i);
                if (find_page_ex_description(page, obj->ex_description, FALSE))
                    pages_used++;
            }

            if (pages_used > 0 && obj->obj_flags.value[0] > 0) {
                perc_full = (pages_used * 100 / obj->obj_flags.value[0] * 100) / 100;
                //      sprintf(buf, "Percentage is %d.", perc_full);
                //      gamelog (buf);

                if (perc_full > 75)
                    cprintf(ch, "It is almost filled with writing.\n\r");
                else if (perc_full > 50)
                    cprintf(ch, "More than half of it is filled with writing.\n\r");
                else if (perc_full > 33)
                    cprintf(ch, "More than a third of it it is filled with writing.\n\r");
                else if (perc_full > 25)
                    cprintf(ch, "Barely a quarter of it is filled with writing.\n\r");
            }

            read_item(obj, ch, buf);
            page_string(ch->desc, buf, 1);
            break;
        }

    case ITEM_SCROLL:
        read_item(obj, ch, buf);
        page_string(ch->desc, buf, 1);
        break;

    case ITEM_MONEY:
        if (obj->obj_flags.value[0] == 1)
            single = TRUE;

        sprintf(buf, "There %s %d coin%s.\n\r", single ? "is" : "are", obj->obj_flags.value[0],
                single ? "" : "s");
        break;
    case ITEM_FURNITURE:
        /* If it's a spice_run_table object, show the game board here */
        if (has_special_on_cmd(obj->programs, "spice_run_table", -1))
            print_spice_run_table(ch, obj);

        for (sCh = obj->occupants; sCh; sCh = sCh->next) {
            switch (GET_POS(sCh->ch)) {
            case POSITION_STANDING:
                if (IS_SET(obj->obj_flags.value[1], FURN_SKIMMER)
                  || IS_SET(obj->obj_flags.value[1], FURN_WAGON))
                    sprintf(tmpBuf, "standing on it");
                else
                    sprintf(tmpBuf, "standing at it");
                break;
            case POSITION_SITTING:
                sprintf(tmpBuf, "sitting on it");
                break;
            case POSITION_RESTING:
                sprintf(tmpBuf, "resting on it");
                break;
            case POSITION_SLEEPING:
                sprintf(tmpBuf, "sleeping on it");
                break;
            default:
                sprintf(tmpBuf, "bad position: %d", GET_POS(sCh->ch));
                break;
            }

            if (sCh->ch && sCh->ch != ch) {
                sprintf(buf, "$N is %s.", tmpBuf);
                act(buf, FALSE, ch, NULL, sCh->ch, TO_CHAR);
            } else if (sCh->ch) {
                sprintf(buf, "You are %s.", tmpBuf);
                act(buf, FALSE, ch, NULL, NULL, TO_CHAR);
            }
        }

        if (IS_SET(obj->obj_flags.value[1], FURN_TABLE)) {
            /* loop pointer to an object around the table */
            OBJ_DATA *around;
            /* counter to keep track of how many chairs are around the table */
            int chair_count = 0;
            /* keep track of how many players are around the table */
            int plyr_count = 0;

            /* look at all the objects (chairs) around the table */
            for (around = obj->around; around; around = around->next_content) {
                /* increment our chair count by chair's capacity */
                chair_count += MAX(around->obj_flags.value[2], 1);

                /* look to see how many people are on the chair */
                for (sCh = around->occupants; sCh; sCh = sCh->next) {
                    /* depending on the position of the occupant 
                     * set up how they are on it
                     */
                    switch (GET_POS(sCh->ch)) {
                    case POSITION_STANDING:
                        if (IS_SET(obj->obj_flags.value[1], FURN_SKIMMER)
                         || IS_SET(obj->obj_flags.value[1], FURN_WAGON))
                            sprintf(tmpBuf, "standing on it");
                        else
                            sprintf(tmpBuf, "standing at it");
                        break;
                    case POSITION_SITTING:
                        sprintf(tmpBuf, "sitting at it");
                        break;
                    case POSITION_RESTING:
                        sprintf(tmpBuf, "resting at it");
                        break;
                    case POSITION_SLEEPING:
                        sprintf(tmpBuf, "sleeping at it");
                        break;
                    default:
                        sprintf(tmpBuf, "bad position: %d", GET_POS(sCh->ch));
                        break;
                    }

                    /* if there's someone on it and the viewer isn't them */
                    if (sCh->ch && sCh->ch != ch) {
                        sprintf(buf, "$N is %s, on $p.", tmpBuf);
                        act(buf, FALSE, ch, around, sCh->ch, TO_CHAR);
                    }
                    /* otherwise there's someone on it so it must be them */
                    else if (sCh->ch) {
                        sprintf(buf, "You are %s, on $p.", tmpBuf);
                        act(buf, FALSE, ch, around, NULL, TO_CHAR);
                    }
                }
            }

            /* list the chairs around it */
            list_obj_to_char(obj->around, ch, 0, FALSE);

            /* get how many people they see around the table */
            plyr_count = count_plyr_list(obj->occupants, ch);

            /* check to see if there are spaces left at the table for
             * chairs to be pulled up, or people to sit/stand
             */
            if (plyr_count + chair_count < MAX(obj->obj_flags.value[2], 1)) {
                /* figure out how many spaces are available */
                /* starting with the capacity */
                int spaces_left = MAX(obj->obj_flags.value[2], 1);

                /* subtract occupied spaces */
                spaces_left -= plyr_count + chair_count;

                sprintf(buf, "There %s %s space%s at it.\n\r", spaces_left == 1 ? "is" : "are",
                        numberize(spaces_left), spaces_left == 1 ? "" : "s");
                send_to_char(buf, ch);
            }

            /* give them a blank line */
            send_to_char("\n\r", ch);
        }

        /* if you can put things on it */
        show_list_of_obj_to_char(obj, obj->contains, ch, "On", bits);
        break;
    case ITEM_MUSICAL:
        if ((IS_SET(obj->obj_flags.value[2], CONT_SHOW_CONTENTS)) && (obj->contains)) {
            if (IS_SET(ch->specials.brief, BRIEF_EQUIP)) {
                char *tmpstr;
                char buffer[MAX_STRING_LENGTH];

                sprintf(buffer, "Attached to %s's charm string is %s.",
                 format_obj_to_char(obj,ch,1),
                 list_obj_to_char(obj->contains, ch, 2, TRUE));
                tmpstr = strdup(buffer);
                tmpstr = format_string(tmpstr);
                cprintf(ch, "%s", tmpstr);
                free(tmpstr);
            } else {
                cprintf(ch, "Attached to %s's charm string is:\n\r%s\n\r",
                 format_obj_to_char(obj,ch,1),
                 list_obj_to_char(obj->contains, ch, 2, TRUE));
            }
        }
        break;
    case ITEM_WORN:
    case ITEM_JEWELRY:
        if (IS_SET(obj->obj_flags.value[1], CONT_SHOW_CONTENTS)) {
            show_list_of_obj_to_char(obj, obj->contains, ch, "On", bits);
        }
        break;
    case ITEM_PLAYABLE:
        switch (obj->obj_flags.value[0]) {
        case PLAYABLE_DICE:
        case PLAYABLE_DICE_WEIGHTED:
            if (obj->obj_flags.value[1]) {
                act("$p show:", FALSE, ch, obj, NULL, TO_CHAR);
                print_dice(ch, obj->obj_flags.value[1], obj->obj_flags.value[2]);
            }
            break;
        case PLAYABLE_BONES:
            if (obj->obj_flags.value[1]) {
                act("$p show:", FALSE, ch, obj, NULL, TO_CHAR);
                print_bones(ch, obj);
            }
            break;
        case PLAYABLE_HALFLING_STONES:
            if (obj->obj_flags.value[1]) {
                print_halfling_stones(ch, obj, FALSE);
            }
            break;
        case PLAYABLE_GYPSY_DICE:
            if (obj->obj_flags.value[1]) {
                act("$p show:", FALSE, ch, obj, NULL, TO_CHAR);
                print_gypsy_dice(ch, obj);
            }
            break;
        }
        break;
    case ITEM_ARMOR:
        /* permanently damaged */
        if (obj->obj_flags.value[3] > 0 && obj->obj_flags.value[1] < obj->obj_flags.value[3]) {
            act("$p is permanently damaged.", FALSE, ch, obj, NULL, TO_CHAR);
        }
        break;
    default:
        break;
    }
}

extern time_t boot_time;

char *
time_delta(char *buf, time_t start, time_t end)
{
    char *bufp = buf;
    time_t uptime = end - start;
    int updays = uptime / (60*60*24);

    if (updays)
        bufp += sprintf(bufp, "%d day%s, ", updays, (updays != 1) ? "s" : "");

    int upminutes = uptime / 60;
    int uphours = (upminutes / 60) % 24;
    upminutes %= 60;

    bufp += sprintf(bufp, "%02d:%02d:%02d", uphours, upminutes, (int)(uptime % 60));

    return buf;
}


void
cmd_systat(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char delta[255];
    char time_str[255];
    char arg1[MAX_LINE_LENGTH];
    long ct;
    struct rusage rusage_data;
    struct rlimit rlim_data;

    if (!ch)
        return;

    argument = one_argument(argument, arg1, sizeof(arg1));

    if (*arg1) {
        if (!str_prefix(arg1, "usage")) {
            if (!getrusage(RUSAGE_SELF, &rusage_data)) {
                cprintf(ch, "Usage statistics:\n\r-----------------\n\r");
                cprintf(ch, "User time used in seconds:              %ld.%ld\n\r",
                        rusage_data.ru_utime.tv_sec, rusage_data.ru_utime.tv_usec);
                cprintf(ch, "System time used in seconds:            %ld.%ld\n\r",
                        rusage_data.ru_stime.tv_sec, rusage_data.ru_stime.tv_usec);
                cprintf(ch, "Maximum resident set size:              %ld\n\r",
                        rusage_data.ru_maxrss);
                cprintf(ch, "Integral shared memory size:            %ld\n\r",
                        rusage_data.ru_idrss);
                cprintf(ch, "Integral unshared data size:            %ld\n\r",
                        rusage_data.ru_idrss);
                cprintf(ch, "Integral unshared stack size:           %ld\n\r",
                        rusage_data.ru_isrss);
                cprintf(ch, "Page size:                              %ld\n\r",
                        sysconf(_SC_PAGESIZE));
                cprintf(ch, "Pages of physical memory:               %ld\n\r",
                        sysconf(_SC_PHYS_PAGES));
                cprintf(ch, "Pages of physical memory available:     %ld\n\r",
                        sysconf(_SC_AVPHYS_PAGES));
                cprintf(ch, "Page reclaims:                          %ld\n\r",
                        rusage_data.ru_minflt);
                cprintf(ch, "Page faults:                            %ld\n\r",
                        rusage_data.ru_majflt);
                cprintf(ch, "Swaps:                                  %ld\n\r",
                        rusage_data.ru_nswap);
                cprintf(ch, "Block input operations:                 %ld\n\r",
                        rusage_data.ru_inblock);
                cprintf(ch, "Block output operations:                %ld\n\r",
                        rusage_data.ru_oublock);
                cprintf(ch, "Messages sent:                          %ld\n\r",
                        rusage_data.ru_msgsnd);
                cprintf(ch, "Messages received:                      %ld\n\r",
                        rusage_data.ru_msgrcv);
                cprintf(ch, "Signals received:                       %ld\n\r",
                        rusage_data.ru_nsignals);
                cprintf(ch, "Voluntary context switches:             %ld\n\r",
                        rusage_data.ru_nvcsw);
                cprintf(ch, "Involuntary context switches:           %ld\n\r",
                        rusage_data.ru_nivcsw);
            }
        }

    } else {
        cprintf(ch, "System statistics:\n\r------------------\n\r");
        ctime_r(&boot_time, time_str);

        cprintf(ch, "System was started on : %.*s.\n\r", (int)strlen(time_str) - 1, time_str);

        ct = time(0);
        ctime_r(&ct, time_str);
        cprintf(ch, "Current system time is: %.*s.\n\r", (int)strlen(time_str) - 1, time_str);

        time_delta(delta, boot_time, ct);
        cprintf(ch, "Uptime: %s\n\r", delta);

        if (lock_new_char)
            send_to_char("Newlock ON  - no new characters.\n\r", ch);
        else
            send_to_char("Newlock OFF - new characters allowed.\n\r", ch);

        if (lock_mortal)
            send_to_char("Mortal Lock ON  - No Mortals allowed.\n\r", ch);
        else
            send_to_char("Mortal Lock OFF - Everyone allowed in.\n\r", ch);

        if (lock_arena)
            send_to_char("Arena Games OFF - Viewing disabled.\n\r", ch);
        else
            send_to_char("Arena Games ON  - Viewing enabled.\n\r", ch);
    }

    return;
}

/* cmd_biography()
 *
 * The actual command called by the Armageddon command parser
 */
void
cmd_biography(CHAR_DATA * ch, const char *arg, Command cmd, int count)
{
 cprintf(ch, "Not implemented in easy version.\n");
}

void
cmd_keyword(CHAR_DATA * ch, const char *arg, Command cmd, int count)
{
    char arg1[MAX_INPUT_LENGTH];
    char arg2[MAX_INPUT_LENGTH];

    if (!*arg) {
        cprintf(ch, "You must specify a keyword to test.\n\r");
        return;
    }

    arg = two_arguments(arg, arg1, sizeof(arg1), arg2, sizeof(arg2));

    if (!*arg2) {
        show_keyword_found(arg1, FIND_CHAR_ROOM | FIND_OBJ_ROOM | FIND_OBJ_INV | FIND_OBJ_EQUIP | FIND_OBJ_ROOM_ON_OBJ,
                           ch);
    } else {
        CHAR_DATA *tar_ch;
        OBJ_DATA *tar_obj, *obj;
        int count = 1;
        char in_on[MAX_INPUT_LENGTH];

        generic_find(arg2, FIND_OBJ_ROOM | FIND_OBJ_INV | FIND_OBJ_EQUIP, ch, &tar_ch, &tar_obj);

        if (!tar_obj) {
            cprintf(ch, "No '%s' here to find keywords for.\n\r", arg2);
            return;
        }

        switch (GET_ITEM_TYPE(tar_obj)) {
        case ITEM_CONTAINER:
            if (!IS_IMMORTAL(ch)
                && is_closed_container(ch, tar_obj)) {
                cprintf(ch, "%s is closed.\n\r", capitalize(format_obj_to_char(tar_obj, ch, 1)));
                return;
            }
            sprintf(in_on, "In");
            break;
        case ITEM_LIGHT:
            if (!IS_CAMPFIRE(tar_obj)) {
                cprintf(ch, "You can't look in %s.\n\r", format_obj_to_char(tar_obj, ch, 1));
                return;
            }
            sprintf(in_on, "In");
            break;
        case ITEM_FURNITURE:
            if (!IS_SET(tar_obj->obj_flags.value[1], FURN_PUT)) {
                cprintf(ch, "There's nothing on %s.\n\r", format_obj_to_char(tar_obj, ch, 1));
                return;
            }
            sprintf(in_on, "On");
            break;
        default:
            cprintf(ch, "There's nothing like that in or on %s.\n\r",
                    format_obj_to_char(tar_obj, ch, 1));
            return;
        }

        for (obj = tar_obj->contains; obj; obj = obj->next_content) {
            if (CAN_SEE_OBJ(ch, obj)
                && (isname(arg1, real_sdesc_keywords_obj(obj, ch))
                    || isname(arg1, OSTR(obj, name)))) {
                if (count == 1) {
                    cprintf(ch, "%s %s:\n\r", in_on, format_obj_to_char(tar_obj, ch, 1));
                }
                show_keyword_obj_to_char(ch, count, arg1, obj);
                count++;
            }
        }

        if (count == 1) {
            cprintf(ch, "You can't find any '%s' in %s.\n\r", arg1,
                    format_obj_to_char(tar_obj, ch, 1));
        }
    }
}

OBJ_DATA *
get_eq_char( CHAR_DATA *ch, int wear_loc ) {
    return ch->equipment[wear_loc];
}

char *
show_eq_to_char( OBJ_DATA *obj, CHAR_DATA *tch, sh_int is_are_first ) {
    static char buf[MAX_STRING_LENGTH];
    char tmp1[MAX_STRING_LENGTH];
    sh_int is_sheath = get_sheathed_weapon(obj) != NULL;

    strcpy( tmp1, format_obj_to_char(obj, tch, TRUE));
    if( !is_are_first ) {
        sprintf(buf, "%s%s%s", tmp1, 
         is_sheath ? "" : " ",
         is_sheath ? "" : is_are(tmp1));
    } else {
        sprintf(buf, "%s%s%s",
         is_sheath ? "" : is_are(tmp1), 
         is_sheath ? "" : " ", 
         tmp1);
    }
    return buf;
}

void
show_equipment(CHAR_DATA * ch, CHAR_DATA * tch)
{
    OBJ_DATA *obj;
    OBJ_DATA *obj2;
    OBJ_DATA *obj3;
    OBJ_DATA *obj4;
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char tmp1[MAX_STRING_LENGTH];
    char tmp2[MAX_STRING_LENGTH];
    char tmp3[MAX_STRING_LENGTH];
    char tmp4[MAX_STRING_LENGTH];
    char descr[MAX_STRING_LENGTH];
    char *finalstr;
    bool fBool;
    int oType;

    buf[0] = '\0';
    descr[0] = '\0';
    // indent the equipment?
    strcat( descr, "   " );

    fBool = FALSE;
    if (IS_NPC(ch) && IS_SET(ch->specials.act, CFL_MOUNT)) {
        obj = get_eq_char(ch, WEAR_BODY);
        if (obj) {
            strcpy(tmp1, format_obj_to_char(obj, tch, TRUE));
            sprintf(buf, "%s has %s on %s back", HSSH(ch), tmp1, HSHR(ch));
            strcat(descr, buf);
            fBool = TRUE;
        }

        obj = get_eq_char(ch, WEAR_NECK);
        if (obj && fBool) {
            strcpy(tmp1, format_obj_to_char(obj, tch, TRUE));
            sprintf(buf, ", and %s around %s neck.", tmp1, HSHR(ch));
            strcat(descr, buf);
        } else if (obj) {
            sprintf(buf, "%s around %s neck.",
             show_eq_to_char(obj, tch, FALSE), HSHR(ch));
            strcat(descr, buf);
        } else if (fBool)
            strcat(descr, ".");

        if (descr[0] != '\0') {
            send_to_char("\n\r", tch);
            finalstr = format_string(strdup(descr));
            send_to_char(finalstr, tch);
            free(finalstr);
        }
        return;
    }

    fBool = FALSE;

    if ((obj = get_eq_char(ch, WEAR_HANDS)) != NULL && CAN_SEE_OBJ(tch, obj)) {
        sprintf(buf, "%s worn on %s hands.", 
         show_eq_to_char(obj, tch, 0), HSHR(ch));
        strcat(descr, buf);
    } else { 
        // no gloves covering hands
        obj = get_eq_char(ch, WEAR_FINGER_L);
        obj2 = get_eq_char(ch, WEAR_FINGER_R);
        if (obj)
            strcpy(tmp1, format_obj_to_char(obj, tch, TRUE));
        if (obj2)
            strcpy(tmp2, format_obj_to_char(obj2, tch, TRUE));

        if (obj != NULL && CAN_SEE_OBJ(tch, obj)
         && obj2 != NULL && CAN_SEE_OBJ(tch, obj2)
         && !strcmp(tmp1, tmp2)) {
            sprintf(buf, "%s is wearing two %s on %s fingers. ",
             HSSH(ch), pluralize(tmp1), HSHR(ch));
            strcat(descr, buf);
        } else {
            if (obj != NULL && CAN_SEE_OBJ(tch, obj)) {
                sprintf(buf, "On %s off hand, %s is wearing %s", 
                 HSHR(ch), HSSH(ch), tmp1 );
                strcat(descr, buf);
                fBool = TRUE;
            }
    
            if (obj2 != NULL && CAN_SEE_OBJ(tch, obj2)) {
                if (fBool)
                    sprintf(buf, " and %s %s on %s right. ", 
                     tmp2, is_are(tmp2), HSHR(ch));
                else
                    sprintf(buf, "On %s primary hand %s %s. ", HSHR(ch),
                     is_are(tmp2), tmp2);
                strcat(descr, buf);
            }
        }
    }

    fBool = FALSE;

    if ((obj = get_eq_char(ch, WEAR_HEAD)) != NULL && CAN_SEE_OBJ(tch, obj)) {
        sprintf(buf, "%s is wearing %s on %s head.", HSSH(ch),
         format_obj_to_char(obj, tch, TRUE), HSHR(ch));
        strcat(descr, buf);
    } 
    fBool = FALSE;

    if ((obj = get_eq_char(ch, WEAR_IN_HAIR)) != NULL 
     && CAN_SEE_OBJ(tch, obj)) {
        sprintf(buf, "%s placed in %s hair.",
         show_eq_to_char(obj, tch, FALSE), HSHR(ch));
        strcat(descr, buf);
    }
    fBool = FALSE;

    if ((obj = get_eq_char(ch, WEAR_FACE)) != NULL 
     && CAN_SEE_OBJ(tch, obj)) {
        sprintf(buf, "%s worn across %s face.",
         show_eq_to_char(obj, tch, FALSE), HSHR(ch));
        strcat(descr, buf);
    }
    fBool = FALSE;

    if ((obj = get_eq_char(ch, WEAR_ABOUT_HEAD)) != NULL 
     && CAN_SEE_OBJ(tch, obj)) {
        sprintf(buf, "%s floating near %s head.", 
         show_eq_to_char(obj, tch, FALSE), HSHR(ch));
        strcat(descr, buf);
    }
    fBool = FALSE;

    obj = get_eq_char(ch, WEAR_RIGHT_EAR);
    obj2 = get_eq_char(ch, WEAR_LEFT_EAR);
    if (obj)
        strcpy(tmp1, format_obj_to_char(obj, tch, TRUE));
    if (obj2)
        strcpy(tmp2, format_obj_to_char(obj2, tch, TRUE));

    if (obj && CAN_SEE_OBJ(tch, obj)
     && obj2 && CAN_SEE_OBJ(tch, obj2) && !strcmp(tmp1, tmp2)) {
        sprintf(buf, "A pair of %s hang from %s ears.", pluralize(tmp1), 
         HSHR(ch));
        strcat(descr, buf);
    } else {
        if (obj && CAN_SEE_OBJ(tch, obj)) {
            sprintf(buf, "Hanging from %s right ear, %s", HSHR(ch),
             show_eq_to_char( obj, tch, TRUE ));
            strcat(descr, buf);
            fBool = TRUE;
        }

        if (obj2 && CAN_SEE_OBJ(tch, obj2)) {
            if (fBool)
                sprintf(buf, " and %s on %s left.",
                 show_eq_to_char(obj2, tch, 0), HSHR(ch));
            else
                sprintf(buf, "Hanging from %s left ear %s.",
                 HSHR(ch),
                 show_eq_to_char(obj2, tch, TRUE));
            strcat(descr, buf);
        } else if (fBool)
            strcat(descr, ".");
    }

    fBool = FALSE;


    if (((obj = get_eq_char(ch, WEAR_NECK)) != NULL && CAN_SEE_OBJ(tch, obj))) {
        sprintf(buf, "%s worn on %s neck.", show_eq_to_char(obj, tch, 0),
         HSHR(ch));
        strcat(descr, buf);
    }
    fBool = FALSE;

    if (((obj = get_eq_char(ch, WEAR_ABOUT_THROAT)) != NULL 
     && CAN_SEE_OBJ(tch, obj))) {
        sprintf(buf, "%s hangs about %s throat.", show_eq_to_char(obj, tch, 0),
         HSHR(ch));
        strcat(descr, buf);
    }
    fBool = FALSE;

    if ((obj = get_eq_char(ch, WEAR_ABOUT)) != NULL && CAN_SEE_OBJ(tch, obj)) {
        sprintf(buf, "%s draped over %s ", show_eq_to_char(obj, tch, 0),
         HSHR(ch));
        if ((obj = get_eq_char(ch, WEAR_BODY)) != NULL 
         && CAN_SEE_OBJ(tch, obj)) {
            sprintf(buf2, "%s.", 
             smash_article(format_obj_to_char(obj, tch, TRUE)));
            strcat(buf, buf2);
        } else
            strcat(buf, "naked chest.");
        strcat(descr, buf);
    } else if ((obj = get_eq_char(ch, WEAR_BODY)) != NULL
     && CAN_SEE_OBJ(tch, obj)) {
        sprintf(buf, "%s being worn on %s body.", show_eq_to_char(obj, tch, 0),
         HSHR(ch));
        strcat(descr, buf);
    }

    if ((obj = get_eq_char(ch, WEAR_ARMS)) != NULL && CAN_SEE_OBJ(tch, obj)) {
        sprintf(buf, "%s protecting %s arms", show_eq_to_char(obj, tch, FALSE),
         HSHR(ch));
        strcat(descr, buf);

        if ((obj = get_eq_char(ch, WEAR_FOREARMS)) != NULL 
         && CAN_SEE_OBJ(tch, obj)) {
            sprintf(buf, ", and %s on %s forearms.", 
             show_eq_to_char(obj, tch, FALSE), HSHR(ch));
            strcat(descr, buf);
        } else {
            strcat(descr, ".");
        }
    } else if ((obj = get_eq_char(ch, WEAR_FOREARMS)) != NULL 
     && CAN_SEE_OBJ(tch, obj)) {
        sprintf(buf, "%s protecting %s forearms.", 
         show_eq_to_char(obj, tch, FALSE), HSHR(ch));
        strcat(descr, buf);
    } 

    if ((obj = get_eq_char(ch, WEAR_LEGS)) != NULL && CAN_SEE_OBJ(tch, obj)) {
        sprintf(buf, "%s is wearing %s on %s legs.", HSSH(ch), 
         format_obj_to_char(obj, tch, TRUE), HSHR(ch));
        strcat(descr, buf);
    }

    obj = get_eq_char(ch, WEAR_WRIST_L);
    obj2 = get_eq_char(ch, WEAR_WRIST_R);
    if (obj)
        strcpy(tmp1, format_obj_to_char(obj, tch, TRUE));
    if (obj2)
        strcpy(tmp2, format_obj_to_char(obj2, tch, TRUE));

    if (obj && CAN_SEE_OBJ(tch, obj)
     && obj2 && CAN_SEE_OBJ(tch, obj2) && !strcmp(tmp1, tmp2)) {
        sprintf(buf, "%s on each wrist.", show_eq_to_char(obj, tch, 0));
        strcat(descr, buf);
    } else {
        if (obj && CAN_SEE_OBJ(tch, obj)) {
            sprintf(buf, "Encircling %s left wrist, %s", HSHR(ch),
             show_eq_to_char( obj, tch, TRUE ));
            strcat(descr, buf);
            fBool = TRUE;
        }

        if (obj2 && CAN_SEE_OBJ(tch, obj2)) {
            if (fBool)
                sprintf(buf, " and %s on %s right.", 
                 show_eq_to_char(obj2, tch, 0), HSHR(ch));
            else
                sprintf(buf, "Encircling %s right wrist %s.", 
                 HSHR(ch), 
                 show_eq_to_char(obj2, tch, TRUE));
            strcat(descr, buf);
        } else if (fBool)
            strcat(descr, ". ");
    }

    fBool = FALSE;

    obj = get_eq_char(ch, WEAR_OVER_SHOULDER_L);
    obj2 = get_eq_char(ch, WEAR_OVER_SHOULDER_R);
    obj3 = get_eq_char(ch, WEAR_SHOULDER_L);
    obj4 = get_eq_char(ch, WEAR_SHOULDER_R);
    if (obj)
        strcpy(tmp1, format_obj_to_char(obj, tch, TRUE));
    if (obj2)
        strcpy(tmp2, format_obj_to_char(obj2, tch, TRUE));
    if (obj3)
        strcpy(tmp3, format_obj_to_char(obj3, tch, TRUE));
    if (obj4)
        strcpy(tmp4, format_obj_to_char(obj4, tch, TRUE));

    // Two identical items worn over each shoulder (covers on locations)
    if (obj != NULL && CAN_SEE_OBJ(tch, obj)
        && obj2 != NULL && CAN_SEE_OBJ(tch, obj2)
        && !strcmp(tmp1, tmp2)) {
        sprintf(buf, "%s has two %s over %s shoulders.", HSSH(ch),
         pluralize(tmp1), HSHR(ch));
        strcat(descr, buf);
    // Two identical items worn on each shoulder (nothing covering)
    } else if (obj == NULL && obj2 == NULL
	&& obj3 != NULL && CAN_SEE_OBJ(tch, obj3)
        && obj4 != NULL && CAN_SEE_OBJ(tch, obj4)
        && !strcmp(tmp3, tmp4)) {
        sprintf(buf, "%s has two %s on %s shoulders.", HSSH(ch),
         pluralize(tmp1), HSHR(ch));
        strcat(descr, buf);
    } else {
	// Left shoulder first, over then on
        if ((obj = get_eq_char(ch, WEAR_OVER_SHOULDER_L)) != NULL 
         && CAN_SEE_OBJ(tch, obj)) {
            sprintf(buf, "%s over %s left shoulder", 
             show_eq_to_char(obj, tch, FALSE), HSHR(ch));
            strcat(descr, buf);
            fBool = TRUE;
        } else if ((obj = get_eq_char(ch, WEAR_SHOULDER_L)) != NULL 
         && CAN_SEE_OBJ(tch, obj)) {
            sprintf(buf, "%s on %s left shoulder", 
             show_eq_to_char(obj, tch, FALSE), HSHR(ch));
            strcat(descr, buf);
            fBool = TRUE;
        }
	// Right shoulder next, over then on
        if ((obj = get_eq_char(ch, WEAR_OVER_SHOULDER_R)) != NULL
         && CAN_SEE_OBJ(tch, obj)) {
            if (fBool)
                sprintf(buf, " and %s over %s right", 
                 show_eq_to_char(obj, tch, FALSE), HSHR(ch));
            else
                sprintf(buf, "Over %s right shoulder %s.", HSHR(ch),
                 show_eq_to_char(obj, tch, TRUE));
            strcat(descr, buf);
        } else if ((obj = get_eq_char(ch, WEAR_SHOULDER_R)) != NULL
         && CAN_SEE_OBJ(tch, obj)) {
            if (fBool)
                sprintf(buf, " and %s on %s right", 
                 show_eq_to_char(obj, tch, FALSE), HSHR(ch));
            else
                sprintf(buf, "On %s right shoulder %s.", HSHR(ch),
                 show_eq_to_char(obj, tch, TRUE));
            strcat(descr, buf);
        }
        if (fBool)
            strcat(descr, ". ");
    }

    fBool = FALSE;

    obj = get_eq_char(ch, ES);
    obj2 = get_eq_char(ch, EP);
    if (obj)
        strcpy(tmp1, format_obj_to_char(obj, tch, TRUE));
    if (obj2)
        strcpy(tmp2, format_obj_to_char(obj2, tch, TRUE));

    if (obj != NULL && CAN_SEE_OBJ(tch, obj)
        && obj2 != NULL && CAN_SEE_OBJ(tch, obj2)
        && !strcmp(tmp1, tmp2)) {
        if (obj->obj_flags.type == ITEM_WEAPON)
            sprintf(buf, "Two %s are wielded in %s hands.", pluralize(tmp1), HSHR(ch));
        else
            sprintf(buf, "Two %s held in %s hands.", 
             show_eq_to_char(obj,tch,0), HSHR(ch));
        strcat(descr, buf);
    } else {
        oType = -1;
        if ((obj = get_eq_char(ch, ETWO)) != NULL && CAN_SEE_OBJ(tch, obj)) {
            if (obj->obj_flags.type == ITEM_WEAPON) {
                sprintf(buf, "%s being wielded in both hands.",
                 show_eq_to_char(obj, tch, FALSE));
            } else {
                sprintf(buf, "%s being held in both hands.", 
                 show_eq_to_char(obj, tch, TRUE));
            }
            strcat(descr, buf);
        } else {
            if ((obj = get_eq_char(ch, EP)) != NULL && CAN_SEE_OBJ(tch, obj)) {
                if (obj->obj_flags.type == ITEM_WEAPON) {
                    sprintf(buf, "%s being wielded in %s primary hand",
                     show_eq_to_char(obj, tch, FALSE), HSHR(ch));
                    oType = ITEM_WEAPON;
                } else
                    sprintf(buf, "%s held in %s primary hand", 
                     show_eq_to_char(obj, tch, FALSE), HSHR(ch));
                strcat(descr, buf);
                fBool = TRUE;
            }
            if ((obj = get_eq_char(ch, ES)) != NULL && CAN_SEE_OBJ(tch, obj)) {
                if (fBool) {
                    if (oType == ITEM_WEAPON 
                     && obj->obj_flags.type == ITEM_WEAPON)
                        sprintf(buf, ", as well as %s in %s off hand.",
                                format_obj_to_char(obj, tch, TRUE), HSHR(ch));
                    else if (obj->obj_flags.type == ITEM_WEAPON)
                        sprintf(buf, ", and %s wielded in %s off hand.",
                         show_eq_to_char(obj, tch, FALSE),  HSHR(ch));
                    else
                        sprintf(buf, ", and %s held in %s off hand.",
                         show_eq_to_char(obj, tch, FALSE), HSHR(ch));
                } else {
                    if (obj->obj_flags.type != ITEM_WEAPON)
                        sprintf(buf, "%s held in %s off hand.",
                         show_eq_to_char(obj, tch, FALSE), HSHR(ch));
                    else
                        sprintf(buf, "%s being wielded in %s off hand.",
                         show_eq_to_char(obj, tch, FALSE), HSHR(ch));
                }
                strcat(descr, buf);
            } else if (fBool)
                strcat(descr, ".");
        }
    }

    fBool = FALSE;

    if (!((obj = get_eq_char(ch, WEAR_ABOUT)) != NULL 
     && CAN_SEE_OBJ(tch, obj))) {
        if ((obj = get_eq_char(ch, WEAR_WAIST)) != NULL 
         && CAN_SEE_OBJ(tch, obj)) {
            sprintf(buf, "%s fits snuggly around %s waist.", 
             format_obj_to_char(obj, tch, TRUE), HSHR(ch));
            strcat(descr, buf);
        }

        if ((obj = get_eq_char(ch, WEAR_BELT)) != NULL 
         && CAN_SEE_OBJ(tch, obj)) {
            sprintf(buf, "%s worn as a belt", 
             show_eq_to_char(obj, tch, FALSE));

            obj = get_eq_char(ch, WEAR_ON_BELT_1);
            obj2 = get_eq_char(ch, WEAR_ON_BELT_2);
            if (obj)
                strcpy(tmp1, format_obj_to_char(obj, tch, TRUE));
            if (obj2)
                strcpy(tmp2, format_obj_to_char(obj2, tch, TRUE));

            if (obj != NULL && CAN_SEE_OBJ(tch, obj)
             && obj2 != NULL && CAN_SEE_OBJ(tch, obj2)
             && !strcmp(tmp1, tmp2)
             && IS_SET(obj->obj_flags.wear_flags, ITEM_WEAR_BELT)
             && IS_SET(obj2->obj_flags.wear_flags, ITEM_WEAR_BELT)) {
                strcat(descr, buf);
                sprintf(buf, ", two %s hanging from it.", pluralize(tmp1));
                strcat(descr, buf);
            } 
            else if (obj != NULL && CAN_SEE_OBJ(tch, obj)
             && obj2 != NULL && CAN_SEE_OBJ(tch, obj2)
             && !strcmp(tmp1, tmp2)
             && !IS_SET(obj->obj_flags.wear_flags, ITEM_WEAR_BELT)
             && !IS_SET(obj2->obj_flags.wear_flags, ITEM_WEAR_BELT)) {
                strcat(descr, buf);
                sprintf(buf, ", two %s thrust through it.", pluralize(tmp1));
                strcat(descr, buf);
            } 
            else if (obj || obj2) {
                if (obj != NULL && CAN_SEE_OBJ(tch, obj)) {
                    obj = get_eq_char(ch, WEAR_BELT);
                    if( IS_SET(obj->obj_flags.wear_flags, ITEM_WEAR_BELT)) {
                        sprintf(buf, "%s hang%s from the left side of %s", tmp1,
                         strstr(tmp1, "a hilt protrudes") ? "ing" : "s",
                         format_obj_to_char(obj, tch, TRUE));
                    } else {
                        sprintf(buf, "%s %sthrust through the left side of %s",
                         tmp1, strstr(tmp1, "a hilt protrudes") ? "" : "is ",
                         format_obj_to_char(obj, tch, TRUE));
                    }
                    strcat(descr, buf);
                    fBool = TRUE;
                }

                if (obj2 && CAN_SEE_OBJ(tch, obj2)) {
                    obj = get_eq_char(ch, WEAR_BELT);
                    if( IS_SET(obj2->obj_flags.wear_flags, ITEM_WEAR_BELT)) {
                        if (fBool) {
                            sprintf(buf, ", and %s hang%s from the right.",
                             tmp2,
                             strstr(tmp2, "a hilt protrudes") ? "ing" : "s");
                        } else {
                            sprintf(buf, "%s hang%s from the right side of %s.",
                             tmp2,
                             strstr(tmp2, "a hilt protrudes") ? "ing" : "s",
                             format_obj_to_char(obj, tch, TRUE));
                        }
                    }
                    else {
                        if (fBool) {
                            sprintf(buf, ", and %s %sthrust through the right.",
                             tmp2,
                             strstr(tmp2, "a hilt protrudes") ? "" : "is ");
                        } else {
                            sprintf(buf, 
                             "%s %sthrust through the right side of %s.",
                             tmp2,
                             strstr(tmp2, "a hilt protrudes") ? "" : "is ",
                             format_obj_to_char(obj, tch, TRUE));
                        }
                    }
                    strcat(descr, buf);
                } else {
                    strcat(descr, ".");
                }
            } else {
                strcat(descr, buf);
                strcat(descr, ".");
            }
        }
    }

    fBool = FALSE;
    if ((obj = get_eq_char(ch, WEAR_FEET)) != NULL && CAN_SEE_OBJ(tch, obj)) {
        sprintf(buf, "%s worn on %s feet. ", 
         show_eq_to_char(obj, tch, FALSE), HSHR(ch));
        strcat(descr, buf);
    }

    obj = get_eq_char(ch, WEAR_ANKLE);
    obj2 = get_eq_char(ch, WEAR_ANKLE_L);
    if (obj)
        strcpy(tmp1, format_obj_to_char(obj, tch, TRUE));
    if (obj2)
        strcpy(tmp2, format_obj_to_char(obj2, tch, TRUE));

    if (obj && CAN_SEE_OBJ(tch, obj)
     && obj2 && CAN_SEE_OBJ(tch, obj2) && !strcmp(tmp1, tmp2)) {
        sprintf(buf, "Two %s are strapped around %s ankles.", pluralize(tmp1),
         HSHR(ch));
        strcat(descr, buf);
    } else {
        if (obj && CAN_SEE_OBJ(tch, obj)) {
            sprintf(buf, "Strapped around %s right ankle, %s", HSHR(ch),
             show_eq_to_char( obj, tch, TRUE ));
            strcat(descr, buf);
            fBool = TRUE;
        }

        if (obj2 && CAN_SEE_OBJ(tch, obj2)) {
            if (fBool)
                sprintf(buf, " and %s on %s left.",
                 show_eq_to_char(obj2, tch, 0), HSHR(ch));
            else
                sprintf(buf, "Strapped around %s left ankle %s.",
                 HSHR(ch),
                 show_eq_to_char(obj2, tch, TRUE));
            strcat(descr, buf);
        } else if (fBool)
            strcat(descr, ".");
    }

    fBool = FALSE;


    if (strcmp(descr, "   ")) {
        send_to_char("\n\r", tch);
        finalstr = format_string(strdup(descr));
        send_to_char(finalstr, tch);
        free(finalstr);
    }

    return;
}
