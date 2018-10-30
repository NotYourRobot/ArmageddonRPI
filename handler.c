/*
 * File: HANDLER.C
 * Usage: Routines for moving around objects and characters.
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

#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <assert.h>
#include <stdlib.h>

#include <stdarg.h>

#include "cities.h"
#include "constants.h"
#include "dictionary.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "creation.h"
#include "db.h"
#include "handler.h"
#include "guilds.h"
#include "clan.h"
#include "skills.h"
#include "vt100c.h"
#include "event.h"
#include "parser.h"
#include "psionics.h"
#include "modify.h"
#include "utility.h"
#include "object_list.h"
#include "wagon_save.h"
#include "memory.h"
#include "watch.h"
#include "reporting.h"

int add_new_item_in_creation(int virt_nr);

/* returns TRUE if it worked, FALSE if not ...all pointers
   passed here must have been created elsewhere */

typedef enum {
    ET_ROOM,
    ET_OBJ,
    ET_CHAR
} et_type;

typedef struct __et_thing {
    et_type type;
    union {
        ROOM_DATA *rm;
        OBJ_DATA *obj;
        CHAR_DATA *ch;
    };
    struct __et_thing *next;
} extracted_thing;

extracted_thing *et_list;

bool
money_mgr(int type, int amt, void *ref1, void *ref2)
{
    CHAR_DATA *ch, *tch;
    OBJ_DATA *pouch, *coins;

    switch (type) {
    case MM_CH2OBJ:
        ch = (CHAR_DATA *) ref1;

        if ((amt <= 0) || (amt > GET_OBSIDIAN(ch)))
            return FALSE;

        GET_OBSIDIAN(ch) -= amt;

        if (!(coins = create_money(amt, COIN_ALLANAK))) {
            gamelog("money_mgr: unable to create coins object");
            return FALSE;
        }

        obj_to_room(coins, ch->in_room);

        break;
    case MM_CH2POUCH:
        ch = (CHAR_DATA *) ref1;

        pouch = (OBJ_DATA *) ref2;

        if ((amt <= 0) || (amt > GET_OBSIDIAN(ch)))
            return FALSE;

        GET_OBSIDIAN(ch) -= amt;

        if (!(coins = create_money(amt, COIN_ALLANAK))) {
            gamelog("money_mgr: unable to create coins object");
            return FALSE;
        }

        obj_to_obj(coins, pouch);
        break;
    case MM_OBJ2CH:
        coins = (OBJ_DATA *) ref1;
        ch = (CHAR_DATA *) ref2;
        if (coins->obj_flags.type != ITEM_MONEY)
            return FALSE;

        /*
         * extract obj will do these
         * obj_from_obj(coins);
         * obj_from_char(coins);
         * obj_from_room(coins);
         */
        GET_OBSIDIAN(ch) += coins->obj_flags.value[0];
        extract_obj(coins);
        ref1 = 0;
        break;
    case MM_CH2CH:
        ch = (CHAR_DATA *) ref1;
        tch = (CHAR_DATA *) ref2;
        if (GET_OBSIDIAN(ch) < amt)
            return FALSE;
        GET_OBSIDIAN(ch) -= amt;
        GET_OBSIDIAN(tch) += amt;
        break;
    default:
        return FALSE;
    }
    return TRUE;
}


/* equipment searchers */
OBJ_DATA *
get_object_in_equip_vis(CHAR_DATA * ch, char *arg, OBJ_DATA * equipment[], int *j)
{
    int number, c = 0;
    char name[MAX_INPUT_LENGTH];
    const char *tmp;

    strcpy(name, arg);
    tmp = name;
    if (!(number = get_number(&tmp)))
        return NULL;

    for ((*j) = 0; (*j) < MAX_WEAR; (*j)++)
        if (equipment[(*j)])
            if (char_can_see_obj(ch, equipment[(*j)]))
                if (isname(tmp, OSTR(equipment[(*j)], name))
                    || isname(tmp, real_sdesc_keywords_obj(equipment[(*j)], ch)))
                    if (++c == number)
                        return (equipment[(*j)]);

    return NULL;
}

OBJ_DATA *
get_object_in_equip(char *arg, OBJ_DATA * equipment[], int *j)
{
    int number, c = 0;
    char name[MAX_INPUT_LENGTH];
    const char *tmp;

    strcpy(name, arg);
    tmp = name;
    if (!(number = get_number(&tmp)))
        return NULL;

    for ((*j) = 0; (*j) < MAX_WEAR; (*j)++)
        if (equipment[(*j)])
            if (isname(tmp, OSTR(equipment[(*j)], name))
                || isname(tmp, real_sdesc_keywords_obj(equipment[(*j)], equipment[(*j)]->carried_by)))
                if (++c == number)
                    return (equipment[(*j)]);

    return NULL;
}


/* function for saving throws */
bool
does_save(CHAR_DATA * ch, int save_type, int mod)
{
    int save_target;
    int roll;
    char buf[MAX_STRING_LENGTH];

    if (IS_SET(ch->specials.nosave, NOSAVE_MAGICK))
        return FALSE;

    if ((save_type < 0) || (save_type > 4))
        return TRUE;

    save_target = ch->specials.apply_saving_throw[save_type];

    if ((save_type == SAVING_ROD) || (save_type == SAVING_BREATH)) {
        if (IS_AFFECTED(ch, CHAR_AFF_BLIND))
            save_target -= 20;
        if (affected_by_spell(ch, SPELL_ELEMENTAL_FOG) && has_sight_spells_resident(ch))
            save_target -= 20;
        if (IS_AFFECTED(ch, CHAR_AFF_DEAFNESS))
            save_target -= 15;
    }

    if ((save_type == SAVING_PARA) &&   /* Being used since there is no POISON */
        ((GET_RACE(ch) == RACE_HALFLING) || (GET_RACE(ch) == RACE_VAMPIRE)))
        return TRUE;

    save_target = MAX(save_target, 1);
    if (IS_IMMORTAL(ch)) {
        sprintf(buf, "%d is your save number.\n\r", save_target);
        send_to_char(buf, ch);
        return TRUE;
    }

    roll = number(1, 100);

    save_target = MAX(5, MIN(95, save_target + mod));

    sprintf(buf, "%s rolled %d and has save of %d.", MSTR(ch, short_descr), roll, save_target);
    shhlog(buf);

    return (roll <= (save_target));
}

/* function for determining generic skill success */
bool
skill_success(CHAR_DATA * ch, CHAR_DATA * tar_ch, int learn_percent)
{
    int chance;

    if (IS_IMMORTAL(ch)) {
        send_to_char("Immortals always succeed.\n\r", ch);
        return TRUE;
    }

    chance = number(1, 100);

    if (tar_ch) {
        // If the target is an NPC, or of lower level than the doer, then
        // immortals always succeed.
        if ((IS_IMMORTAL(ch) && IS_NPC(tar_ch))
            || (IS_IMMORTAL(ch) && GET_LEVEL(ch) > GET_LEVEL(tar_ch))) {
            send_to_char("Immortals always succeed.\n\r", ch);
            return TRUE;
        }
        // Against higher level immortals, you fail
        if (IS_IMMORTAL(ch) && GET_LEVEL(ch) <= GET_LEVEL(tar_ch)) {
            send_to_char("Ooops, immortals don't always succeed against other immortals.\n\r", ch);
            return FALSE;
        }

        if (IS_SET(tar_ch->specials.nosave, NOSAVE_SKILLS))
            return (TRUE);
        if (GET_POS(tar_ch) <= POSITION_SLEEPING)
            return (TRUE);
        if (IS_AFFECTED(tar_ch, CHAR_AFF_SUBDUED))
            return (TRUE);

        chance -= (15 * (POSITION_STANDING - GET_POS(tar_ch)));
        if (!CAN_SEE(tar_ch, ch))
            chance += 25;
        if (IS_AFFECTED(tar_ch, CHAR_AFF_SILENCE))
            chance += 15;
        if (affected_by_spell(ch, PSI_MINDWIPE))
            chance -= 25;
        /* Hallucinatory drug that affects the brain */
        if (affected_by_spell(ch, POISON_SKELLEBAIN))
            chance -= 30;
        /* Hallucinatory drug that affects the brain phase 2*/
        if (affected_by_spell(ch, POISON_SKELLEBAIN_2))
            chance -= 40;
        /* Hallucinatory drug that affects the brain phase 3 */
        if (affected_by_spell(ch, POISON_SKELLEBAIN_3))
            chance -= 50;
        chance = MAX(chance, 1);
    }
    return (learn_percent >= chance);
}


char *
first_name(const char *namelist, char *into)
{
    register char *point;

    for (point = into; isalpha(*namelist); namelist++, point++)
        *point = *namelist;

    *point = '\0';

    return (into);
}

int
isname(const char *str, const char *namelist)
{
    register const char *curname, *curstr;

    if (!str || !namelist)
        return 0;
    if (!*str && !*namelist)
        return 1;
    if (!*str || !*namelist)
        return 0;

    if( index(str, '.') != NULL ) {
       char buf[MAX_STRING_LENGTH];
       replace_char(str, '.', ' ', buf, sizeof(buf));
       return isallname(buf, namelist);
    }

    curname = namelist;
    for (;;) {
        if (*curname == '[') {
            /* advance to either EOL or ] */
            for (; *curname && *curname != ']'; curname++) {
            };
        };

        for (curstr = str;; curstr++, curname++) {
            /* if (!*curstr && !isalpha (*curname)) -- Old version: Tiernan */
            if (!*curstr && !isalnum(*curname))
                return (1);

            if (!*curname)
                return (0);

            if (!*curstr || *curname == ' ')
                break;

            if (LOWER(*curstr) != LOWER(*curname))
                break;
        }

        /* skip to next name */

        for (; (*curname == '[') || isalpha(*curname); curname++);
        if (!*curname)
            return (0);

        curname++;              /* first char of new name */
    }
}

int
isallname(const char *str, const char *namelist)
{
    size_t len;
    char temp_string[MAX_INPUT_LENGTH];

    if (!str || !namelist)
        return 0;
    if (!*str && !*namelist)
        return 1;
    if (!*str || !*namelist)
        return 0;

    len = strlen(str);
    if (len >= MAX_INPUT_LENGTH) {
        gamelog("isallname: STRING TO LONG");
        return 0;
    }

    /* get the first word out of temp_str, and put it in str */
    while ((str = one_argument(str, temp_string, sizeof(temp_string))), strlen(temp_string)) {
        if (!isname(temp_string, namelist))
            return (FALSE);
    }
    return (TRUE);
}

int
isnameprefix(const char *str, const char *namelist)
{
    register const char *curname, *curstr;

    if (!str || !namelist)
        return 0;
    if (!*str && !*namelist)
        return 1;
    if (!*str || !*namelist)
        return 0;

    curname = namelist;
    for (;;) {
        for (curstr = str;; curstr++, curname++) {
            if (!*curstr)
                return (1);

            if (!*curname)
                return (0);

            if (!*curstr || *curname == ' ')
                break;

            if (LOWER(*curstr) != LOWER(*curname))
                break;
        }

        /* skip to next name */

        for (; isalpha(*curname); curname++);
        if (!*curname)
            return (0);
        curname++;              /* first char of new name */
    }
}

int
isnamebracketsok(const char *str, const char *namelist)
{
    register const char *curname, *curstr;

    if (!str || !namelist)
        return 0;
    if (!*str && !*namelist)
        return 1;
    if (!*str || !*namelist)
        return 0;

    curname = namelist;
    for (;;) {
        for (curstr = str;; curstr++, curname++) {
            if (!*curstr && !isalpha(*curname))
                return (1);

            if (!*curname)
                return (0);

            if (!*curstr || *curname == ' ')
                break;

            if (LOWER(*curstr) != LOWER(*curname))
                break;
        }

        /* skip to next name */

        for (; isalpha(*curname); curname++);
        if (!*curname)
            return (0);
        curname++;              /* first char of new name */
    }
}

int
isallnamebracketsok(const char *str, const char *namelist)
{
    size_t len;
    char temp_string[MAX_INPUT_LENGTH];

    if (!str || !namelist)
        return 0;
    if (!*str && !*namelist)
        return 1;
    if (!*str || !*namelist)
        return 0;

    len = strlen(str);
    if (len >= MAX_INPUT_LENGTH) {
        gamelog("isallnamebracketsok:STRING TO LONG");
        return 0;
    }

    /* get the first word out of temp_str, and put it in str */
    while ((str = one_argument(str, temp_string, sizeof(temp_string))), strlen(temp_string)) {
        if (!isnamebracketsok(temp_string, namelist))
            return (FALSE);
    };
    return (TRUE);
}

int
isanynamebracketsok(const char *str, const char *namelist)
{
    size_t len;
    char temp_string[MAX_INPUT_LENGTH];

    if (!str || !namelist)
        return 0;
    if (!*str && !*namelist)
        return 1;
    if (!*str || !*namelist)
        return 0;

    len = strlen(str);
    if (len >= MAX_INPUT_LENGTH) {
        gamelog("isanynamebracketsok:STRING TO LONG");
        return 0;
    }

    /* get the first word out of temp_str, and put it in str */
    while ((str = one_argument(str, temp_string, sizeof(temp_string))), strlen(temp_string)) {
        if (isnamebracketsok(temp_string, namelist))
            return (TRUE);
    };
    return (FALSE);
}

int NEW_KEYWORD_SYSTEM = FALSE;

bool
check_char_name(CHAR_DATA * ch, CHAR_DATA * tar_ch, const char *arg)
{
    int vis = 0;
    char *visible_keywords = NULL;
    bool found = FALSE;
 
    if (!*arg)
        return FALSE;

    if (!tar_ch)
        return FALSE;

    if ((ch == tar_ch) && (!stricmp(arg, "me") || !stricmp(arg, "self")))
        return TRUE;

    if (!(vis = CAN_SEE(ch, tar_ch)))
        return FALSE;

    // Truename has power in that it always works no matter what
    if (NEW_KEYWORD_SYSTEM ) {
      // If you are staff or NPC -Morg
      if (!IS_PLAYER(ch) && isname(arg, MSTR(tar_ch, name)))
        return TRUE;
    } else {
      if (isname(arg, MSTR(tar_ch, name)))
        return TRUE;
    }

    /* Ought to be just keywords but for all those old PCs who have their
     * short_descr keywords in their extkwds, we have to exclude them.
     */
    if (isname(arg, tar_ch->player.extkwds))
        return TRUE;

    // visible sdesc keywords
    visible_keywords = generate_keywords_from_string(real_sdesc(tar_ch, ch, vis));
    found = isname(arg, visible_keywords);
    free(visible_keywords); // Good housekeeping
    if (found)
        return TRUE;

    if ((ch != tar_ch) && (GET_SEX(tar_ch) == SEX_MALE) && !stricmp(arg, "him"))
        return TRUE;

    if ((ch != tar_ch) && (GET_SEX(tar_ch) == SEX_FEMALE) && !stricmp(arg, "her"))
        return TRUE;

    if ((ch != tar_ch) && (GET_SEX(tar_ch) == SEX_NEUTER) && !stricmp(arg, "it"))
        return TRUE;

    return FALSE;
}

bool
check_char_name_world(CHAR_DATA * ch, CHAR_DATA * tar_ch, const char *arg) 
{
   return check_char_name_world_raw(ch, tar_ch, arg, TRUE);
}

bool
check_char_name_world_raw(CHAR_DATA * ch, CHAR_DATA * tar_ch, const char *arg, bool useCurrSdesc)
{
    if (!*arg)
        return FALSE;

    if (!tar_ch)
        return FALSE;

    if (isname(arg, MSTR(tar_ch, name)) || isname(arg, tar_ch->player.extkwds)
        || (useCurrSdesc && isname(arg, (char *) PERS(ch, tar_ch)))
        || isname(arg, MSTR(tar_ch, short_descr)))
        return TRUE;

    if ((ch == tar_ch) && (!stricmp(arg, "me") || !stricmp(arg, "self")))
        return TRUE;

    if ((ch != tar_ch) && (GET_SEX(tar_ch) == SEX_MALE) && !stricmp(arg, "him"))
        return TRUE;

    if ((ch != tar_ch) && (GET_SEX(tar_ch) == SEX_FEMALE) && !stricmp(arg, "her"))
        return TRUE;

    if ((ch != tar_ch) && (GET_SEX(tar_ch) == SEX_NEUTER) && !stricmp(arg, "it"))
        return TRUE;

    return FALSE;
}

bool
check_char_name_world_all_name(CHAR_DATA * ch, CHAR_DATA * tar_ch, char *arg)
{
   return check_char_name_world_all_name_raw(ch, tar_ch, arg, TRUE);
}


bool
check_char_name_world_all_name_raw(CHAR_DATA * ch, CHAR_DATA * tar_ch, char *arg, bool useCurrSdesc)
{
    if (!*arg)
        return FALSE;

    if (!tar_ch)
        return FALSE;

    if (isallname(arg, MSTR(tar_ch, name))
        || isallname(arg, tar_ch->player.extkwds)
        || (useCurrSdesc && isallname(arg, (char *) PERS(ch, tar_ch)))
        || isallname(arg, MSTR(tar_ch, short_descr)))
        return TRUE;

    if ((ch == tar_ch) && (!stricmp(arg, "me") || !stricmp(arg, "self")))
        return TRUE;

    if ((ch != tar_ch) && (GET_SEX(tar_ch) == SEX_MALE) && !stricmp(arg, "him"))
        return TRUE;

    if ((ch != tar_ch) && (GET_SEX(tar_ch) == SEX_FEMALE) && !stricmp(arg, "her"))
        return TRUE;

    if ((ch != tar_ch) && (GET_SEX(tar_ch) == SEX_NEUTER) && !stricmp(arg, "it"))
        return TRUE;

    return FALSE;
}

void
make_sure_obj_is_nowhere(OBJ_DATA * obj, char *when)
{
    int i;

    if (obj->in_room) {
        errorlogf("Object %s was in a room at point %s." " removing from room",
                OSTR(obj, short_descr), when);
        obj_from_room(obj);
    }

    if (obj->in_room) {
        errorlog("It's still in a room, check this bug out.");
        obj->in_room = 0;
    }

    if (obj->carried_by) {
        errorlogf("Object %s was carried_by at point %s.", OSTR(obj, short_descr),
                when);
        obj_from_char(obj);
    }

    if (obj->carried_by) {
        errorlog("It's still carried_by, check this bug out.");
        obj->carried_by = 0;
    }

    if (obj->in_obj) {
        errorlogf("Object %s was in_obj at point %s.", OSTR(obj, short_descr), when);
        obj_from_obj(obj);
    }

    if (obj->in_obj) {
        errorlog("It's still in_obj, check this bug out.");
        obj->in_obj = 0;
    }

    if (obj->equipped_by) {
        errorlogf("Object %s was in_equip at point %s.", OSTR(obj, short_descr), when);
        for (i = 0; (i < MAX_WEAR) && obj->equipped_by; i++)
            if (obj->equipped_by->equipment[i] == obj) {
                unequip_char(obj->equipped_by, i);
            }
        if (i == MAX_WEAR) {
            gamelog("It wasnt even in that persons equipment.");
            obj->equipped_by = 0;
        }
    }
    if (obj->equipped_by) {
        errorlog("It was still equipped_by, check this out.");
        obj->equipped_by = 0;
    }
}

int
affect_bonus(OBJ_DATA * obj, int loc)
{
    int j;

    for (j = 0; j < MAX_OBJ_AFFECT; j++)
        if (obj->affected[j].location == loc)
            break;

    if (j == MAX_OBJ_AFFECT)
        return (0);
    else
        return (obj->affected[j].modifier);

}



void
affect_modify(CHAR_DATA * ch, int loc, long mod, long bitv, bool add)
{
    int maxabil;
    char buf[MAX_STRING_LENGTH];

    if (add) {
        MUD_SET_BIT(ch->specials.affected_by, bitv);
    } else {
        REMOVE_BIT(ch->specials.affected_by, bitv);
        mod = -mod;
    }

    maxabil = 20;

    switch (loc) {

    case CHAR_APPLY_NONE:
        break;

    case CHAR_APPLY_STR:
        SET_STR(ch, GET_STR(ch) + mod);
        break;

    case CHAR_APPLY_AGL:
        SET_AGL(ch, GET_AGL(ch) + mod);
        break;

    case CHAR_APPLY_WIS:
        SET_WIS(ch, GET_WIS(ch) + mod);
        break;

    case CHAR_APPLY_END:
        SET_END(ch, GET_END(ch) + mod);
        break;

    case CHAR_APPLY_THIRST:
    case CHAR_APPLY_HUNGER:
        break;

    case CHAR_APPLY_MANA:
        ch->points.mana_bonus += mod;
        break;

    case CHAR_APPLY_HIT:
        ch->points.max_hit += mod;
        break;

    case CHAR_APPLY_STUN:
        ch->points.max_stun += mod;
        break;

    case CHAR_APPLY_MOVE:
        ch->points.move_bonus += mod;
        break;

    case CHAR_APPLY_OBSIDIAN:
    case CHAR_APPLY_DAMAGE:
        break;

    case CHAR_APPLY_AC:
        SET_ARMOR(ch, GET_ARMOR(ch) + mod);
        break;

    case CHAR_APPLY_OFFENSE:
        SET_OFF(ch, GET_OFF(ch) + mod);
        break;

    case CHAR_APPLY_SAVING_PARA:
        ch->specials.apply_saving_throw[0] += mod;
        break;

    case CHAR_APPLY_SAVING_ROD:
        ch->specials.apply_saving_throw[1] += mod;
        break;

    case CHAR_APPLY_SAVING_PETRI:
        ch->specials.apply_saving_throw[2] += mod;
        break;

    case CHAR_APPLY_SAVING_BREATH:
        ch->specials.apply_saving_throw[3] += mod;
        break;

    case CHAR_APPLY_SAVING_SPELL:
        ch->specials.apply_saving_throw[4] += mod;
        break;

    case CHAR_APPLY_DETOX:
    case CHAR_APPLY_SKILL_SNEAK:
    case CHAR_APPLY_SKILL_HIDE:
    case CHAR_APPLY_SKILL_LISTEN:
    case CHAR_APPLY_SKILL_CLIMB:
        break;

    case CHAR_APPLY_DEFENSE:
        SET_DEF(ch, GET_DEF(ch) + mod);
        break;

    case CHAR_APPLY_CFLAGS:
        if (mod > 0)
            MUD_SET_BIT(ch->specials.act, mod);
        else if (mod < 0)
            REMOVE_BIT(ch->specials.act, -mod);
        break;

    case CHAR_APPLY_PSI:
        break;

    default:

        /* set skill modifier, if is in range */
        if ((loc >= CHAR_APPLY_SKILL_OFFSET) && (loc < (CHAR_APPLY_SKILL_OFFSET + MAX_SKILLS))) {
            int skill;
            skill = loc - CHAR_APPLY_SKILL_OFFSET;

            /* if (has_skill(ch,skill))   
             * This might not be a good idea, because if they don't have the skill, wear an item
             * to modify it, and learn the skill, the remove the item, will have a negative
             * skill */
            set_skill_percentage(ch, skill, get_skill_percentage(ch, skill) + mod);

            break;
        } else {
            sprintf(buf, "[%s] Unknown apply adjust attempt (handler.c, affect_modify).",
                    MSTR(ch, name));
            shhlog(buf);
            break;
        }
    }                           /* switch */
}


// used by get_raw_skill to pull the modifier if the skill matches the
// requested skill
int
pull_skill_affect( int curr_skill, int sk_num, int loc, int mod ) {
    if (loc >= CHAR_APPLY_SKILL_OFFSET 
     && loc < (CHAR_APPLY_SKILL_OFFSET + MAX_SKILLS)
     && sk_num == loc - CHAR_APPLY_SKILL_OFFSET ) {
        curr_skill -= mod;
    }

    return curr_skill;
}

// get the character's raw skill (without object/spell affects)
int
get_raw_skill(CHAR_DATA *ch, int sk_num ) {
    int skill = 0;
    struct affected_type *af;
    int i, j;

    if( !ch || sk_num < 0 || sk_num >= MAX_SKILLS ) {
        return 0;
    }

    skill = get_skill_percentage(ch, sk_num);

    // loop over
    for( i = 0; i < MAX_WEAR; i++ ) {
        if (ch->equipment[i] 
         && i != WEAR_ON_BACK && i != WEAR_BACK && i != WEAR_ON_BELT_1
         && i != WEAR_ON_BELT_2) {
            for (j = 0; j < MAX_OBJ_AFFECT; j++) {
               skill = pull_skill_affect( skill, sk_num, 
                ch->equipment[i]->affected[j].location, 
                ch->equipment[i]->affected[j].modifier );
            }
        }
    }

    for (af = ch->affected; af; af = af->next)
        skill = pull_skill_affect( skill, sk_num, af->location, af->modifier );

    return skill;
}




/* This updates a character by subtracting everything he is affected by */
/* restoring original abilities, and then affecting all again           */

void
affect_total(CHAR_DATA * ch)
{
    struct affected_type *af;
    int i, j;

    for (i = 0; i < MAX_WEAR; i++) {
        if ((ch->equipment[i]) && (i != WEAR_ON_BACK) && (i != WEAR_BACK) && (i != WEAR_ON_BELT_1)
            && (i != WEAR_ON_BELT_2))
            for (j = 0; j < MAX_OBJ_AFFECT; j++)
                affect_modify(ch, ch->equipment[i]->affected[j].location,
                              ch->equipment[i]->affected[j].modifier,
                              ch->equipment[i]->obj_flags.bitvector, FALSE);
    }


    for (af = ch->affected; af; af = af->next)
        affect_modify(ch, af->location, af->modifier, af->bitvector, FALSE);

    ch->tmpabilities = ch->abilities;

    for (i = 0; i < MAX_WEAR; i++) {
        if ((ch->equipment[i]) && (i != WEAR_ON_BACK) && (i != WEAR_BACK) && (i != WEAR_ON_BELT_1)
            && (i != WEAR_ON_BELT_2))
            for (j = 0; j < MAX_OBJ_AFFECT; j++)
                affect_modify(ch, ch->equipment[i]->affected[j].location,
                              ch->equipment[i]->affected[j].modifier,
                              ch->equipment[i]->obj_flags.bitvector, TRUE);
    }

    for (af = ch->affected; af; af = af->next)
        affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);

    /* Make certain values are between 0..100, not < 0 and not > 100! */
    SET_STR(ch, MAX(0, MIN(GET_STR(ch), MAX_ABILITY)));
    SET_AGL(ch, MAX(0, MIN(GET_AGL(ch), MAX_ABILITY)));
    SET_WIS(ch, MAX(0, MIN(GET_WIS(ch), MAX_ABILITY)));
    SET_END(ch, MAX(0, MIN(GET_END(ch), MAX_ABILITY)));
}



/* Insert an affect_type in a char_data structure
   Automatically sets apropriate bits and apply's */
void
affect_to_char(CHAR_DATA * ch, struct affected_type *af)
{
    struct affected_type *affected_alloc;

    CREATE(affected_alloc, struct affected_type, 1);

    memcpy(affected_alloc, af, sizeof(struct affected_type));
    affected_alloc->initiation = time(NULL);
    affected_alloc->expiration =
        affected_alloc->initiation + (RT_ZAL_HOUR * affected_alloc->duration) - 1;
    affected_alloc->next = ch->affected;
    ch->affected = affected_alloc;

    affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);
    affect_total(ch);
}



/* Remove an affected_type structure from a char (called when duration
   reaches zero). Pointer *af must never be NIL! Frees mem and calls
   affect_location_apply                                                */
void
affect_remove(CHAR_DATA * ch, struct affected_type *af)
{
    struct affected_type *hjp;

    assert(ch);
    assert(ch->affected);
    assert(af);

    affect_modify(ch, af->location, af->modifier, af->bitvector, FALSE);

    if (af->type == PSI_HEAR)
        remove_listener(ch);

    // If we're removing a mood altering spell, clear their mood
    if( af->type == SPELL_FURY
     || af->type == AFF_MUL_RAGE
     || af->type == SPELL_CALM
     || af->type == SPELL_FEAR ) {
       change_mood(ch, NULL);
    }

    // if possess corpse, return
    if (af->type == SPELL_POSSESS_CORPSE)
        cmd_return(ch, "", 0, 0);

    // If we're removing the planeshift spell, send them back to where they left
    if (af->type == SPELL_PLANESHIFT) 
	    planeshift_return(ch);

    /* remove structure *af from linked list */

    if (ch->affected == af)     /* remove head of list */
        ch->affected = af->next;
    else {
        for (hjp = ch->affected; (hjp->next) && (hjp->next != af); hjp = hjp->next);

        if (hjp->next != af) {
            gamelog
                ("FATAL : Could not locate affected_type in ch->affected. (handler.c, affect_remove)");
            exit(1);
        }
        hjp->next = af->next;   /* skip the af element */
    }

    // This must happen after the affect is removed, or we will recurse.
    if (af->type == SPELL_RECITE)
        remove_contact(ch);

    // If we're removing the burrow spell, make them stand up after removal
    extern bool stand_from_burrow(CHAR_DATA * ch, bool forced);
    if(af->type == SPELL_BURROW)
        stand_from_burrow(ch, TRUE); // TRUE because the affect is removed

    free(af);
    affect_total(ch);
}

/* Call remove_all_affects_remove to remove all affects upon a character */
void
remove_all_affects_from_char(CHAR_DATA * ch)
{
    struct affected_type *hjp;

    while ((hjp = ch->affected))
        affect_remove(ch, hjp);
}

void
affect_loc_from_char(CHAR_DATA * ch, int skill, int location)
{
    struct affected_type *hjp, *pp;

    hjp = ch->affected;
    while (hjp) {
        pp = hjp;
        hjp = hjp->next;
        if ((hjp->type == skill) && (hjp->location == location))
            affect_remove(ch, hjp);
    }

}

/* Call affect_remove with every spell of spelltype "skill" */
void
affect_from_char(CHAR_DATA * ch, int skill)
{
    struct affected_type *hjp;
    struct affected_type *next_hjp;

    for (hjp = ch->affected; hjp; hjp = next_hjp) {
        next_hjp = hjp->next;
        if (hjp->type == skill)
            affect_remove(ch, hjp);
    }

}

/* Return if a char is affected by a spell (SPELL_XXX)
 * returns the affect if found, otherwise NULL indicates not affected 
 */
struct affected_type *
affected_by_spell(CHAR_DATA * ch, int skill)
{
    struct affected_type *aff_node = NULL;

    if (ch->affected)
        for (aff_node = ch->affected; aff_node; aff_node = aff_node->next)
            if (aff_node->type == skill)
                break;

    return (aff_node);
}

int
affected_by_flags(CHAR_DATA * ch, int flag)
{
    struct affected_type *af;
    int i = FALSE;

    for (af = ch->affected; af; af = af->next) {
        if (af->bitvector & flag) {
            i = TRUE;
            break;
        }
    }

    return i;
}

int
affected_by_type_in_location(CHAR_DATA * ch, int type, int loc)
{
    struct affected_type *af;
    int i = FALSE;

    for (af = ch->affected; af; af = af->next) {
        if ((af->type == type) && (af->location == loc)) {
            i = TRUE;
            break;
        }
    }

    return i;
}

int
affected_by_type_in_bitvector(CHAR_DATA * ch, int type, int bvt)
{
    struct affected_type *af;
    int i = FALSE;

    for (af = ch->affected; af; af = af->next) {
        if ((af->type == type) && (af->bitvector == bvt)) {
            i = TRUE;
            break;
        }
    }

    return i;
}

int
find_apply_duration_from_location(CHAR_DATA * ch, int type, int loc)
{
    struct affected_type *af;
    int i = 0;

    for (af = ch->affected; af; af = af->next) {
        /* If multiple instances, the modifier is increased */
        if ((af->type == type) && (af->location == loc))
            i += af->duration;
    }

    return i;
}

int
find_apply_power_from_location(CHAR_DATA * ch, int type, int loc)
{
    struct affected_type *af;
    int i = 0;

    for (af = ch->affected; af; af = af->next) {
        /* If multiple instances, the modifier is increased */
        if ((af->type == type) && (af->location == loc))
            i += af->power;
    }

    return i;
}

int
find_apply_modifier_from_location(CHAR_DATA * ch, int type, int loc)
{
    struct affected_type *af;
    int i = 0;
    char buf[MAX_STRING_LENGTH];

    for (af = ch->affected; af; af = af->next) {
        /* If multiple instances, the modifier is increased */
        if ((af->type == type) && (af->location == loc))
            i += af->modifier;
    }

    sprintf(buf, "modifier is: %d", i);
/*
  gamelog (buf);
*/
    return (i);
}

void
affect_join(CHAR_DATA * ch, struct affected_type *af, bool avg_dur, bool avg_mod)
{
    struct affected_type *hjp, *next;
    bool found = FALSE;

    for (hjp = ch->affected; !found && hjp; hjp = next) {
        next = hjp->next;
        if (hjp->type == af->type) {
            af->duration += hjp->duration;
            if (avg_dur)
                af->duration /= 2;

            af->modifier += hjp->modifier;
            if (avg_mod)
                af->modifier /= 2;

            affect_remove(ch, hjp);
            affect_to_char(ch, af);
            found = TRUE;
        }
    }
    if (!found)
        affect_to_char(ch, af);
}

int
num_room_lights(ROOM_DATA * rm)
{
    int i = 0;
    CHAR_DATA *ch;
    OBJ_DATA *obj;

    if (!rm) {
        gamelog("Null value 'rm' sent to num_room_lights() ");
        return 0;
    }

    for (ch = rm->people; ch; ch = ch->next_in_room) {
        if (((obj = ch->equipment[EP]) || (obj = ch->equipment[ES])
             || (obj = ch->equipment[WEAR_ABOUT_HEAD])) && obj->obj_flags.type == ITEM_LIGHT) {
            /* gamelog ("Light found."); */
            i++;
        }
    }

    if (i < 0) {
        rm->light = 0;
        i = 0;
        gamelog("Room lights were less than 0.  Setting to 0.");
    }

    if (i != rm->light) {
        rm->light = i;
        gamelog("Room lights were incorrect.  Correcting.");
    }

    return i;
}



/* base things needed to remove a character from a room */
/* includes no manipulation of lights, or status (such as riding) */
bool
remove_char_from_room(CHAR_DATA * ch)
{
    if (!ch || !ch->in_room)
        return FALSE;

    if (ch->prev_in_room)
        ch->prev_in_room->next_in_room = ch->next_in_room;

    if (ch->next_in_room)
        ch->next_in_room->prev_in_room = ch->prev_in_room;

    if (ch == ch->in_room->people)
        ch->in_room->people = ch->next_in_room;

    ch->in_room = NULL;

    ch->prev_in_room = NULL;
    ch->next_in_room = NULL;
    return TRUE;
}


/* move a player out of a room */
void
char_from_room_move(CHAR_DATA * ch)
{
    ROOM_DATA *rm;

    if (!ch || !ch->in_room)
        return;

    rm = ch->in_room;

    /* if (is_char_falling (ch)) 
     * remove_char_from_falling (ch); */

    if (ch->on_obj) {
        show_remove_occupant(ch, "", "");
    }

    ch_lights_from_room(ch);

    /* do the actual removal */
    remove_char_from_room(ch);

    /*  num_room_lights(rm); */

    /* only removing a character from a room caused by movement should
     * wipe the ldesc  -Morg
     */
    WIPE_LDESC(ch);
}

/* move a player out of a room */
void
char_from_room(CHAR_DATA * ch)
{
    CHAR_DATA *i;
    ROOM_DATA *rm;

    if (!ch->in_room)
        return;

    rm = ch->in_room;

    /* if (is_char_falling (ch))
     * remove_char_from_falling (ch); */

    for (i = ch->in_room->people; i; i = i->next_in_room)
        if (i->specials.subduing && i->specials.subduing == ch)
            break;

    if (i) {
        i->specials.subduing = NULL;
        REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_SUBDUED);
    }

    if (ch->specials.subduing) {
        REMOVE_BIT(ch->specials.subduing->specials.affected_by, CHAR_AFF_SUBDUED);
        ch->specials.subduing = NULL;
    }

    for (i = ch->in_room->people; i; i = i->next_in_room)
        if (i->specials.riding == ch)
            break;
    if (i)
        i->specials.riding = NULL;

    if (ch->specials.riding)
        ch->specials.riding = NULL;

    ch_lights_from_room(ch);

    /* call the base remove from room function */
    remove_char_from_room(ch);
    /*  num_room_lights(rm); */

}

/* base stuff needed to add a character to a room */
/* (doesn't handle lights, position or anything else) */
bool
add_char_to_room(CHAR_DATA * ch, ROOM_DATA * room)
{
    if (!room || !ch)
        return FALSE;

    ch->prev_in_room = NULL;
    ch->next_in_room = room->people;

    room->people = ch;
    if (ch->next_in_room)
        ch->next_in_room->prev_in_room = ch;

    ch->in_room = room;
    return TRUE;
}


/* place a character in a room */
void
char_to_room(CHAR_DATA * ch, ROOM_DATA * room)
{
    int j;
    int sneak = 0;
    CHAR_DATA *tch;
    static int recursive_depth = 0;
    int sneak_roll = number(1, 101);
    int old_room_tuluk = 0;

    if (room_in_city(ch->in_room) == CITY_TULUK) {
      old_room_tuluk = 1;
    }

    recursive_depth++;

    /* call the base add to room function */
    add_char_to_room(ch, room);

    if (!ch || !ch->in_room)
        return;

    if (will_char_fall_in_room(ch, room)) {
        if (!is_char_falling(ch))
            add_char_to_falling(ch);

        if (IS_SET(ch->in_room->room_flags, RFL_NO_FLYING) && IS_AFFECTED(ch, CHAR_AFF_FLYING)) {
            /* strip there flying spell if not immortal and have flying in
             * a no_fly room */

            if (IS_AFFECTED(ch, CHAR_AFF_FLYING))
                affect_from_char(ch, SPELL_FLY);
            REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_FLYING);

        }
    } else {
        if (is_char_falling(ch))
            remove_char_from_falling(ch);
    }


    ch_lights_to_room(ch);

    /* furniture */
    if (ch->on_obj && ch->on_obj->in_room != ch->in_room)
        remove_occupant(ch->on_obj, ch);

    if (IS_AFFECTED(ch, CHAR_AFF_SNEAK)) {
        if ((GET_GUILD(ch) != GUILD_BURGLAR) && (GET_GUILD(ch) != GUILD_PICKPOCKET)
            && (GET_GUILD(ch) != GUILD_ASSASSIN) && (ch->in_room->sector_type <= SECT_CITY))
            affect_from_char(ch, SKILL_SNEAK);

        for (j = 0; j < MAX_WEAR; j++) {
            if ((j == WEAR_BODY) || (j == WEAR_FEET) || (j == WEAR_LEGS) || (j == WEAR_ARMS))
                if (ch->equipment[j] && (ch->equipment[j]->obj_flags.type == ITEM_ARMOR)
                    && (ch->equipment[j]->obj_flags.material != MATERIAL_CLOTH)
                    && (ch->equipment[j]->obj_flags.material != MATERIAL_SKIN))
                    affect_from_char(ch, SKILL_SNEAK);
        }
    }

    for (tch = ch->in_room->people; tch; tch = tch->next_in_room)
        if ((IS_SET(tch->specials.act, CFL_FLEE))
            && (CAN_SEE(tch, ch))
            && (tch != ch)
            && (!IS_IN_SAME_TRIBE(tch, ch))) {
            if (IS_NPC(tch)) {
                sneak = succeed_sneak(ch, tch, sneak_roll);
                if (!((sneak) && (GET_SPEED(ch) == SPEED_SNEAKING)))
                    if (recursive_depth < 4)
                        parse_command(tch, "flee self");
            } else {
                if (ch->in_room == tch->in_room)
                    act("A frightening image appears in your mind as $N enters, and you attempt to escape!", FALSE, tch, 0, ch, TO_CHAR);
                parse_command(tch, "flee self");
            }
        }
    recursive_depth--;

    /*  if (ch->in_room)
     * num_room_lights(ch->in_room); */
}


float
calc_object_weight(OBJ_DATA * obj)
{
    float wt = 0;
    OBJ_DATA *k;
    char bug[MAX_STRING_LENGTH];

    if ((obj == NULL) || (!obj)) {
        sprintf(bug, "NULL object passed to calc_object_weight().");
        gamelog(bug);
        return (wt);
    }

    // always grab base weight
    wt = obj->obj_flags.weight;

    // food does a % based on amount eaten from default object
    if (obj->obj_flags.type == ITEM_FOOD
      && obj->obj_flags.value[0] && obj_default[obj->nr].value[0]) {
        int percent = PERCENTAGE(obj->obj_flags.value[0],
         obj_default[obj->nr].value[0]);

        if (obj_default[obj->nr].value[0] < obj->obj_flags.value[0])
           percent = 100;

        wt = (percent * wt) / 100;
    }

    // The *100 here is because you /100 when calling calc_object_weight
    // So you need to reup it since we're going to /100 again in a bit
    for (k = obj->contains; k; k = k->next_content)
        wt += calc_object_weight(k) * 100;

    if (obj->obj_flags.type == ITEM_DRINKCON)
        wt += obj->obj_flags.value[1] * 100 / 3;

    // Do this to represent the fake decimal system
    wt /= 100;

    return (wt);
}

/* This should be identical to calc_object_weight() except that
   containers are rated by their capacity, not their weight.  
   -Nessalin 4/13/2003 */

float
calc_object_weight_for_cargo_bay(OBJ_DATA * obj)
{
    float wt = 0;
    /*  OBJ_DATA *k; */
    char bug[MAX_STRING_LENGTH];

    if ((obj == NULL) || (!obj)) {
        sprintf(bug, "NULL object passed to calc_object_weight().");
        gamelog(bug);
        return (wt);
    }

    if (obj->obj_flags.type == ITEM_CONTAINER) {
        wt = ((int) (obj->obj_flags.value[0]));
    } 
    else
        wt = calc_object_weight(obj);

    return (wt);
}

/*
 * recursive function to figure out how much weight is being subdued
 * by ch.  Handles recursive subduing (ch subdues p1 who has p2 subdued)
 */
int
calc_subdued_weight(CHAR_DATA * ch)
{
    int weight = 0;
    CHAR_DATA *victim = ch->specials.subduing;

    if (victim != NULL && victim->in_room == ch->in_room) {
        /* add the victim's body weight */
        weight += (GET_WEIGHT(victim) * 10);

        /* add for their equipment */
        weight += calc_carried_weight(victim);

        /* recursively add who they're subduing */
        weight += calc_subdued_weight(victim);
    }

    return (weight);
}


int
calc_carried_weight(CHAR_DATA * ch)
{
    float wt = 0;
    int j = 0;
    OBJ_DATA *o;
    CHAR_DATA *rider = NULL;

    for (o = ch->carrying; o; o = o->next_content)
        wt += calc_object_weight(o);


    for (j = 0; j < MAX_WEAR; j++)
        if (ch->equipment[j]) {
            if (j == WEAR_BACK)
                wt += (int) (calc_object_weight(ch->equipment[j]) * 0.55);
            else
                wt += (int) (calc_object_weight(ch->equipment[j]) * 0.65);
        }

    if (ch->in_room)
        for (rider = ch->in_room->people; rider; rider = rider->next_in_room)
            if (rider && rider->specials.riding && (rider->specials.riding == ch))
                break;

    if (rider) {
        wt += GET_WEIGHT(rider) * 10;
        wt += calc_carried_weight(rider);
    }

    if (ch->lifting) {
        wt += MIN(GET_OBJ_WEIGHT(ch->lifting), (CAN_CARRY_W(ch) - wt));
    }

    wt += (int) (GET_OBSIDIAN(ch) / 200);

    return (wt);
}

int
calc_wagon_weight(OBJ_DATA * wagon)
{
    int wht = 0;
    //  char buf[MAX_STRING_LENGTH];

/*
  sprintf(buf, "Obj: %s.", OSTR(wagon, short_descr));
  gamelog(buf);
*/
    map_wagon_rooms(wagon, (void *) add_room_weight, (void *) &wht);
/*
  sprintf(buf, "After map wagon rooms, Wag weight: %d", wht);
  gamelog(buf);
*/
    wht *= 10;

    if (wht <= 3000)
        wht += 1000;
    else {
        if (wht <= 10000)
            wht += 3000;
        else
            wht += 5000;
    }
/*
  sprintf(buf, "return Wag weight: %d", wht);
  gamelog(buf);
*/
    return (wht);
}

int
calc_wagon_weight_empty(OBJ_DATA * wagon)
{
    int wagon_weight = 0;
/*  char buf[MAX_STRING_LENGTH];  */
    extern void add_room_weight_no_people(struct room_data *room, void *arg);

    map_wagon_rooms(wagon, (void *) add_room_weight_no_people, (void *) &wagon_weight);
/*
  sprintf(buf, "wagon weight is: %d", wagon_weight);
  gamelog (buf);
*/
    if (wagon_weight <= 3000)
        wagon_weight += 1000;
    else {
        if (wagon_weight <= 10000)
            wagon_weight += 3000;
        else
            wagon_weight += 5000;
    }
/*
  sprintf(buf, "Returning wagon weight of: %d", wagon_weight);
  gamelog (buf);
*/
    return (wagon_weight);
}

int
calc_wagon_weight_to_beasts(OBJ_DATA * wagon, int weight)
{
    int num_beasts = 0, wt = 0;
    //  char buf[MAX_STRING_LENGTH];
    struct harness_type *temp_beast;

    wt = weight;

    if (!wagon->beasts) {
        gamelog("Warning! Wagon has no beasts pulling it!");
        return (wt);
    }

    for (temp_beast = wagon->beasts; temp_beast; temp_beast = temp_beast->next) {
        if (GET_POS(temp_beast->beast) >= POSITION_SLEEPING)
            num_beasts++;
    }

    if (num_beasts > 0)
        wt /= ((num_beasts > 1) ? (num_beasts * 2) : 1);
    else
        gamelog("Warning! Num_Beasts in calc_carried_w is 0 or lower!");
    return (wt);
}

int
calc_carried_weight_no_lift(CHAR_DATA * ch) {
  int j, wt = 0;
  OBJ_DATA *o;
  CHAR_DATA *rider = NULL;
  
  for (o = ch->carrying; o; o = o->next_content) {
    wt += calc_object_weight(o);
  }
  
  for (j = 0; j < MAX_WEAR; j++) {
    if (ch->equipment[j]) {
      if (j == WEAR_BACK) {
        wt += (int) (calc_object_weight(ch->equipment[j]) * 0.55);
      } else {
        wt += (int) (calc_object_weight(ch->equipment[j]) * 0.65);
      }
    }
  }
  
  if (ch->in_room)
    for (rider = ch->in_room->people; rider; rider = rider->next_in_room)
            if (rider && rider->specials.riding && (rider->specials.riding == ch))
                break;

    if (rider) {
        wt += GET_WEIGHT(rider) * 10;
        wt += calc_carried_weight(rider);
    }

    wt += (int) (GET_OBSIDIAN(ch) / 200);

    return (wt);
}

int
count_objs_carried(CHAR_DATA * ch)
{
    int nc;
    OBJ_DATA *i;

    for (nc = 0, i = ch->carrying; i; i = i->next_content, nc++);
    return (nc);
}


/* give an object to a char   */
void
obj_to_char(OBJ_DATA * object,  /* Object being moved     */
            CHAR_DATA * ch)
{                               /* Character getting item */
    OBJ_DATA *temp_obj;

    make_sure_obj_is_nowhere(object, "obj_to_char");

    /* object sdesc is same as first item in container sdesc OR
     * there are no objects in the container */

    if ((!(ch->carrying)) || (!(strcmp("", OSTR(ch->carrying, short_descr))))
        || (!(strcmp(OSTR(object, short_descr), OSTR(ch->carrying, short_descr))))) {
        object->prev_content = NULL;
        object->next_content = ch->carrying;
        ch->carrying = object;
    }

    /* Everything else */
    else {
        for (temp_obj = ch->carrying; ((temp_obj) && (temp_obj->next_content)
                                       &&
                                       (strcmp
                                        (OSTR(object, short_descr),
                                         OSTR(temp_obj->next_content, short_descr))));
             temp_obj = temp_obj->next_content);

        /* Got to end of list without finding a match */
        if (!temp_obj->next_content) {
            object->prev_content = NULL;
            object->next_content = ch->carrying;
            ch->carrying = object;
        } else
            /* Didn't get to the end of the list, found a match */
        {
            object->prev_content = temp_obj;
            object->next_content = temp_obj->next_content;
            temp_obj->next_content = object;
        }
    }

    if (object->next_content)
        object->next_content->prev_content = object;

    object->carried_by = ch;

    IS_CARRYING_N(ch) = count_objs_carried(ch);
}


/* take an object from a char */
void
obj_from_char(OBJ_DATA * object)
{
    CHAR_DATA *k;

    if (object->lifted_by)
        remove_all_lifting(object);

    if (!object->carried_by)
        return;

    if (object->prev_content)
        object->prev_content->next_content = object->next_content;
    if (object->next_content)
        object->next_content->prev_content = object->prev_content;
    if (object == object->carried_by->carrying)
        object->carried_by->carrying = object->next_content;

    object->prev_content = NULL;
    object->next_content = NULL;

    k = object->carried_by;
    object->carried_by = NULL;


    IS_CARRYING_N(k) = count_objs_carried(k);

}

void
equip_char(CHAR_DATA * ch, OBJ_DATA * obj, int pos)
{
    int j;

    assert(pos >= 0 && pos < MAX_WEAR);
    assert(ch->equipment[pos] == NULL);

    make_sure_obj_is_nowhere(obj, "equip_char");

    // People affected by the disorient spell can't EP, ES, or ETWO anything
    if (affected_by_spell(ch, PSI_DISORIENT)) {
        if (pos == EP || pos == ES || pos == ETWO) {

            act("You lose your grip upon $o, which slips out of your hands.", TRUE, ch, obj, ch,
                TO_CHAR);
            act("$n loses $s grip upon $o, which slips out of $s hands.", TRUE, ch, obj, ch,
                TO_NOTVICT);
            obj_to_room(obj, ch->in_room);
            return;
        }
    }

    ch->equipment[pos] = obj;
    obj->equipped_by = ch;

    if ((pos != WEAR_ON_BACK) && (pos != WEAR_BACK) && (pos != WEAR_ON_BELT_1)
        && (pos != WEAR_ON_BELT_2)) {
        for (j = 0; j < MAX_OBJ_AFFECT; j++)
            affect_modify(ch, obj->affected[j].location, obj->affected[j].modifier,
                          obj->obj_flags.bitvector, TRUE);
    }
    affect_total(ch);
}

OBJ_DATA *
unequip_char(CHAR_DATA * ch, int pos)
{
    int j;
    OBJ_DATA *obj;

    assert(pos >= 0 && pos < MAX_WEAR);
    if (!ch->equipment[pos])
        return NULL;

    obj = ch->equipment[pos];

    ch->equipment[pos] = 0;
    obj->equipped_by = 0;

    if ((pos != WEAR_ON_BACK) && (pos != WEAR_BACK) && (pos != WEAR_ON_BELT_1)
        && (pos != WEAR_ON_BELT_2))
        for (j = 0; j < MAX_OBJ_AFFECT; j++)
            affect_modify(ch, obj->affected[j].location, obj->affected[j].modifier,
                          obj->obj_flags.bitvector, FALSE);

    affect_total(ch);

    return obj;
}


int
get_number(const char **name)
{
    int i;
    char *ppos;
    char number[64];

    /* initializing our variables */
    number[0] = '\0';

    /* using *name here because we use ppos to assign into *name later */
    if (isdigit(**name) && (ppos = (char *) index(*name, '.'))) {
        /* put a '\0' where the '.' is */
        *ppos++ = '\0';
        /* copy it into number to be used later */
        strncpy(number, *name, sizeof(number));
        /* point *name to after the '.' */
        *name = ppos;

        /* look at the number */
        for (i = 0; *(number + i); i++)
            if (!isdigit(*(number + i)))
                return (0);

        /* atoi it */
        return (atoi(number));
    }
    return (1);
}

OBJ_DATA *
get_weapon(CHAR_DATA * ch, int *hand)
{

    switch (*hand) {
    case EP:
        if (ch->equipment[EP] && (ch->equipment[EP]->obj_flags.type == ITEM_WEAPON))
            return (ch->equipment[EP]);
        else
            return (NULL);
        break;
    case ES:
        if (ch->equipment[ES] && (ch->equipment[ES]->obj_flags.type == ITEM_WEAPON))
            return (ch->equipment[ES]);
        else
            return (NULL);
        break;
    case ETWO:
        if (ch->equipment[ETWO] && (ch->equipment[ETWO]->obj_flags.type == ITEM_WEAPON))
            return (ch->equipment[ETWO]);
        else
            return (NULL);
        break;
    case WEAR_WRIST_R:
        if (ch->equipment[WEAR_WRIST_R]
            && (ch->equipment[WEAR_WRIST_R]->obj_flags.type == ITEM_WEAPON))
            return (ch->equipment[WEAR_WRIST_R]);
        else
            return (NULL);
        break;
    case WEAR_WRIST_L:
        if (ch->equipment[WEAR_WRIST_L]
            && (ch->equipment[WEAR_WRIST_L]->obj_flags.type == ITEM_WEAPON))
            return (ch->equipment[WEAR_WRIST_L]);
        else
            return (NULL);
        break;
    default:
        /* search order = ep, etwo, pwrist, es, swrist */

        if (ch->equipment[EP] && (ch->equipment[EP]->obj_flags.type == ITEM_WEAPON)) {
            *hand = EP;
            return (ch->equipment[EP]);
        } else if (ch->equipment[ETWO] && (ch->equipment[ETWO]->obj_flags.type == ITEM_WEAPON)) {
            *hand = ETWO;
            return (ch->equipment[ETWO]);
        } else if (ch->equipment[WEAR_WRIST_R]
                   && (ch->equipment[WEAR_WRIST_R]->obj_flags.type == ITEM_WEAPON)) {
            *hand = WEAR_WRIST_R;
            return (ch->equipment[WEAR_WRIST_R]);
        } else if (ch->equipment[ES] && (ch->equipment[ES]->obj_flags.type == ITEM_WEAPON)) {
            *hand = ES;
            return (ch->equipment[ES]);
        } else if (ch->equipment[WEAR_WRIST_L]
                   && (ch->equipment[WEAR_WRIST_L]->obj_flags.type == ITEM_WEAPON)) {
            *hand = WEAR_WRIST_L;
            return (ch->equipment[WEAR_WRIST_L]);
        } else {
            *hand = 0;
            return (NULL);
        }
    }
}


OBJ_DATA *
get_obj_equipped(CHAR_DATA * ch, const char *name, int vis)
{
    OBJ_DATA *i;
    int loc;
    int number, count = 1;
    char tmpname[MAX_INPUT_LENGTH];
    const char *tmp;

    strcpy(tmpname, name);
    tmp = tmpname;
    if (!(number = get_number(&tmp)))
        return (0);

    for (loc = WEAR_BELT; loc < MAX_WEAR; loc++) {
        // If there's an object equipped in this location
        if ((i = ch->equipment[loc])
         // and they can see it, or no vis requirement
         && (!vis || (vis && CAN_SEE_OBJ(ch, i)))
         // and it's named appropriately
         && (isname(tmp, OSTR(i, name)) 
          || (isname(tmp, real_sdesc_keywords_obj(i, ch))))) {
            // number check
            if( count == number ) {
                return i;
            }
            count++;
        }
    }

    return NULL;
}

int
get_worn_positon(CHAR_DATA * ch, OBJ_DATA * obj)
{
    int loc;
    int found_pos = -1;

    if (ch)
        for (loc = 0; loc < MAX_WEAR; loc++)
            if (obj == ch->equipment[loc])
                found_pos = loc;

    return (found_pos);
}

/* search a given list for an object, and return a pointer to that object */
OBJ_DATA *
get_obj_in_list(CHAR_DATA *ch, const char *name, OBJ_DATA * list)
{                               /* get_obj_in_list */
    OBJ_DATA *i;
    int j, number;
    char tmpname[MAX_INPUT_LENGTH];
    const char *tmp;

    strcpy(tmpname, name);
    tmp = tmpname;
    if (!(number = get_number(&tmp)))
        return (0);

    for (i = list, j = 1; i && (j <= number); i = i->next_content)
        if ((isname(tmp, OSTR(i, name))) || (isname(tmp, real_sdesc_keywords_obj(i, ch)))) {
            if (j == number)
                return (i);
            j++;
        }
    return (0);
}                               /* get_obj_in_list */


/* returns first object in list which matches vnum 'v' */
OBJ_DATA *
get_obj_in_list_vnum(int v, OBJ_DATA * list)
{
    OBJ_DATA *i;

    for (i = list; i; i = i->next_content)
        if (obj_index[i->nr].vnum == v)
            return (i);

    return NULL;
}


/* Search a given list for an object number, and return a ptr to that obj */
OBJ_DATA *
get_obj_in_list_num(int num, OBJ_DATA * list)
{
    OBJ_DATA *i;

    for (i = list; i; i = i->next_content)
        if (i->nr == num)
            return (i);

    return (0);
}

/*search the entire world for an object, and return a pointer  */
OBJ_DATA *
get_obj(const char *name)
{
    OBJ_DATA *i = NULL;
    int j, number;
    char tmpname[MAX_INPUT_LENGTH];
    const char *tmp;

    strcpy(tmpname, name);
    tmp = tmpname;
    if (!(number = get_number(&tmp)))
        return (0);

    for (i = object_list, j = 1; i && (j <= number); i = i->next) {
        if ((isname(tmp, OSTR(i, name))) || (isname(tmp, real_sdesc_keywords_obj(i, NULL)))) {
            if (j == number)
                return i;          // found
            j++;
        }
    }

    return NULL; // not found
}

OBJ_DATA *
get_obj_zone(int zone_num, const char *find_string)
{
    ROOM_DATA *tmprm;
    OBJ_DATA *tmpobj;
    char tmpname[MAX_INPUT_LENGTH];
    const char *find_tmp;
    int j = 1, number;

    strcpy(tmpname, find_string);
    find_tmp = tmpname;

    if (!(number = get_number(&find_tmp)))
        return 0;

    for (tmprm = zone_table[zone_num].rooms; tmprm; tmprm = tmprm->next_in_zone) {
        for (tmpobj = tmprm->contents; tmpobj; tmpobj = tmpobj->next_content) {
            if (isname(find_tmp, OSTR(tmpobj, name))
                || isname(find_tmp, real_sdesc_keywords_obj(tmpobj, NULL))) {
                if (j == number) {
                    return tmpobj;
                }
                j++;
            }
        }
    }

    return NULL;
}

/*search the entire world for an object number, and return a pointer  */
OBJ_DATA *
get_obj_num(int vnum)
{
    int rnum = real_object(vnum);
    if (rnum < 0 || rnum > top_of_obj_t) {
        return 0;
    }

    OBJ_DATA *objit;
    for (objit = obj_index[rnum].list; objit; objit = objit->next_num)
        return objit;

    return 0;
}

OBJ_DATA *
get_obj_keyword_vnum(const char *key, int vnum)
{
    int rnum = real_object(vnum);
    if (rnum < 0 || rnum > top_of_obj_t) {
        return 0;
    }

    OBJ_DATA *objit;
    for (objit = obj_index[rnum].list; objit; objit = objit->next_num)
        if (isname(key, OSTR(objit, name)) ||
            isname(key, real_sdesc_keywords_obj(objit, NULL)))
            return objit;

    return 0;
}

OBJ_DATA *
get_obj_num_offset(int num, int offset)
{
    OBJ_DATA *i;

    int count = 0;
    for (i = object_list; i; i = i->next) {
        if (obj_index[i->nr].vnum == num) {
            count++;
            if (count == offset)
                return i;
        }
    }

    return NULL;
}


OBJ_DATA *
get_obj_zone_num_offset(int zone, int num, int offset)
{
    OBJ_DATA *i;
    int count = 0;

    for (i = object_list; i; i = i->next) {
        if ((obj_index[i->nr].vnum == num) && i->in_room && (i->in_room->number / 1000 == zone)) {
            count++;
            if (count == offset)
                return i;
        }
    }

    return NULL;
}

CHAR_DATA *
get_char_num_offset(int vnum, int offset)
{
    CHAR_DATA *i;
    int count = 0;

    for (i = character_list; i; i = i->next) {
        if (npc_index[i->nr].vnum == vnum) {
            count++;
            if (count == offset)
                return i;
        }
    }

    return NULL;
}

CHAR_DATA *
get_char_zone_num_offset(int zone, int vnum, int offset)
{
    CHAR_DATA *i;
    int count = 0;

    for (i = character_list; i; i = i->next) {
        if (npc_index[i->nr].vnum == vnum && i->in_room && (i->in_room->number / 1000 == zone)) {
            count++;
            if (count == offset)
                return i;
        }
    }

    return NULL;
}

/* return an integer corresponding to an exit, given the name of the */
/* exit and the character issuing the command.                       */
int
get_direction(CHAR_DATA * ch, char *arg)
{
    int i;
    int len = 0;

    if (arg)
        len = strlen(arg);

    for (i = 0; *dir_name[i] != '\n'; i++)
        /* if (*arg == *dir_name[i]) */
        if (!stricmp(arg, dir_name[i]) || (len == 1 && arg[0] == dir_name[i][0]))
            break;
    if (*dir_name[i] == '\n')
        return -1;

    if (!EXIT(ch, i))           /* No such exit from room */
        return -1;

    return i;
}

int
get_direction_from_name(const char *direction_name)
{
    if (is_abbrev(direction_name, "north"))
        return DIR_NORTH;
    if (is_abbrev(direction_name, "south"))
        return DIR_SOUTH;
    if (is_abbrev(direction_name, "east"))
        return DIR_EAST;
    if (is_abbrev(direction_name, "west"))
        return DIR_WEST;
    if (is_abbrev(direction_name, "up"))
        return DIR_UP;
    if (is_abbrev(direction_name, "down"))
        return DIR_DOWN;
    if (is_abbrev(direction_name, "out"))
        return DIR_OUT;

    return DIR_UNKNOWN;
}

/* return a room that is dist max 5 away from char, in direction dir */
ROOM_DATA *
get_room_distance(CHAR_DATA * ch, int dist, int dir)
{
    int n;
    ROOM_DATA *r;
    ROOM_DATA *tmproom;

    if (!(r = ch->in_room))
        return NULL;

    if ((dir < DIR_NORTH) || (dir > DIR_OUT))
        return NULL;

    if (dist == 0)
        return (ch->in_room);

    if (dir == DIR_OUT) {
        if (ch->in_room->wagon_exit)
            return ch->in_room->wagon_exit;

        return NULL;
    }

    for (n = 0; n < dist; n++)
        if (r->direction[dir] && r->direction[dir]->to_room
            && (!IS_SET(r->direction[dir]->exit_info, EX_CLOSED))
            && (!IS_SET(r->direction[dir]->exit_info, EX_SPL_SAND_WALL)))
            /*        && (!is_secret(r->direction[dir])))
             * took this out, because people were not able to see
             * through secret doors even if they were open */
        {
            tmproom = r->direction[dir]->to_room;
            r = tmproom;
        }

    if (r == ch->in_room) {
        return NULL;
    } else
        return r;

}

ROOM_DATA *
get_room_num(int nr)
{
    int zone = (nr / 1000);
    ROOM_DATA *r;

    if ((zone > top_of_zone_table) || (zone < 0))
        return NULL;
    if (!zone_table[zone].rooms) {
		gamelog("did not find rooms for this zone");
        return NULL;
	}
    for (r = zone_table[zone].rooms; r; r = r->next_in_zone) {
        if (r->number == nr)
            return r;
	}
    return NULL;
}



/* search a room for a char, and return a pointer if found..  */
CHAR_DATA *
get_char_room(const char *name, ROOM_DATA * room)
{
    CHAR_DATA *i;
    int j, number;
    char tmpname[MAX_INPUT_LENGTH];
    const char *tmp;

    strcpy(tmpname, name);
    tmp = tmpname;
    if (!(number = get_number(&tmp)))
        return (0);

    for (i = room->people, j = 1; i && (j <= number); i = i->next_in_room)
        /* Now account for hooded figures - Tiernan 4/23/98 */
        if (isname(tmp, GET_NAME(i)) || isname(tmp, i->player.extkwds)
            || (isname(tmp, real_sdesc_keywords(i)))) {
            if (j == number)
                return (i);
            j++;
        }
    return (0);
}


/* search a room for a char, and return a pointer if found..  */
CHAR_DATA *
get_char_room_num(int num, ROOM_DATA * room)
{
    CHAR_DATA *i;

    for (i = room->people; i; i = i->next_in_room)
        if (IS_NPC(i) && (npc_index[i->nr].vnum == num))
            return (i);
    return (0);
}

void
get_arg_char_room_vis(CHAR_DATA * i, CHAR_DATA * j, char *buffer)
{
    char buf[256], name[256];
    CHAR_DATA *tmp;
    int number;

    *buffer = '\0';

    if (!CAN_SEE(i, j))
        return;
    if (i->in_room != j->in_room)
        return;

    one_argument(GET_NAME(j), buf, sizeof(buf));

    number = 0;
    for (tmp = i->in_room->people; tmp; tmp = tmp->next_in_room) {
        if (CAN_SEE(i, tmp)) {
            if (isname(buf, GET_NAME(tmp)) || isname(buf, tmp->player.extkwds))
                number++;
            if (tmp == j)
                break;
        }
    }

    if (!tmp)
        return;

    if (number == 1) {
        strcpy(name, buf);
    } else {
        sprintf(name, "%d.%s", number, buf);
    }

    strcpy(buffer, name);
}

void
get_arg_char_far_room_vis(CHAR_DATA * i, CHAR_DATA * j, ROOM_DATA * tar_room, char *buffer)
{
    char buf[256], name[256];
    CHAR_DATA *tmp;
    int number;

    *buffer = '\0';

    if (!CAN_SEE(i, j))
        return;

    one_argument(GET_NAME(j), buf, sizeof(buf));

    number = 0;
    for (tmp = tar_room->people; tmp; tmp = tmp->next_in_room) {
        if (CAN_SEE(i, tmp)) {
            if ((isname(buf, GET_NAME(tmp))) || (isname(buf, tmp->player.extkwds)))
                number++;
            if (tmp == j)
                break;
        }
    }

    if (!tmp)
        return;

    if (number == 1)
        strcpy(name, buf);
    else
        sprintf(name, "%d.%s", number, buf);

    strcpy(buffer, name);
}

/* search all over the world for a char, and return a pointer if found */
CHAR_DATA *
get_char(const char *name)
{
    CHAR_DATA *i;
    int j, number;
    char tmpname[MAX_INPUT_LENGTH];
    const char *tmp;

    if (!*name)
        return 0;

    strcpy(tmpname, name);
    tmp = tmpname;
    if (!(number = get_number(&tmp)))
        return (0);

    for (i = character_list, j = 1; i && (j <= number); i = i->next)
        if (isname(tmp, GET_NAME(i)) || isname(tmp, i->player.extkwds)) {
            if (j == number)
                return (i);
            j++;
        }
    return (0);
}

/* search all over the world for a char, and return a pointer if found */
/* find pc's only by account */
CHAR_DATA *
get_char_pc_account(const char *name, const char *account)
{
    CHAR_DATA *i;

    if (!*name)
        return 0;

    if (!account || (account && account[0] == '\0'))
        return get_char_pc(name);

    for (i = character_list; i; i = i->next)
        if ((isname(name, GET_NAME(i)) || isname(name, i->player.extkwds))
            && !IS_NPC(i)
            && ((i->desc && !strcmp(account, i->desc->player_info->name))
                || (!i->desc && !strcmp(account, i->player.info[0]))))
            return (i);

    return NULL;
}


/* search all over the world for a char, and return a pointer if found */
/* find pc's only */
CHAR_DATA *
get_char_pc(const char *name)
{
    CHAR_DATA *i;

    if (!*name)
        return 0;

    for (i = character_list; i; i = i->next)
        if ((isname(name, GET_NAME(i)) || isname(name, i->player.extkwds))
            && !IS_NPC(i))
            return (i);

    return NULL;
}

void
get_arg_char_world_vis(CHAR_DATA * i, CHAR_DATA * j, char *buffer)
{
    char buf[256], name[256];
    CHAR_DATA *tmp;
    int number;

    *buffer = '\0';

    if (!CAN_SEE(i, j))
        return;

    one_argument(GET_NAME(j), buf, sizeof(buf));

    number = 0;
    for (tmp = i->in_room->people; tmp; tmp = tmp->next_in_room)
        if (CAN_SEE(i, tmp)) {
            if (isname(buf, GET_NAME(tmp)) || isname(buf, tmp->player.extkwds))
                number++;
            if (tmp == j)
                break;
        }

    if (!tmp)
        for (tmp = character_list; tmp; tmp = tmp->next)
            if (CAN_SEE(i, tmp) && (tmp->in_room != i->in_room)) {
                if (isname(buf, GET_NAME(tmp)) || isname(buf, tmp->player.extkwds))
                    number++;
                if (tmp == j)
                    break;
            }

    if (!tmp)
        return;

    if (number == 1) {
        strcpy(name, buf);
    } else {
        sprintf(name, "%d.%s", number, buf);
    }

    strcpy(buffer, name);
}



/* search all over the world for a char num, and return a pointer if found */
CHAR_DATA *
get_char_num(int nr)
{
    CHAR_DATA *i;

    for (i = character_list; i; i = i->next)
        if (i->nr == nr)
            return (i);

    return (0);
}

/* search all over the world for a char with vnum, and return a pointer 
   if found */
CHAR_DATA *
get_char_vnum(int vnum)
{
    CHAR_DATA *i;

    for (i = character_list; i; i = i->next)
        if (npc_index[i->nr].vnum == vnum)
            return (i);

    return (0);
}


CHAR_DATA *
get_char_keyword_vnum(const char *key, int vnum)
{
    if (key && !*key)
        return 0;

    int rnum = real_mobile(vnum);
    if (rnum < 0 || rnum > top_of_npc_t) {
        return 0;
    }

    const char *afternum = key;
    int number;
    if (!(number = get_number(&afternum)))
        return (0);

    int j = 1;
    CHAR_DATA *charit;
    for (charit = npc_index[rnum].list; charit; charit = charit->next_num) {
        if (isname(key, GET_NAME(charit)) || isname(key, charit->player.extkwds)) {
            if (j == number)
                return charit;
            j++;
        }
    }

    return 0;
}


/* put an object in a room */
void
obj_to_room_old(OBJ_DATA * object,      /* Object to put     */
                ROOM_DATA * room)
{                               /* Room to put it in */
    assert(room);               /* Insure the room exists */

    make_sure_obj_is_nowhere(object, "obj_to_room");

    if (object->obj_flags.type == ITEM_FIRE)
        if (object->obj_flags.value[0])
            room->light++;

    if (IS_SET(room->room_flags, RFL_FALLING)
        && (room->direction[DIR_DOWN])
        && !is_obj_falling(object)
        && (GET_OBJ_WEIGHT(object) > 0)
        && (!((object->obj_flags.type == ITEM_WAGON)
              && (IS_SET(object->obj_flags.value[3], WAGON_CAN_GO_SILT))
              && (room->sector_type == SECT_SILT))))
        add_obj_to_falling(object);
    else
        remove_obj_from_falling(object);

    object->prev_content = NULL;
    object->next_content = room->contents;
    room->contents = object;
    if (object->next_content)
        object->next_content->prev_content = object;

    object->in_room = room;
}

/* put an object in a room */
/* I'm modifying this so that we can have items listed in a room by
   lumps. Eventually when you look at a room you should see [50] arrows,
   you get the idea.  The original versino of this function is above,
   in obj_to_room_old. -Nessalin 06/20/97 */

void
obj_to_room(OBJ_DATA * object,  /* Object to put     */
            ROOM_DATA * room)
{                               /* Room to put it in */
    OBJ_DATA *temp_obj;

    assert(room);               /* Insure the room exists */

    make_sure_obj_is_nowhere(object, "obj_to_room");

    if (object->obj_flags.type == ITEM_FIRE)
        if (object->obj_flags.value[0])
            room->light++;

    if (IS_SET(room->room_flags, RFL_FALLING)
        && (room->direction[DIR_DOWN])
        && !is_obj_falling(object)
        && (GET_OBJ_WEIGHT(object) > 0)
        &&
        (!(object->obj_flags.type == ITEM_FURNITURE
           && IS_SET(object->obj_flags.value[1], FURN_SKIMMER)
           && room->sector_type == SECT_SILT))
        && (!((object->obj_flags.type == ITEM_WAGON)
              && (IS_SET(object->obj_flags.value[3], WAGON_CAN_GO_SILT))
              && (room->sector_type == SECT_SILT)))) {
        add_obj_to_falling(object);
    } else
        remove_obj_from_falling(object);

    /* object sdesc is same as first item in room sdesc OR
     * there are no objects in the room */

    if ((!(room->contents)) || (!(strcmp("", OSTR(room->contents, long_descr))))
        || (!(strcmp(OSTR(object, long_descr), OSTR(room->contents, long_descr))))) {
        object->prev_content = NULL;
        object->next_content = room->contents;
        room->contents = object;
    }

    /* Everything else */

    else {
        for (temp_obj = room->contents; ((temp_obj) && (temp_obj->next_content)
                                         &&
                                         (strcmp
                                          (OSTR(object, long_descr),
                                           OSTR(temp_obj->next_content, long_descr))));
             temp_obj = temp_obj->next_content);

        /* Got to end of list without finding a match */
        if (!temp_obj->next_content) {
            object->prev_content = NULL;
            object->next_content = room->contents;
            room->contents = object;
        } else
            /* Didn't get to the end of the list, found a match */
        {
            object->prev_content = temp_obj;
            object->next_content = temp_obj->next_content;
            temp_obj->next_content = object;
        }
    }

    if (object->next_content)
        object->next_content->prev_content = object;

    object->in_room = room;

    if (object->obj_flags.type == ITEM_WAGON) {
        ROOM_DATA *ent_room = NULL;
        ent_room = get_wagon_entrance_room(object);

        if (ent_room && !IS_SET(object->obj_flags.value[3], WAGON_NO_LEAVE))
            ent_room->wagon_exit = room;
    }
#ifdef WAGON_SAVE
    /* only update the list after boot */
    if (object->obj_flags.type == ITEM_WAGON && object->nr != 0 && done_booting) {
        add_wagon_save(object);
        save_wagon_saves();
    }
#endif

}

/* Find the cache object in the room */
OBJ_DATA *
find_artifact_cache(ROOM_DATA * room)
{
	OBJ_DATA *obj_object = NULL, *next_obj;

	if (!room)
		return NULL;

	// Look in the room
	for (obj_object = room->contents; obj_object; obj_object = next_obj) {
            next_obj = obj_object->next_content;
	    if (obj_index[obj_object->nr].vnum == 140) // Winner!
		    break;
	}
	return obj_object;
}

/* Returns a cache object in the room (new or existing) */
OBJ_DATA *
create_artifact_cache(ROOM_DATA * room)
{
	OBJ_DATA *cache_object = NULL;

	if (!room)
		return NULL;

	// Look for one already in the room
	cache_object = find_artifact_cache(room);

	// Not found, create it
        if (!cache_object) {
		cache_object = read_object(140, VIRTUAL);
		obj_to_room(cache_object, room);
	}

	return cache_object;
}

/* Put an object in a room's artifact cache */
void
obj_to_artifact_cache(OBJ_DATA * object,  /* Object to put     */
            ROOM_DATA * room)
{
    OBJ_DATA *cache_obj;

    assert(room);               /* Insure the room exists */

    make_sure_obj_is_nowhere(object, "obj_to_artifact_cache");

    cache_obj = create_artifact_cache(room); // Creates if not already made 
    obj_to_obj(object, cache_obj); // Move it on in

    return;
}

extern bool unbooting_db;

ROOM_DATA *
get_wagon_entrance_room(OBJ_DATA * wagon)
{
    if (!wagon)
        return NULL;

    if (wagon->obj_flags.type != ITEM_WAGON)
        return NULL;

    if (wagon->obj_flags.value[0] == 0)
        return NULL;

    return get_room_num(wagon->obj_flags.value[0]);
}

/* Take an object from a room */
void
obj_from_room(OBJ_DATA * object)
{
    if (!object->in_room)
        return;

    if (object->obj_flags.type == ITEM_FIRE)
        if (object->obj_flags.value[0])
            object->in_room->light--;

    if (is_obj_falling(object))
        remove_obj_from_falling(object);

    if (object->obj_flags.type == ITEM_WAGON) {
        ROOM_DATA *ent_room = NULL;
        ent_room = get_wagon_entrance_room(object);

        if (ent_room)
            ent_room->wagon_exit = NULL;
    }
#ifdef WAGON_SAVE
    if (object->obj_flags.type == ITEM_WAGON && object->nr != 0
        && find_wagon_save_by_compare(object, object->in_room->number)) {
        remove_wagon_save(object);
        if (!unbooting_db) {
            save_wagon_saves();
        }
    }
#endif

    if (object->prev_content)
        object->prev_content->next_content = object->next_content;
    if (object->next_content)
        object->next_content->prev_content = object->prev_content;
    if (object == object->in_room->contents)
        object->in_room->contents = object->next_content;

    object->in_room = NULL;
    object->prev_content = NULL;
    object->next_content = NULL;
}


/* put an object in an object (quaint)  */
void
obj_to_obj(OBJ_DATA * obj,      /* Object being moved */
           OBJ_DATA * obj_to)
{                               /* Container object   */
    OBJ_DATA *temp_obj;

    make_sure_obj_is_nowhere(obj, "obj_to_obj");

    /* object sdesc is same as first item in container sdesc OR
     * there are no objects in the container */

    if ((!(obj_to->contains)) || (!(strcmp("", OSTR(obj_to->contains, short_descr))))
        || (!(strcmp(OSTR(obj, short_descr), OSTR(obj_to->contains, short_descr))))) {
        obj->prev_content = NULL;
        obj->next_content = obj_to->contains;
        obj_to->contains = obj;
    }

    /* Everything else */
    else {
        for (temp_obj = obj_to->contains; ((temp_obj) && (temp_obj->next_content)
                                           &&
                                           (strcmp
                                            (OSTR(obj, short_descr),
                                             OSTR(temp_obj->next_content, short_descr))));
             temp_obj = temp_obj->next_content);

        /* Got to end of list without finding a match */
        if (!temp_obj->next_content) {
            obj->prev_content = NULL;
            obj->next_content = obj_to->contains;
            obj_to->contains = obj;
        } else
            /* Didn't get to the end of the list, found a match */
        {
            obj->prev_content = temp_obj;
            obj->next_content = temp_obj->next_content;
            temp_obj->next_content = obj;
        }
    }

    if (obj->next_content)
        obj->next_content->prev_content = obj;

    obj->in_obj = obj_to;
}

/* remove an object from an object */
void
obj_from_obj(OBJ_DATA * obj)
{
    OBJ_DATA *obj_from;

    if (obj->in_obj) {
        if (obj->next_content)
            obj->next_content->prev_content = obj->prev_content;
        if (obj->prev_content)
            obj->prev_content->next_content = obj->next_content;
        if (obj == obj->in_obj->contains)
            obj->in_obj->contains = obj->next_content;

        obj_from = obj->in_obj;

        obj->in_obj = 0;
        obj->prev_content = NULL;
        obj->next_content = NULL;
    }
}

void
obj_around_obj(OBJ_DATA * obj, OBJ_DATA * obj_to)
{
    OBJ_DATA *temp_obj;

    make_sure_obj_is_nowhere(obj, "obj_around_obj");

    /* object sdesc is same as first item in container sdesc OR
     * there are no objects in the container */

    if ((!(obj_to->around)) || (!(strcmp("", OSTR(obj_to->around, short_descr))))
        || (!(strcmp(OSTR(obj, short_descr), OSTR(obj_to->around, short_descr))))) {
        obj->prev_content = NULL;
        obj->next_content = obj_to->around;
        obj_to->around = obj;
    }

    /* Everything else */
    else {
        for (temp_obj = obj_to->around; ((temp_obj) && (temp_obj->next_content)
                                         &&
                                         (strcmp
                                          (OSTR(obj, short_descr),
                                           OSTR(temp_obj->next_content, short_descr))));
             temp_obj = temp_obj->next_content);

        /* Got to end of list without finding a match */
        if (!temp_obj->next_content) {
            obj->prev_content = NULL;
            obj->next_content = obj_to->around;
            obj_to->around = obj;
        } else
            /* Didn't get to the end of the list, found a match */
        {
            obj->prev_content = temp_obj;
            obj->next_content = temp_obj->next_content;
            temp_obj->next_content = obj;
        }
    }

    if (obj->next_content)
        obj->next_content->prev_content = obj;

    obj->table = obj_to;
}

void
obj_from_around_obj(OBJ_DATA * obj)
{
    OBJ_DATA *obj_from;

    if (obj->table) {
        if (obj->next_content)
            obj->next_content->prev_content = obj->prev_content;
        if (obj->prev_content)
            obj->prev_content->next_content = obj->next_content;
        if (obj == obj->table->around)
            obj->table->around = obj->next_content;

        obj_from = obj->table;

        obj->table = 0;
        obj->prev_content = NULL;
        obj->next_content = NULL;
    }
}


/* wtf?  this might be causing major fuckups... */
void
object_list_new_owner(OBJ_DATA * list, CHAR_DATA * ch)
{
    if (list) {
/*
   object_list_new_owner(list->contains, ch);
   object_list_new_owner(list->next_content, ch);
 */
        list->carried_by = ch;
    }
}

/* Extract an object from the world */
void
extract_obj(OBJ_DATA * obj)
{
    int l;
    char buf2[256];

    if (!obj)
        return;

    heap_delete(obj, event_obj_cmp);
    remove_obj_all_barter(obj);

    sprintf(buf2, "%s", OSTR(obj, short_descr));

    while (obj->guards)
        stop_guard(obj->guards->guard, 0);

    if (obj->in_room)
        obj_from_room(obj);
    else if (obj->carried_by)
        obj_from_char(obj);
    else if (obj->equipped_by) {
        for (l = 0; l < MAX_WEAR; l++)
            if (obj->equipped_by->equipment[l] == obj)
                break;
        if (l == MAX_WEAR) {
            gamelog("extract_obj: gah!");
            exit(1);
        }
        unequip_char(obj->equipped_by, l);
    } else if (obj->in_obj)
        obj_from_obj(obj);

    if (obj->lifted_by)
        remove_all_lifting(obj);

    make_sure_obj_is_nowhere(obj, "extract_obj");

    /* handle objects around the obj */
    while (obj->around) {
        OBJ_DATA *tmpAround = obj->around;

        obj_from_around_obj(tmpAround);
        extract_obj(tmpAround);
    }

    /* handle character's on it */
    while (obj->occupants) {
        remove_occupant(obj, obj->occupants->ch);
    }

    while (obj->contains)
        extract_obj(obj->contains);

    if (!unbooting_db && obj->obj_flags.type == ITEM_WAGON && obj->obj_flags.value[0] >= 73000
        && obj->obj_flags.value[0] <= 73999)
        free_reserved_room(get_room_num(obj->obj_flags.value[0]));

    // FIXME:  remove_object_from_list
    if (obj->prev)
        obj->prev->next = obj->next;
    if (obj->next)
        obj->next->prev = obj->prev;
    if (obj == object_list)
        object_list = obj->next;

    if (obj->nr >= 0) {
        if (obj->prev_num)
            obj->prev_num->next_num = obj->next_num;
        if (obj->next_num)
            obj->next_num->prev_num = obj->prev_num;
        if (obj == (OBJ_DATA *) obj_index[obj->nr].list)
            obj_index[obj->nr].list = obj->next_num;

        obj_index[obj->nr].number--;
    }

  //  JS_clear_obj_pointer(obj);

    if (shutdown_game) {
        free_obj(obj);
        return;
    }

    extracted_thing *et = malloc(sizeof(*et));
    et->type = ET_OBJ;
    et->obj = obj;
    et->next = et_list;
    et_list = et;
}

/* Extract a wagon from the world */
void
extract_wagon(OBJ_DATA * obj)
{
    int l;
    char buf2[256];

    heap_delete(obj, event_obj_cmp);
    remove_obj_all_barter(obj);

    sprintf(buf2, "%s", OSTR(obj, short_descr));

    while (obj->guards)
        stop_guard(obj->guards->guard, 0);

    if (obj->in_room)
        obj_from_room(obj);
    else if (obj->carried_by)
        obj_from_char(obj);
    else if (obj->equipped_by) {
        for (l = 0; l < MAX_WEAR; l++)
            if (obj->equipped_by->equipment[l] == obj)
                break;
        if (l == MAX_WEAR) {
            gamelog("extract_obj: gah!");
            exit(1);
        }
        unequip_char(obj->equipped_by, l);
    } else if (obj->in_obj)
        obj_from_obj(obj);

    make_sure_obj_is_nowhere(obj, "extract_obj");

    while (obj->contains)
        extract_obj(obj->contains);

    // FIXME:  remove_object_from_list
    if (obj->prev)
        obj->prev->next = obj->next;
    if (obj->next)
        obj->next->prev = obj->prev;
    if (obj == object_list)
        object_list = obj->next;

    if (obj->nr >= 0) {
        if (obj->prev_num)
            obj->prev_num->next_num = obj->next_num;
        if (obj->next_num)
            obj->next_num->prev_num = obj->prev_num;
        if (obj == (OBJ_DATA *) obj_index[obj->nr].list)
            obj_index[obj->nr].list = obj->next_num;

        obj_index[obj->nr].number--;
    }
    free_obj(obj);
}

void
update_object(OBJ_DATA * obj, int use)
{
    if (!obj)
        return;

    if (obj->obj_flags.timer > 0)
        obj->obj_flags.timer -= use;

    if (obj->contains)
        update_object(obj->contains, use);

    if (obj->next_content)
        update_object(obj->next_content, use);
}

void
update_char_objects(CHAR_DATA * ch)
{
    int i;

    update_char_lights(ch);

    for (i = 0; i < MAX_WEAR; i++)
        if (ch->equipment[i])
            update_object(ch->equipment[i], 2);

    if (ch->carrying)
        update_object(ch->carrying, 1);
}


int extracting_mount = 0;

void
extract_char_mnt_and_objs(CHAR_DATA * ch)
{
    struct follow_type *hitch, *mnt;
    int l;

    for (l = 0; l < MAX_WEAR; l++)
        if (ch->equipment[l])
            extract_obj(ch->equipment[l]);
    while (ch->carrying)
        extract_obj(ch->carrying);

    extracting_mount = 1;
    if (ch->specials.riding) {
        for (l = 0; l < MAX_WEAR; l++)
            if (ch->specials.riding->equipment[l])
                extract_obj(ch->specials.riding->equipment[l]);
        while (ch->specials.riding->carrying)
            extract_obj(ch->specials.riding->carrying);

        for (hitch = ch->specials.riding->followers; hitch;)
            if (IS_SET(hitch->follower->specials.act, CFL_MOUNT) && ch->in_room
                && hitch->follower->in_room && (ch->in_room == hitch->follower->in_room)
                && !IS_AFFECTED(hitch->follower, CHAR_AFF_SUMMONED)) {
                mnt = hitch;
                hitch = hitch->next;
                extract_char(mnt->follower);
            } else
                hitch = hitch->next;

        extract_char(ch->specials.riding);
    }
    for (hitch = ch->followers; hitch;)
        if (IS_SET(hitch->follower->specials.act, CFL_MOUNT) && ch->in_room
            && hitch->follower->in_room && (ch->in_room == hitch->follower->in_room)
            && !IS_AFFECTED(hitch->follower, CHAR_AFF_SUMMONED)) {
            mnt = hitch;
            hitch = hitch->next;
            extract_char(mnt->follower);
        } else
            hitch = hitch->next;
    extracting_mount = 0;
}


/* extract a ch completely from the world, and leave his stuff behind */
void
extract_char(CHAR_DATA * ch)
{
    CHAR_DATA *k, *next_char;
    struct descriptor_data *t_desc;
    int rnum = 0;
    DESCRIPTOR_DATA *tmpd;

    // If they're in the psionics listener list, clean 'em up.
    remove_listener(ch);

    /* Break off any contact they've made and remove the contact affect
     * before we write out their pfile. -Tiernan */
    if (ch->specials.contact && !affected_by_spell(ch, PSI_CONCEAL)) {
        if (has_skill(ch->specials.contact, PSI_TRACE)) {
            if (psi_skill_success(ch->specials.contact, 0, PSI_TRACE, 0)) {
                cprintf(ch->specials.contact, "You sense %s withdraw from your mind.\n\r", MSTR(ch, short_descr));
            } else {
                cprintf(ch->specials.contact, "You sense a foreign presence withdraw from your mind.\n\r");
                gain_skill(ch->specials.contact, PSI_TRACE, 1);
            }
        } else {
            cprintf(ch->specials.contact, "You sense a foreign presence withdraw from your mind.\n\r");
        }
        remove_contact(ch);
    }

    /* same for subdue */
    REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_SUBDUED);

    /* moved from just above extract_char_mnt_and_objs call at end of function */
    /* saves the person before removing objects, preserving affects  -Morg */
    if (!IS_NPC(ch) && !affected_by_spell(ch, SPELL_POSSESS_CORPSE))
        save_char(ch);

    flush_queued_commands(ch, TRUE);

    rem_from_command_q(ch);
    remove_char_all_barter(ch);

    remove_char_from_falling(ch);

    for (tmpd = descriptor_list; tmpd; tmpd = tmpd->next) {
        if (find_ch_in_monitoring(tmpd->monitoring, ch))
            remove_monitoring(tmpd, ch, MONITOR_COMPLETE);
    }

    /* Added by Morgenes to stop bad-pointers when people die/quit */
    if (ch->lifting) {
        remove_lifting(ch->lifting, ch);
    }

    heap_delete(ch, event_ch_tar_ch_cmp);
    if (ch->desc && (ch->desc->character != ch))
        ch->desc = 0;
    if (!IS_NPC(ch) && !ch->desc) {
        for (t_desc = descriptor_list; t_desc; t_desc = t_desc->next)
            if (t_desc->original == ch)
                cmd_return(t_desc->character, "", 0, 0);
    }

    if (ch->desc) {
        /* forget snooping */
        if (ch->desc->snoop.snooping)
            ch->desc->snoop.snooping->desc->snoop.snoop_by = 0;

        if (ch->desc->snoop.snoop_by) {
            send_to_char("Your victim is no longer among us.\n\r", ch->desc->snoop.snoop_by);
            ch->desc->snoop.snoop_by->desc->snoop.snooping = 0;
        }
        ch->desc->snoop.snooping = ch->desc->snoop.snoop_by = 0;
    }

    stop_all_fighting(ch);

    /* Make sure we remove them from any furniture they were on -Tiernan */
    if (ch->on_obj)
        remove_occupant(ch->on_obj, ch);

    remove_all_hates_fears(ch);

    for (k = character_list; k; k = k->next) {
        if (does_hate(k, ch))
            remove_from_hates(k, ch);
        if (does_fear(k, ch))
            remove_from_fears(k, ch);
        /*
         * taken care of below
         * if (k->specials.guarding == ch)
         * k->specials.guarding = 0;
         */
        if (k->specials.subduing == ch)
            k->specials.subduing = 0;
        if (k->specials.riding == ch) {
            k->specials.riding = 0;
        }
        if (k->specials.contact == ch) {
            act("You feel your mental contact withdrawing from the mind of your target.", FALSE, k,
                0, 0, TO_CHAR);
            remove_contact(k);
        }
    }

    /*   shhlog ("EXTRACT CHAR: Before Stop fighting"); */
    for (k = combat_list; k; k = next_char) {
        next_char = k->next_fighting;
        stop_fighting(k, ch);
    }

    /* check if they dont get to save there equipment 
     * I'm taking this our b/c quit regardless is disabled anywho and it's 
     * mucking up the character saving stuff. -Hal */

#undef QUIT_REGARDLESS
#ifdef QUIT_REGARDLESS
    if (((IS_NPC(ch) || (!IS_IMMORTAL(ch) && !IS_SET(ch->in_room->room_flags, RFL_SAFE)
                         && (GET_GUILD(ch) != GUILD_RANGER))))) {       /* Ten */
        /* they do not save their items */
        if (!IS_NPC(ch))
            del_char_objs(ch);

        if (ch->in_room && !extracting_mount) {
            for (l = 0; l < MAX_WEAR; l++)
                if (ch->equipment[l])
                    obj_to_room(unequip_char(ch, l), ch->in_room);
            while (ch->carrying) {
                i = ch->carrying;
                obj_from_char(i);
                obj_to_room(i, ch->in_room);
            }
        } else {
            /* assert(ch->in_room == 0) */
            for (l = 0; l < MAX_WEAR; l++)
                if (ch->equipment[l])
                    extract_obj(ch->equipment[l]);
            while (ch->carrying)
                extract_obj(ch->carrying);
        }
    } else {
        /* they do save their items */
        save_char_objs(ch);
        extract_char_mnt_and_objs(ch);
    }
#endif
    /*  shhlog ("EXTRACT CHAR: Before extract char mnt and objs"); */

    extract_char_mnt_and_objs(ch);

    if (ch->followers || ch->master)
        die_follower(ch);

    /* shhlog ("EXTRACT CHAR: Before die guard"); */
    /*  if (ch->guards || ch->specials.guarding) */
    die_guard(ch);

    if (is_char_watching(ch))
        stop_watching_raw(ch);

    stop_reverse_watching(ch);

    /* shhlog ("EXTRACT CHAR: Before remove harness"); */
    if (ch->specials.harnessed_to)
        remove_harness(ch);

    /* shhlog ("EXTRACT CHAR: Before in room"); */
    if (ch->in_room) {
        if (!IS_NPC(ch) && (GET_POS(ch) == POSITION_DEAD))
            rnum = home_room(ch)->number;
        else
            rnum = ch->in_room->number;
        ch->specials.was_in_room = ch->in_room;
        char_from_room(ch);
    }

    /* shhlog ("EXTRACT CHAR: Before list adjustment"); */
    if (ch->prev)
        ch->prev->next = ch->next;
    if (ch->next)
        ch->next->prev = ch->prev;
    if (ch == character_list)
        character_list = ch->next;

    /* shhlog ("EXTRACT CHAR: Before armor"); */

    ch->abilities.armor = race[(int) GET_RACE(ch)].natural_armor;

    /* shhlog ("EXTRACT CHAR: Before Position"); */
    if (!IS_NPC(ch)) {
        set_hit(ch, MAX(GET_HIT(ch), 1));
        set_char_position(ch, POSITION_STANDING);
    }

    /* shhlog ("EXTRACT CHAR: Before JS_clear_char_pointer"); */
 //   JS_clear_char_pointer(ch);

    /* shhlog ("EXTRACT CHAR: Before 'return'"); */
    if (ch->desc)
        if (ch->desc->original)
            cmd_return(ch, "in_extract_hack", 0, 0);

    // This is needed because if a send_shadow/shadowwalk/shapechange creatur is returned
    // from, the return command removes the NPC.  Trying to double-extract the NPC can
    // cause problems.
    // -Nessalin 8/23/2003
    if (!ch) {
        return;
    }

    if (IS_NPC(ch))
        if (ch->nr >= 0) {
            if (ch->prev_num)
                ch->prev_num->next_num = ch->next_num;
            if (ch->next_num)
                ch->next_num->prev_num = ch->prev_num;
            if (ch == (CHAR_DATA *) npc_index[ch->nr].list)
                npc_index[ch->nr].list = ch->next_num;

            npc_index[ch->nr].number--;
        }

    if (ch->desc) {
        ch->desc->connected = CON_SLCT;
        show_main_menu(ch->desc);
        return;
    }

    if (shutdown_game) {
        free_char(ch);
        return;
    }

    extracted_thing *et = malloc(sizeof(*et));
    et->type = ET_CHAR;
    et->ch = ch;
    et->next = et_list;
    et_list = et;
}


CHAR_DATA *
get_char_room_vis(CHAR_DATA * ch, const char *name)
{
   return get_char_other_room_vis(ch, ch->in_room, name);
}

CHAR_DATA *
get_char_other_room_vis(CHAR_DATA *ch, ROOM_DATA *room, const char *name) 
{
    CHAR_DATA *i;
    int j, number;
    char tmpname[MAX_INPUT_LENGTH];
    const char *tmp;

    if (!ch || !*name || !room)
        return 0;

    if (room == ch->in_room 
     && ((!stricmp(name, "self")) || (!stricmp(name, "me")))
     && CAN_SEE(ch, ch))
        return ch;

    strcpy(tmpname, name);
    tmp = tmpname;
    if (!(number = get_number(&tmp)))
        return (0);

    for (i = room->people, j = 1; i && (j <= number); i = i->next_in_room)
        if (check_char_name(ch, i, tmp)) {
            if (j == number)
                return i;
            j++;
        }

    return (0);
}

CHAR_DATA *
get_char_vis(CHAR_DATA * ch, const char *name)
{
    CHAR_DATA *i;
    int j, number;
    char tmpname[MAX_INPUT_LENGTH];
    const char *tmp;

    if (!*name)
        return (0);

    if (((!stricmp(name, "self")) || (!stricmp(name, "me"))) && CAN_SEE(ch, ch))
        return (ch);

    strcpy(tmpname, name);
    tmp = tmpname;
    if (!(number = get_number(&tmp)))
        return (0);

    for (i = ch->in_room->people, j = 1; i && (j <= number); i = i->next_in_room)
        if (check_char_name(ch, i, tmp)) {
            if (j == number)
                return (i);
            j++;
        }

    for (i = character_list; i && (j <= number); i = i->next)
        if ((i->in_room) != (ch->in_room) && check_char_name(ch, i, tmp)) {
            if (j == number)
                return (i);
            j++;
        }

    return (0);
}

CHAR_DATA *
get_char_world(CHAR_DATA * ch, const char *name) 
{
   return get_char_world_raw(ch, name, TRUE);
}

CHAR_DATA *
get_char_world_raw(CHAR_DATA * ch, const char *name, bool useCurrSdesc)
{
    CHAR_DATA *i;
    int j, number;
    char tmpname[MAX_INPUT_LENGTH];
    const char *tmp;


    if (!*name)
        return (0);

    if (((!stricmp(name, "self")) || (!stricmp(name, "me"))) && CAN_SEE(ch, ch))
        return (ch);

    strcpy(tmpname, name);
    tmp = tmpname;
    if (!(number = get_number(&tmp)))
        return (0);

    for (i = ch->in_room->people, j = 1; i && (j <= number); i = i->next_in_room)
        if (check_char_name_world(ch, i, tmp)) {
            if (j == number)
                return (i);
            j++;
        }

    for (i = character_list; i && (j <= number); i = i->next)
        if ((i->in_room) != (ch->in_room) && check_char_name_world_raw(ch, i, tmp, useCurrSdesc)) {
            if (j == number)
                return (i);
            j++;
        }

    return (0);
}

CHAR_DATA *
get_char_world_all_name(CHAR_DATA * ch, const char *name) 
{
   return get_char_world_all_name_raw(ch, name, TRUE);
}

CHAR_DATA *
get_char_world_all_name_raw(CHAR_DATA * ch, const char *name, bool useCurrSdesc)
{
    CHAR_DATA *i;
    char tmpname[MAX_INPUT_LENGTH];

    if (!*name)
        return (NULL);

    strcpy(tmpname, name);

    if (((!stricmp(name, "self")) || (!stricmp(name, "me"))) && CAN_SEE(ch, ch))
        return (ch);

    for (i = ch->in_room->people; i; i = i->next_in_room)
        if (check_char_name_world_all_name(ch, i, tmpname)) {
            return (i);
        }

    for (i = character_list; i; i = i->next)
        if (i->in_room != ch->in_room && check_char_name_world_all_name_raw(ch, i, tmpname, useCurrSdesc)) {
            return (i);
        }

    return (NULL);
}



CHAR_DATA *
get_char_zone(int zone, const char *name)
{
    CHAR_DATA *i;
    int j, number;
    char tmpname[MAX_INPUT_LENGTH];
    const char *tmp;


    if (!*name)
        return (0);

    strcpy(tmpname, name);
    tmp = tmpname;
    if (!(number = get_number(&tmp)))
        return (0);


    for (i = character_list, j = 1; i && (j <= number); i = i->next)
        if (i->in_room && (i->in_room->number / 1000 == zone))
            if (isname(tmp, GET_NAME(i)) || isname(tmp, i->player.extkwds)) {
                if (j == number)
                    return (i);
                j++;
            }

    return (0);
}

CHAR_DATA *
get_char_zone_vis(int zone, CHAR_DATA * ch, const char *name)
{
    CHAR_DATA *i;
    int j, number;
    char tmpname[MAX_INPUT_LENGTH];
    const char *tmp;


    if (!*name)
        return (0);

    if (((!stricmp(name, "self")) || (!stricmp(name, "me"))) && CAN_SEE(ch, ch))
        return (ch);

    strcpy(tmpname, name);
    tmp = tmpname;
    if (!(number = get_number(&tmp)))
        return (0);

    for (i = ch->in_room->people, j = 1; i && (j <= number); i = i->next_in_room)
        if (check_char_name_world(ch, i, tmp)) {
            if (j == number)
                return (i);
            j++;
        }

    for (i = character_list; i && (j <= number); i = i->next)
        if ((i->in_room) != (ch->in_room) && (i->in_room) && (i->in_room->number / 1000) == (zone)
            && check_char_name(ch, i, tmp)) {
            if (j == number)
                return (i);
            j++;
        }

    return (0);
}




/* search all over the world for a char num, and return a pointer if found */
CHAR_DATA *
get_char_zone_num(int zone, int nr)
{
    CHAR_DATA *i;

    for (i = character_list; i; i = i->next)
        if (i->in_room && (i->in_room->number / 1000 == zone))
            if (i->nr == nr)
                return (i);

    return (0);
}





OBJ_DATA *
get_obj_in_list_vis_by_num(CHAR_DATA * ch, int num, OBJ_DATA * list)
{
    OBJ_DATA *i;
    int j;

    for (i = list, j = 1; i; i = i->next_content, j++)
        if (CAN_SEE_OBJ(ch, i))
            if (j == num)
                return (i);
    return (0);
}


OBJ_DATA *
get_obj_in_list_vis(CHAR_DATA * ch, const char *name, OBJ_DATA * list)
{
    OBJ_DATA *i;
    int j, number;
    char tmpname[MAX_INPUT_LENGTH];
    const char *tmp;

    strcpy(tmpname, name);
    tmp = tmpname;
    if (!(number = get_number(&tmp)))
        return (0);

    for (i = list, j = 1; i && (j <= number); i = i->next_content)
        if ((isname(tmp, OSTR(i, name))) || (isname(tmp, real_sdesc_keywords_obj(i, ch))))
            if (CAN_SEE_OBJ(ch, i)) {
                if (j == number)
                    return (i);
                j++;
            }
    return (0);
}

/* search carrying & room for object, and return a pointer */
OBJ_DATA *
get_obj_room_vis(CHAR_DATA * ch, const char *arg)
{
    OBJ_DATA *i;
    int j, number;
    char tmpname[MAX_INPUT_LENGTH];
    const char *tmp;

    strcpy(tmpname, arg);
    tmp = tmpname;
    if (!(number = get_number(&tmp)))
        return NULL;

    /* scan items carried */
    for (i = ch->carrying, j = 1; i && j <= number; i = i->next_content)
        if ((isname(tmp, OSTR(i, name))) || (isname(tmp, real_sdesc_keywords_obj(i, ch))))
            if (CAN_SEE_OBJ(ch, i))
                if (j++ == number)
                    return (i);

    /* scan room */
    for (i = ch->in_room->contents, j = 1; i && j <= number; i = i->next_content)
        if ((isname(tmp, OSTR(i, name))) || (isname(tmp, real_sdesc_keywords_obj(i, ch))))
            if (CAN_SEE_OBJ(ch, i))
                if (j++ == number)
                    return (i);

    return (NULL);
}


/*search the entire world for an object, and return a pointer  */
OBJ_DATA *
get_obj_vis(CHAR_DATA * ch, const char *name)
{
    OBJ_DATA *i;
    int j, number;
    char tmpname[MAX_INPUT_LENGTH];
    const char *tmp;

    strcpy(tmpname, name);
    tmp = tmpname;
    if (!(number = get_number(&tmp)))
        return (0);

    i = get_obj_room_vis(ch, name);
    if (i)
        return i;

    // FIXME:  object_list_foreach
    /* ok.. no luck yet. scan the entire obj list   */
    for (i = object_list, j = 1; i && j <= number; i = i->next)
        if ((isname(tmp, OSTR(i, name))) || (isname(tmp, real_sdesc_keywords_obj(i, ch))))
            if (CAN_SEE_OBJ(ch, i))
                if (j++ == number)
                    return (i);

    return (NULL);
}                               /* get_obj_vis */



/*
   #define MONEY_OBJECT 1951
 */

#define MONEY_OBJ_ONE      1997
#define MONEY_OBJ_SOME     1998
#define MONEY_OBJ_PILE     1999

OBJ_DATA *
create_money(int amount, int type)
{
    OBJ_DATA *obj;
    if (amount <= 0) {
        errorlog("Attempt to create negative money.");
        return NULL;
    }

    if (amount == 1)
        obj = read_object(coins[type].sing_vnum, VIRTUAL);
    else if ((amount > 1) && (amount < 20))
        obj = read_object(coins[type].some_vnum, VIRTUAL);
    else
        obj = read_object(coins[type].many_vnum, VIRTUAL);

    if (!obj)
        return NULL;

    obj->obj_flags.value[0] = amount;
    obj->obj_flags.cost = amount;
    /* using ixian's coin weight figures */
    obj->obj_flags.weight = MAX((int) (amount / 200), 1) * 100;
    return obj;
}



/* Generic Find, designed to find any object/character                    */
/* Calling :                                                              */
/*  *arg     is the sting containing the string to be searched for.       */
/*           This string doesn't have to be a single word, the routine    */
/*           extracts the next word itself.                               */
/*  bitv..   All those bits that you want to "search through".            */
/*           Bit found will be result of the function                     */
/*  *ch      This is the person that is trying to "find"                  */
/*  **tar_ch Will be NULL if no character was found, otherwise points     */
/* **tar_obj Will be NULL if no object was found, otherwise points        */
/*                                                                        */
/* The routine returns an integer indicating which flag resulted in a     */
/* match.                                                                 */

int
generic_find(const char *arg, int bitvector, CHAR_DATA * ch, CHAR_DATA ** tar_ch,
             OBJ_DATA ** tar_obj)
{
    int drink_type;
    static const char * const ignore[] = {
        "the",
        "in",
        "on",
        "at",
        "\n"
    };

    int i, count, number;
    char name[256];
    char tmpname[MAX_INPUT_LENGTH];
    const char *tmp;
    bool found;
    OBJ_DATA *obj;
    CHAR_DATA *rch;
    found = FALSE;


    /* Eliminate spaces and "ignore" words */
    while (*arg && !found) {
        for (; *arg == ' '; arg++);

        for (i = 0; (name[i] = *(arg + i)) && (name[i] != ' '); i++);
        name[i] = 0;
        arg += i;
        if (search_block(name, ignore, TRUE) > -1)
            found = TRUE;
    }

    if (!name[0])
        return (0);

#ifdef DONT_IGNORE_BRACKETED_WORDS

    /* -------------------------------- */
    /* look for any [ or ] in the word  */
    /* and ignore it if it has either   */
    /*     by suggestion of Azroen      */
    /*              -Tenebrius 4/7/1998 */
    /* -------------------------------- */

    for (i = 0; i < strlen(name); i++) {
        if (name[i] == '[')
            return (0);

        if (name[i] == ']')
            return (0);
    }

#endif

    *tar_ch = 0;
    *tar_obj = 0;

    /* for objects, grab the number for #.object */
    strcpy(tmpname, name);
    tmp = tmpname;
    if (!(number = get_number(&tmp)))
        return (0);

    count = 1;

    /* Find person in room */
    if (IS_SET(bitvector, FIND_CHAR_ROOM)) {
        for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
            if (check_char_name(ch, rch, tmp)) {
                if (count == number) {
                    *tar_ch = rch;
                    return (FIND_CHAR_ROOM);
                }
                count++;
            }
        }
    }

    if (IS_SET(bitvector, FIND_CHAR_WORLD)) {
        if ((*tar_ch = get_char_vis(ch, name))) {
            return (FIND_CHAR_WORLD);
        }
    }

    if (IS_SET(bitvector, FIND_OBJ_INV)) {
        for (obj = ch->carrying; obj && (count <= number); obj = obj->next_content)
            if (isname(tmp, real_sdesc_keywords_obj(obj, ch)) || (isname(tmp, OSTR(obj, name))))
                if (CAN_SEE_OBJ(ch, obj)) {
                    if (count == number) {
                        *tar_obj = obj;
                        return (FIND_OBJ_INV);
                    }
                    count++;
                }
    }

    if (IS_SET(bitvector, FIND_OBJ_EQUIP)) {
        for (i = 0; i < MAX_WEAR && (count <= number); i++)
            if (ch->equipment[i]
                && (isname(tmp, OSTR(ch->equipment[i], name))
                    || isname(tmp, real_sdesc_keywords_obj(ch->equipment[i], ch)))) {
                if (count == number) {
                    *tar_obj = ch->equipment[i];
                    return (FIND_OBJ_EQUIP);
                }
                count++;
            }
    }

    if (IS_SET(bitvector, FIND_OBJ_ROOM)) {
        for (obj = ch->in_room->contents; obj && (count <= number); obj = obj->next_content)
            if (isname(tmp, real_sdesc_keywords_obj(obj, ch)) || (isname(tmp, OSTR(obj, name))))
                if (CAN_SEE_OBJ(ch, obj)) {
                    if (count == number) {
                        *tar_obj = obj;
                        return (FIND_OBJ_ROOM);
                    }
                    count++;
                }
    }

    if (IS_SET(bitvector, FIND_OBJ_ROOM_ON_OBJ)) {
        OBJ_DATA *robj;

        for (robj = ch->in_room->contents; robj && (count <= number); robj = robj->next_content) {
            if (GET_ITEM_TYPE(robj) == ITEM_FURNITURE && IS_SET(robj->obj_flags.value[1], FURN_PUT)) {
                for (obj = robj->contains; obj && (count <= number); obj = obj->next_content) {
                    if (isname(tmp, real_sdesc_keywords_obj(obj, ch))
                        || (isname(tmp, OSTR(obj, name)))) {
                        if (CAN_SEE_OBJ(ch, obj)) {
                            if (count == number) {
                                *tar_obj = obj;
                                return (FIND_OBJ_ROOM_ON_OBJ);
                            }
                        }
                    }
                }
            }
        }
    }

    if (IS_SET(bitvector, FIND_OBJ_WORLD)) {
        if ((*tar_obj = get_obj_vis(ch, name))) {
            return (FIND_OBJ_WORLD);
        }
    }


    if ((IS_SET(bitvector, FIND_LIQ_CONT_INV) || IS_SET(bitvector, FIND_LIQ_CONT_ROOM)
         || IS_SET(bitvector, FIND_LIQ_CONT_EQUIP) || IS_SET(bitvector, FIND_LIQ_ROOM))
        && ((drink_type = search_block(name, drinknames, TRUE)) >= 0)) {
        shhlog("DOing the liquid thing");
        if (IS_SET(bitvector, FIND_LIQ_CONT_INV))
            for (obj = ch->carrying; obj; obj = obj->next_content)
                if (obj->obj_flags.type == ITEM_DRINKCON)
                    if (obj->obj_flags.value[2] == drink_type)
                        if ((obj->obj_flags.value[0] != 0)
                            || (IS_SET(obj->obj_flags.value[3], LIQFL_INFINITE)))
                            if (CAN_SEE_OBJ(ch, obj)) {
                                shhlog("DOing the liquid thing");
                                *tar_obj = obj;
                                return (FIND_LIQ_CONT_INV);
                            }


        if (IS_SET(bitvector, FIND_LIQ_CONT_ROOM) && ch->in_room)
            for (obj = ch->in_room->contents; obj; obj = obj->next_content)
                if (obj->obj_flags.type == ITEM_DRINKCON)
                    if (obj->obj_flags.value[2] == drink_type)
                        if ((obj->obj_flags.value[0] != 0)
                            || (IS_SET(obj->obj_flags.value[3], LIQFL_INFINITE)))
                            if (CAN_SEE_OBJ(ch, obj)) {
                                *tar_obj = obj;
                                return (FIND_LIQ_CONT_ROOM);
                            }
        if (IS_SET(bitvector, FIND_LIQ_ROOM))
            if (ch->in_room) {
                if ((ch->in_room->sector_type == SECT_SILT) && drink_type == LIQ_SILT)
                    return (FIND_LIQ_ROOM);
                else if ((ch->in_room->sector_type == SECT_DESERT) && drink_type == LIQ_SAND)
                    return (FIND_LIQ_ROOM);
            }
    }
    return (0);

}


/*
 * Find a keyword that works for the given target character for ch in the
 * current room.  (Mainly used for emotes)
 *  *tar_ch - Target to find in the room
 *  *ch - character who is looking
 *  *keyword - string to hold the resulting keyword
 *  *keyword_len length of the keyword object
 */
int
find_ch_keyword(CHAR_DATA * tar_ch, CHAR_DATA * ch, char *keyword, size_t keyword_len)
{
    int count = 1;
    char name[MAX_STRING_LENGTH];
    CHAR_DATA *rch;

    one_argument(MSTR(tar_ch, name), name, sizeof(name));

    if (!name[0]) {
        keyword[0] = '\0';
        return (0);
    }

    /* look through the people in the room to see if they have the same keyword
     * and increment the counter for them.
     */
    for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
        if (check_char_name(ch, rch, name)) {
            if (rch == tar_ch) {
                sprintf(keyword, "%d.%s", count, name);
                return (FIND_CHAR_ROOM);
            }
            count++;
        }
    }

    keyword[0] = '\0';
    return 0;
}

/*
 * Find a keyword that works for the given object for ch given the bitvector
 * Calling :
 *  *obj     is the object to be searched for.
 *  bitv..   All those bits that you want to "search through".
 *           Bit found will be result of the function
 *  *ch      This is the person that is trying to "find"
 *  *keyword resulting keyword that was found
 *  keyword_len  size of memory to write keyword in
 */
int
find_obj_keyword(OBJ_DATA * tar_obj, int bitvector, CHAR_DATA * ch, char *keyword,
                 size_t keyword_len)
{
    int i, count;
    char name[MAX_STRING_LENGTH];
    OBJ_DATA *obj = 0;
    CHAR_DATA *rch;

    one_argument(OSTR(tar_obj, name), name, sizeof(name));

    if (!name[0]) {
        keyword[0] = '\0';
        return (0);
    }

    count = 1;

    /* look through the people in the room to see if they have the same keyword
     * and increment the counter for them.
     */
    for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
        if (check_char_name(ch, rch, name)) {
            count++;
        }
    }

    /* look for the object in their inventory */
    if (IS_SET(bitvector, FIND_OBJ_INV)) {
        for (obj = ch->carrying; obj; obj = obj->next_content) {
            if (isname(name, real_sdesc_keywords_obj(obj, ch)) || (isname(name, OSTR(obj, name))))
                if (CAN_SEE_OBJ(ch, obj)) {
                    if (obj == tar_obj) {
                        sprintf(keyword, "%d.%s", count, name);
                        return (FIND_OBJ_INV);
                    }
                    count++;
                }
        }
    }

    /* look in equipment */
    if (IS_SET(bitvector, FIND_OBJ_EQUIP)) {
        for (i = 0; i < MAX_WEAR; i++) {
            obj = ch->equipment[i];
            if (obj && (isname(name, OSTR(obj, name))
                        || isname(name, real_sdesc_keywords_obj(obj, ch)))) {
                if (obj == tar_obj) {
                    sprintf(keyword, "%d.%s", count, name);
                    return (FIND_OBJ_EQUIP);
                }
                count++;
            }
        }
    }

    /* look in the room */
    if (IS_SET(bitvector, FIND_OBJ_ROOM)) {
        for (obj = ch->in_room->contents; obj; obj = obj->next_content) {
            if (isname(name, real_sdesc_keywords_obj(obj, ch)) || (isname(name, OSTR(obj, name)))) {
                if (CAN_SEE_OBJ(ch, obj)) {
                    if (obj == tar_obj) {
                        sprintf(keyword, "%d.%s", count, name);
                        return (FIND_OBJ_ROOM);
                    }
                    count++;
                }
            }
        }
    }

    /* look in the room for an object on the object */
    if (IS_SET(bitvector, FIND_OBJ_ROOM_ON_OBJ)) {
        OBJ_DATA *robj;

        for (robj = ch->in_room->contents; robj; robj = robj->next_content) {
            if (GET_ITEM_TYPE(robj) == ITEM_FURNITURE && IS_SET(robj->obj_flags.value[1], FURN_PUT)) {
                for (obj = robj->contains; obj; obj = obj->next_content) {
                    if (isname(name, real_sdesc_keywords_obj(obj, ch))
                        || (isname(name, OSTR(obj, name)))) {
                        if (CAN_SEE_OBJ(ch, obj)) {
                            if (obj == tar_obj) {
                                sprintf(keyword, "%d.%s", count, name);
                                return (FIND_OBJ_ROOM_ON_OBJ);
                            }
                            count++;
                        }
                    }
                }
            }
        }
    }

    keyword[0] = '\0';
    return (0);
}

void
show_keyword_char_to_char(CHAR_DATA * ch, int count, const char *keyword, CHAR_DATA * vict)
{
    char name[MAX_STRING_LENGTH];

    name[0] = '\0';

    if (IS_IMMORTAL(ch) && IS_NPC(vict)) {
        sprintf(name, " - [#%d]", npc_index[vict->nr].vnum);
    } else if (IS_IMMORTAL(ch)) {
        sprintf(name, " - %s (%s)", vict->name, vict->account);
    }

    cprintf(ch, "%3d.%s - %s%s\n\r", count, keyword, PERS(ch, vict), name);
}

void
show_keyword_obj_to_char(CHAR_DATA * ch, int count, const char *keyword, OBJ_DATA * obj)
{
    char vnum[MAX_STRING_LENGTH];

    vnum[0] = '\0';

    if (IS_IMMORTAL(ch)) {
        sprintf(vnum, " - [#%d]", obj_index[obj->nr].vnum);
    }

    cprintf(ch, "%3d.%s - %s%s\n\r", count, keyword, format_obj_to_char(obj, ch, 1), vnum);
}

void
show_keyword_found(const char *name, int bitvector, CHAR_DATA * ch)
{
    int i, count;
    OBJ_DATA *obj = 0;
    CHAR_DATA *rch;
    bool foundObjInRoom = FALSE;
    bool foundChInRoom = FALSE;
    bool foundInInv = FALSE;
    bool foundInEq = FALSE;
    bool needNL = FALSE;

    if (!name[0]) {
        return;
    }

    count = 1;

    /* look through the people in the room to see if they have the same keyword
     * and increment the counter for them.
     */
    for (rch = ch->in_room->people; rch; rch = rch->next_in_room) {
        if (check_char_name(ch, rch, name)) {
            if (CAN_SEE(ch, rch)) {
                if (!foundChInRoom) {
                    cprintf(ch, "In the room:\n\r");
                    foundChInRoom = TRUE;
                    needNL = TRUE;
                }
                show_keyword_char_to_char(ch, count, name, rch);
                count++;
            }
        }
    }

    /* look for the object in their inventory */
    if (IS_SET(bitvector, FIND_OBJ_INV)) {
        for (obj = ch->carrying; obj; obj = obj->next_content) {
            if (isname(name, real_sdesc_keywords_obj(obj, ch)) || (isname(name, OSTR(obj, name)))) {
                if (CAN_SEE_OBJ(ch, obj)) {
                    if (needNL)
                        cprintf(ch, "\n\r");
                    if (!foundInInv) {
                        cprintf(ch, "In Inventory:\n\r");
                        foundInInv = TRUE;
                        needNL = TRUE;
                    }
                    show_keyword_obj_to_char(ch, count, name, obj);
                    count++;
                }
            }
        }
    }

    /* look in equipment */
    if (IS_SET(bitvector, FIND_OBJ_EQUIP)) {
        for (i = 0; i < MAX_WEAR; i++) {
            obj = ch->equipment[i];
            if (obj && (isname(name, OSTR(obj, name))
                        || isname(name, real_sdesc_keywords_obj(obj, ch)))) {
                if (needNL)
                    cprintf(ch, "\n\r");
                if (!foundInEq) {
                    cprintf(ch, "In Equipment:\n\r");
                    foundInEq = TRUE;
                    needNL = TRUE;
                }
                show_keyword_obj_to_char(ch, count, name, obj);
                count++;
            }
        }
    }

    /* look in the room */
    if (IS_SET(bitvector, FIND_OBJ_ROOM)) {
        for (obj = ch->in_room->contents; obj; obj = obj->next_content) {
            if (isname(name, real_sdesc_keywords_obj(obj, ch)) || (isname(name, OSTR(obj, name)))) {
                if (CAN_SEE_OBJ(ch, obj)) {
                    if (needNL)
                        cprintf(ch, "\n\r");
                    if (!foundObjInRoom) {
                        cprintf(ch, "In the room:\n\r");
                        foundObjInRoom = TRUE;
                        needNL = TRUE;
                    }
                    show_keyword_obj_to_char(ch, count, name, obj);
                    count++;
                }
            }
        }
    }

    /* look in the room for an object on the object */
    if (IS_SET(bitvector, FIND_OBJ_ROOM_ON_OBJ)) {
        OBJ_DATA *robj;

        for (robj = ch->in_room->contents; robj; robj = robj->next_content) {
            bool foundOnObj = FALSE;
            if (GET_ITEM_TYPE(robj) == ITEM_FURNITURE && IS_SET(robj->obj_flags.value[1], FURN_PUT)) {
                for (obj = robj->contains; obj; obj = obj->next_content) {
                    if (isname(name, real_sdesc_keywords_obj(obj, ch))
                        || isname(name, OSTR(obj, name))) {
                        if (CAN_SEE_OBJ(ch, obj)) {
                            if (!foundOnObj) {
                                cprintf(ch, "On %s:\n\r", format_obj_to_char(robj, ch, 1));
                                foundOnObj = TRUE;
                                needNL = TRUE;
                            }
                            show_keyword_obj_to_char(ch, count, name, obj);
                            count++;
                        }
                    }
                }
            }
        }
    }

    /* handle not found */
    if (count == 1) {
        cprintf(ch, "No '%s' found.\n\r", name);
    }
}

OBJ_DATA *
find_reserved_obj(void)
{
    int i;
    OBJ_DATA *obj;

    for (i = 73000; i <= 73999; i++)
        if (real_object(i) < 0) /* Winner, it's not being used. */
            break;

    if (i > 73999)              /* We're all out, don't give them anything. */
        return NULL;

    add_new_item_in_creation(i);
    if (!(obj = read_object(i, VIRTUAL)))       /* Couldn't load it */
        return NULL;

    return obj;

}

void
free_reserved_obj(OBJ_DATA * rm_obj)
{
    int nr, i, j;
    int new_top_of_obj_t;
    struct index_data *new_obj_index;
    struct obj_default_data *new_obj_default;
    OBJ_DATA *obj, *next_obj;

    if (!rm_obj)
        return;

    if (rm_obj->nr > 73999 || rm_obj->nr < 73000)
        return;

    nr = real_object(rm_obj->nr);
    if (nr == -1)               /* No such object, damn we're good it's already gone. */
        return;

    /* fix/destroy existing objects */
    for (obj = object_list; obj; obj = next_obj) {
        next_obj = obj->next;
        if (obj->nr == nr)
            extract_obj(obj);
        else if (obj->nr > nr)
            obj->nr--;
    }

    new_top_of_obj_t = top_of_obj_t - 1;
    CREATE(new_obj_index, struct index_data, new_top_of_obj_t + 1);
    CREATE(new_obj_default, struct obj_default_data, new_top_of_obj_t + 1);

    for (i = 0; i < nr; i++) {
        new_obj_index[i] = obj_index[i];
        new_obj_default[i] = obj_default[i];
    }

    for (i = nr + 1; i <= top_of_obj_t; i++) {
        new_obj_index[i - 1] = obj_index[i];
        new_obj_default[i - 1] = obj_default[i];
    }

    free((char *) obj_index);
    obj_index = new_obj_index;
    free((char *) obj_default);
    obj_default = new_obj_default;
    top_of_obj_t = new_top_of_obj_t;

    for (i = 0; i <= top_of_zone_table; i++)
        for (j = 0; zone_table[i].cmd[j].command != 'S'; j++)
            switch (zone_table[i].cmd[j].command) {
            case 'O':
                if (zone_table[i].cmd[j].arg1 > nr)
                    zone_table[i].cmd[j].arg1--;
                break;
            case 'P':
                if (zone_table[i].cmd[j].arg1 > nr)
                    zone_table[i].cmd[j].arg1--;
                if (zone_table[i].cmd[j].arg3 > nr)
                    zone_table[i].cmd[j].arg3--;
                break;
            case 'A':
                if (zone_table[i].cmd[j].arg1 > nr)
                    zone_table[i].cmd[j].arg1--;
                if (zone_table[i].cmd[j].arg3 > nr)
                    zone_table[i].cmd[j].arg3--;
                break;
            case 'G':
                if (zone_table[i].cmd[j].arg1 > nr)
                    zone_table[i].cmd[j].arg1--;
                break;
            case 'E':
                if (zone_table[i].cmd[j].arg1 > nr)
                    zone_table[i].cmd[j].arg1--;
                break;
            default:
                break;
            }
    update_producing_d(nr);

    save_objects(RESERVED_Z);
    return;
}

void
clear_reserved_room(ROOM_DATA *room)
{
    room->next = room_list;
    room_list = room;
    room->next_in_zone = zone_table[RESERVED_Z].rooms;
    zone_table[RESERVED_Z].rooms = room;
    zone_table[RESERVED_Z].rcount++;

    room->zone = RESERVED_Z;
    room->room_flags = 0;
    room->sector_type = SECT_INSIDE;
    room->name = strdup("Reserved Room");
    room->bdesc_index = 0;
    room->description = strdup("Empty.\n\r");
    room->ex_description = NULL;
    for (int i = 0; i < 6; i++) {
        if (room->direction[i]) {
            free(room->direction[i]->general_description);
            free(room->direction[i]->keyword);
            free(room->direction[i]);
        }
        room->direction[i] = NULL;
    }
    room->light = 0;
    free_specials(room->specials);
    room->specials = 0;
    room->state = 0;
    room->contents = NULL;
    room->people = NULL;
}

ROOM_DATA *
find_reserved_room(void)
{
    int num = 73000;
    ROOM_DATA *room, *tmp_room = 0;

    for (room = zone_table[RESERVED_Z].rooms; room; room = room->next_in_zone)
        if (!room->people && !room->contents && !room->description) {
            clear_reserved_room(room);
            return room;
        }

    for (; num <= zone_table[RESERVED_Z].top; num++)
        /* An uncreated room in the res'd area */
        if ((tmp_room = get_room_num(num)) == NULL)
            break;

    if (num > zone_table[RESERVED_Z].top)
        return NULL;

    CREATE(room, ROOM_DATA, 1);
    clear_reserved_room(room);
    room->number = num;

    return room;
}

void
free_reserved_room(ROOM_DATA *room)
{
    if (!room)
        return;

    if (room->number > 73999 || room->number < 73000)
        return;

    extract_room(room, 1);
    save_rooms(&zone_table[RESERVED_Z], RESERVED_Z);
    return;
}

CHAR_DATA *
find_opponent_in_room(CHAR_DATA * ch)
{
    CHAR_DATA *tmpch;

    if (!ch->in_room)
        return NULL;
    if (IS_CALMED(ch))
        return NULL;
    if (!IS_SET(ch->specials.act, CFL_AGGRESSIVE) && !ch->specials.hates)
        return NULL;

    if (ch->specials.fighting) {
        if ((ch->in_room == ch->specials.fighting->in_room) && CAN_SEE(ch, ch->specials.fighting))
            return ch->specials.fighting;
        else if (ch->in_room != ch->specials.fighting->in_room)
            return NULL;
    } else {
        for (tmpch = ch->in_room->people; tmpch; tmpch = tmpch->next_in_room) {
            if (!IS_IMMORTAL(tmpch) && CAN_SEE(ch, tmpch) && does_hate(ch, tmpch))
                return tmpch;
            else if (IS_SET(ch->specials.act, CFL_AGGRESSIVE) && (GET_RACE(ch) == RACE_ELEMENTAL)
                     && (tmpch->player.guild != ch->player.guild) && CAN_SEE(ch, tmpch)
                     && !IS_IMMORTAL(tmpch))
                return tmpch;
            else if (IS_SET(ch->specials.act, CFL_AGGRESSIVE) && (GET_RACE(ch) == RACE_HALFLING)
                     && (GET_RACE(tmpch) = !RACE_HALFLING) && CAN_SEE(ch, tmpch)
                     && !IS_IMMORTAL(tmpch))
                return tmpch;
            else if (IS_SET(ch->specials.act, CFL_AGGRESSIVE) && (IS_TRIBE(ch, TRIBE_CAI_SHYZN)) && (!IS_TRIBE(tmpch, TRIBE_CAI_SHYZN)) && (IS_TRIBE(tmpch, !24999)) && /* is this right? !24999 */
                     CAN_SEE(ch, tmpch) && !IS_IMMORTAL(tmpch))
                return tmpch;
            else if (IS_SET(ch->specials.act, CFL_AGGRESSIVE) && (GET_RACE(ch) != RACE_ELEMENTAL)
                     && (GET_RACE(ch) != RACE_HALFLING) && (IS_IN_SAME_TRIBE(tmpch, ch))
                     && CAN_SEE(ch, tmpch) && !IS_IMMORTAL(tmpch))
                return tmpch;
            else if (tmpch->specials.fighting && (!IS_IN_SAME_TRIBE(tmpch->specials.fighting, ch))
                     && IS_IN_SAME_TRIBE(tmpch, ch) && CAN_SEE(ch, tmpch->specials.fighting))
                return tmpch->specials.fighting;
        }
    }

    return NULL;
}

bool
is_char_falling(CHAR_DATA * ch)
{
    struct char_fall_data *t;

    for (t = char_fall_list; t; t = t->next)
        if (t->falling == ch)
            return TRUE;
    return FALSE;
}

void
remove_char_from_falling(CHAR_DATA * ch)
{
    struct char_fall_data *remove, *t;
    if (is_char_falling(ch)) {
        if (char_fall_list->falling == ch) {
            remove = char_fall_list;
            char_fall_list = char_fall_list->next;
        } else {
            for (t = char_fall_list; t->next->falling != ch; t = t->next);
            remove = t->next;
            t->next = remove->next;
        }
        free((char *) remove);
    }
}

void
add_char_to_falling(CHAR_DATA * ch)
{
    struct char_fall_data *new;

    if (!is_char_falling(ch)) {
        CREATE(new, struct char_fall_data, 1);
        new->falling = ch;
        new->num_rooms = 0;
        new->next = char_fall_list;
        char_fall_list = new;
    }
}

bool
is_obj_expired(OBJ_DATA *obj)
{
    char dstr[32];
    if (!get_obj_extra_desc_value(obj, "[EXPIRATION]", dstr, ARRAY_LENGTH(dstr)))
        return FALSE;

    unsigned long endtime = strtoul(dstr, NULL, 10);
    if (endtime == 0)
        return FALSE;

    time_t now = time(NULL);
    if (now < endtime)
        return FALSE;

    return TRUE;
}

bool
is_obj_falling(OBJ_DATA *obj)
{
    struct obj_fall_data *t;

    for (t = obj_fall_list; t; t = t->next)
        if (t->falling == obj)
            return TRUE;
    return FALSE;
}

void
remove_obj_from_falling(OBJ_DATA * obj)
{
    struct obj_fall_data *remove, *t;

    if (is_obj_falling(obj)) {
        if (obj_fall_list->falling == obj) {
            remove = obj_fall_list;
            obj_fall_list = remove->next;
        } else {
            for (t = obj_fall_list; t->next->falling != obj; t = t->next);
            remove = t->next;
            t->next = remove->next;
        }
        free((char *) remove);
    }
}

void
add_obj_to_falling(OBJ_DATA * obj)
{
    struct obj_fall_data *new;

    if (!is_obj_falling(obj)) {
        CREATE(new, struct obj_fall_data, 1);
        new->falling = obj;
        new->next = obj_fall_list;
        obj_fall_list = new;
    }
}

void
release_extracted()
{
    struct timeval start;

    perf_enter(&start, "release_extracted()");
    while (et_list) {
        extracted_thing *et, *prev = NULL;

        /* Order is critical here, since we must free nodes in the same order
         * they were extracted.  Therefore, we chomp from the end. */
        for (et = et_list; et && et->next; et = et->next)
            prev = et;
        if (prev)
            prev->next = NULL;
        if (et == et_list)
            et_list = NULL;

        switch (et->type) {
        case ET_CHAR:  free_char(et->ch); break;
        case ET_OBJ: free_obj(et->obj); break;
        case ET_ROOM: free_room(et->rm); break;
        default:
            abort();  // This is fatally bad
        }
        free(et);
    }
    perf_exit("release_extracted()", start);
}

void
extract_room(ROOM_DATA * room, int fixup_links)
{
    int dir;
    ROOM_DATA *troom;
    CHAR_DATA *ch;

    room->watched_by = NULL;
    heap_delete(room, event_room_cmp);
    while ((ch = room->people)) {
        room->people = ch->next_in_room;
        if (IS_NPC(ch))
            ch->in_room = 0;
        ch->specials.was_in_room = NULL;
        extract_char(ch);
    }

    while (room->contents)
        extract_obj(room->contents);

    if (room_list == room) {
        room_list = room->next;
    } else {
        for (troom = room_list; troom->next != room; troom = troom->next);
        if (troom->next == room)
            troom->next = room->next;
    }

    zone_table[room->zone].rcount--;

    if (zone_table[room->zone].rooms == room) {
        zone_table[room->zone].rooms = room->next_in_zone;
    } else {
        for (troom = zone_table[room->zone].rooms; troom->next_in_zone != room;
             troom = troom->next_in_zone);
        troom->next_in_zone = room->next_in_zone;
    }

    if (fixup_links) {
        for (troom = room_list; troom; troom = troom->next) {
            for (dir = 0; dir < 6; dir++) {
                if (troom->direction[dir]) {
                    if (troom->direction[dir]->to_room == room) {
                        free(troom->direction[dir]->general_description);
                        free(troom->direction[dir]->keyword);
                        free((char *) troom->direction[dir]);
                        troom->direction[dir] = 0;
                    }
                }
            }

            if (troom->wagon_exit == room)
                troom->wagon_exit = NULL;
        }
    }

 //   JS_clear_room_pointer(room);

    if (shutdown_game) {
        free_room(room);
        return;
    }

    extracted_thing *et = malloc(sizeof(*et));
    et->type = ET_ROOM;
    et->rm = room;
    et->next = et_list;
    et_list = et;
}

int
PERCENTAGE(int amount, int max)
{
    if (max > 0)
        return (amount * 100) / max;

    return 0;
}

#define ADD_OR_UPDATE_EDESC(key,val,list)  \
    do { \
        struct extra_descr_data *finddesc; \
        struct extra_descr_data *newdesc;  \
        for (finddesc = (list); finddesc && strcmp(key, finddesc->keyword); finddesc = finddesc->next)  ; \
        if (!finddesc) {                   \
            global_game_stats.edescs++;    \
            CREATE(newdesc, struct extra_descr_data, 1); \
            newdesc->next = (list);        \
            (list) = newdesc;              \
            finddesc = newdesc;            \
            finddesc->keyword = key ? strdup(key) : strdup(""); \
            finddesc->description = val ? strdup(val) : strdup(""); \
            finddesc->def_descr = 0;       \
        } else {                           \
            if (val && strcmp((val), finddesc->description) == 0) \
               return(TRUE);  /* keyword and value match, so we're done */  \
            free(finddesc->description);   \
            finddesc->description = val ? strdup(val) : strdup(""); \
            finddesc->def_descr = 0;       \
            return(TRUE);     /* keyword was the same, updated description, we're done */ \
        } \
    } while(0);

int
set_obj_extra_desc_value(struct obj_data *obj, const char *set_name, const char *set_desc)
{
    ADD_OR_UPDATE_EDESC(set_name, set_desc, obj->ex_description);
    return (TRUE);
}

int
get_obj_extra_desc_value(struct obj_data *obj, const char *want_name, char *buffer, int length)
{
    size_t len;
    struct extra_descr_data *edesc;

    for (edesc = obj->ex_description; edesc && strcmp(edesc->keyword, want_name); edesc = edesc->next); /* do nothing find one wanted */

    if (!edesc)
        return (FALSE);

    /* Tenebrius remove comment and edesc if still working after 2004/10/01 */
    /* if someone is editing this, it will be null or something            */
#ifndef EDESC_FIX
    if (!(edesc->description))
        return (FALSE);
#endif

    len = strlen(edesc->description);
    if (len > length)
        return (FALSE);

    memcpy(buffer, edesc->description, (len + 1) * sizeof(char));
    return (TRUE);
}

int
set_char_extra_desc_value(struct char_data *ch, const char *set_name, const char *set_desc)
{
    ADD_OR_UPDATE_EDESC(set_name, set_desc, ch->ex_description);
    return (TRUE);
}

int
add_char_extra_desc_value(struct char_data *ch, const char *set_name, const char *set_desc)
{
    ADD_OR_UPDATE_EDESC(set_name, set_desc, ch->ex_description);
    return (TRUE);
}

int
get_char_extra_desc_value(struct char_data *ch, const char *want_name, char *buffer, int length)
{
    size_t len;
    struct extra_descr_data *edesc;

    for (edesc = ch->ex_description; edesc && strcmp(edesc->keyword, want_name); edesc = edesc->next);  /* do nothing find one wanted */

    if (!edesc)
        return (FALSE);

    /* Tenebrius remove comment and edesc if still working after 2004/10/01 */
    /* if someone is editing this, it will be null or something            */
#ifndef EDESC_FIX
    if (!(edesc->description))
        return (FALSE);
#endif

    len = strlen(edesc->description);
    if (len >= length)
        return (FALSE);

    // Since we've already calculated the length, a memcpy() will be faster than a strcpy()
    memcpy(buffer, edesc->description, sizeof(char) * (len + 1));
    return (TRUE);
}

int
rem_char_extra_desc_value(CHAR_DATA * ch, const char *rem_name)
{
    struct extra_descr_data *tmp = NULL, *last = NULL;

    for (tmp = ch->ex_description; tmp; last = tmp, tmp = tmp->next)
        if (strcmp(rem_name, tmp->keyword) == 0)
            break;

    if (!tmp)
        return 0;

    if (tmp == ch->ex_description)
        ch->ex_description = tmp->next;
    else {
        last->next = tmp->next;
    }

    free(tmp->keyword);
    free(tmp->description);
    free(tmp);
    global_game_stats.edescs--;

    return 1;
}

int
rem_obj_extra_desc_value(OBJ_DATA * obj, const char *rem_name)
{
    struct extra_descr_data *tmp = NULL, *last = NULL;

    for (tmp = obj->ex_description; tmp; last = tmp, tmp = tmp->next)
        if (strcmp(rem_name, tmp->keyword) == 0)
            break;

    if (!tmp)
        return 0;

    if (tmp == obj->ex_description)
        obj->ex_description = tmp->next;
    else {
        last->next = tmp->next;
    }

    free(tmp->keyword);
    free(tmp->description);
    free(tmp);
    global_game_stats.edescs--;

    return 1;
}

int
rem_room_extra_desc_value(ROOM_DATA * rm, const char *rem_name)
{
    struct extra_descr_data *tmp = NULL, *last = NULL;

    for (tmp = rm->ex_description; tmp; last = tmp, tmp = tmp->next)
        if (strcmp(rem_name, tmp->keyword) == 0)
            break;

    if (!tmp)
        return 0;

    if (tmp == rm->ex_description)
        rm->ex_description = tmp->next;
    else {
        last->next = tmp->next;
    }

    free(tmp->keyword);
    free(tmp->description);
    free(tmp);
    global_game_stats.edescs--;

    return 1;
}

int
set_room_extra_desc_value(struct room_data *room, const char *set_name, const char *set_desc)
{
    ADD_OR_UPDATE_EDESC(set_name, set_desc, room->ex_description);
    return (TRUE);
}

int
get_room_extra_desc_value(struct room_data *room, const char *want_name, char *buffer, int length)
{
    size_t len;
    struct extra_descr_data *edesc;

    for (edesc = room->ex_description; edesc && strcmp(edesc->keyword, want_name); edesc = edesc->next);        /* do nothing find one wanted */

    if (!edesc || !edesc->description)
        return (FALSE);

    len = strlen(edesc->description);
    if (len >= length)
        return (FALSE);

    memcpy(buffer, edesc->description, (len + 1) * sizeof(char));
    return (TRUE);
}


/*
 * Use a tool, if its of the given type reduce its number of uses
 */
bool
use_tool(OBJ_DATA * obj, int tool_type)
{
    if (obj == NULL || GET_ITEM_TYPE(obj) != ITEM_TOOL || obj->obj_flags.value[0] != tool_type)
        return FALSE;

    if (obj->obj_flags.value[1] <= 0)
        return FALSE;

    obj->obj_flags.value[1]--;
    return TRUE;
}


/*
 * Look for a item of a given type in the person's hands
 */
OBJ_DATA *
get_item_held(CHAR_DATA * ch, int itype)
{
    OBJ_DATA *obj;

    if ((obj = ch->equipment[EP]) != NULL && GET_ITEM_TYPE(obj) == itype)
        return obj;

    if ((obj = ch->equipment[ES]) != NULL && GET_ITEM_TYPE(obj) == itype)
        return obj;

    if ((obj = ch->equipment[ETWO]) != NULL && GET_ITEM_TYPE(obj) == itype)
        return obj;

    return NULL;
}

