/*
 * File: OCREATION.C
 * Usage: Routines and commands for online object creation.
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
#include <ctype.h>
#include <stdlib.h>

#include "constants.h"
#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "comm.h"
#include "parser.h"
#include "handler.h"
#include "forage.h"
#include "db.h"
#include "skills.h"
#include "limits.h"
#include "modify.h"
#include "guilds.h"
#include "cities.h"
#include "event.h"
#include "creation.h"
#include "barter.h"
#include "object_list.h"
#include "reporting.h"
#include "special.h"

int
add_new_item_in_creation(int virt_nr)
{
    int real_nr = alloc_obj(virt_nr, TRUE, TRUE);
    update_producing(real_nr);
    return (TRUE);
}

void ocopy_virtual(int vnum_source, int vnum_target ) {
    struct obj_default_data *source;
    struct obj_default_data *target;
    int rnum_target, rnum_source;
    int j;                      /*flag = FALSE, virt_nr, real_nr = 0, i; */

    add_new_item_in_creation(vnum_target);
    rnum_target = real_object(vnum_target);
    rnum_source = real_object(vnum_source);

    target = &(obj_default[rnum_target]);
    source = &(obj_default[rnum_source]);

    free(target->name);
    target->name = source->name ? strdup(source->name) : strdup("");

    free(target->short_descr);
    target->short_descr = source->short_descr ? strdup(source->short_descr) : strdup("");

    free(target->long_descr);
    target->long_descr = source->long_descr ? strdup(source->long_descr) : strdup("");

    free(target->description);
    target->description = source->description ? strdup(source->description) : strdup("");

    free_edesc_list(target->exdesc);
    target->exdesc = copy_edesc_list(source->exdesc);

    for (j = 0; j < 6; j++)
        target->value[j] = source->value[j];

    target->type = source->type;
    target->material = source->material;
    target->wear_flags = source->wear_flags;
    target->extra_flags = source->extra_flags;

    target->weight = source->weight;
    target->size = source->size;
    target->cost = source->cost;
    target->temp = source->temp;
    target->length = source->length;

    target->dam_dice_size = source->dam_dice_size;
    target->dam_num_dice = source->dam_num_dice;
    target->dam_plus = source->dam_plus;

    free_specials(target->programs);
    target->programs = copy_specials(source->programs);
    memcpy(&(target->affected[0]), &(source->affected[0]),
           sizeof(struct obj_affected_type) * MAX_OBJ_AFFECT);
}

void ocopy_real(OBJ_DATA *source, int vnum_target ) {
    struct obj_default_data *target;
    int rnum_target;
    int j;                      /*flag = FALSE, virt_nr, real_nr = 0, i; */

    add_new_item_in_creation(vnum_target);
    rnum_target = real_object(vnum_target);

    target = &(obj_default[rnum_target]);

    free(target->name);
    target->name = OSTR(source, name) ? strdup(OSTR(source, name)) : strdup("");

    free(target->short_descr);
    target->short_descr = OSTR(source, short_descr) ? strdup(OSTR(source, short_descr)) : strdup("");

    free(target->long_descr);
    target->long_descr = OSTR(source, long_descr) ? strdup(OSTR(source, long_descr)) : strdup("");

    free(target->description);
    target->description = OSTR(source, description) ? strdup(OSTR(source, description)) : strdup("");

    free_edesc_list(target->exdesc);
    target->exdesc = copy_edesc_list(source->ex_description);

    for (j = 0; j < 6; j++)
        target->value[j] = source->obj_flags.value[j];

    target->type = source->obj_flags.type;
    target->material = source->obj_flags.material;
    target->wear_flags = source->obj_flags.wear_flags;
    target->extra_flags = source->obj_flags.extra_flags;

    target->weight = source->obj_flags.weight;
    target->size = source->obj_flags.size;
    target->cost = source->obj_flags.cost;
    target->temp = source->obj_flags.temp;
    target->length = source->obj_flags.length;

    target->dam_dice_size = source->dam_dice_size;
    target->dam_num_dice = source->dam_num_dice;
    target->dam_plus = source->dam_plus;

    free_specials(target->programs);
    target->programs = copy_specials(source->programs);

    memcpy(&(target->affected[0]), &(source->affected[0]),
           sizeof(struct obj_affected_type) * MAX_OBJ_AFFECT);
}


void
cmd_ocopy(struct char_data *ch, const char *argument, Command cmd, int count)
{
    char buf1[256], buf2[256];
    OBJ_DATA *obj;
    int vnum_source, vnum_target;
    int rnum_source;

    SWITCH_BLOCK(ch);

    argument = two_arguments(argument, buf1, sizeof(buf1), buf2, sizeof(buf2));
    if (!*buf1 || !*buf2) {
        send_to_char("Usage: ocopy [<source#>|<source>] <target#>\n\r", ch);
        return;
    }

    vnum_target = atoi(buf2);

    if (!has_privs_in_zone(ch, vnum_target / 1000, MODE_CREATE)) {
        cprintf(ch,
         "You do not have string edit privileges in zone %d.(MODE_CREATE)\n\r",
         vnum_target / 100);
        return;
    }

    if (real_object(vnum_target) != -1) {
        cprintf(ch, "An object already exists with number %d.\n\r",
         vnum_target);
        return;
    }


    if( is_number(buf1)) {
        vnum_source = atoi(buf1);
        cprintf(ch, "Trying to copy from item #%d to #%d.\n\r",
         vnum_source, vnum_target);

        if (!has_privs_in_zone(ch, vnum_source / 1000, MODE_MODIFY)) {
            cprintf(ch,
             "You are not authorized to read item from zone %d.(MODE_MODIFY)\n\r",
              vnum_source / 1000);
            return;
        }

        rnum_source = real_object(vnum_source);
        if (rnum_source < 0) {
            cprintf(ch, "Source vnum didnt translate right. error");
            return;
        }

        ocopy_virtual(vnum_source, vnum_target);

        cprintf(ch, 
         "Done copying from default of item #%d to default of #%d.\n\r",
         vnum_source, vnum_target);
    } else {
        if( (obj = get_obj_in_list_vis( ch, buf1, ch->carrying ) ) == NULL ) {
            cprintf( ch, "You aren't carrying any '%s'.\n\r", buf1 );
            return;
        }

        ocopy_real(obj, vnum_target);
        cprintf(ch, "Done copying your %s to default of #%d.\n\r",
        OSTR(obj, short_descr), vnum_target);
    }
}

/* create an object */
void
cmd_ocreate(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256];
    /* int flag = FALSE, virt_nr, real_nr = 0;  i, j; */
    int virt_nr;
    /*   OBJ_DATA *tmpob, *obj; */
    OBJ_DATA *obj;

    argument = one_argument(argument, buf, sizeof(buf));

    if (0 == strlen(buf)) {
      send_to_char("Usage: ocreate <number>\n\r", ch);
      return;
    }

    if (!is_number(buf)) {
      cprintf(ch, "Usage: ocreate <number>\n\r'%s' is not a number.\n\r", buf);
      return;
    }

    virt_nr = atoi(buf);

    if (virt_nr < 0) {
        send_to_char("You can't do that.\n\r", ch);
        return;
    }

    //    if ((!zone_table[virt_nr/1000].is_open) && (GET_LEVEL(ch) < HIGHLORD ) && (virt_nr/1000 != 0)) {
    //      send_to_char ("You cannot create items from closed zones.\n\r", ch);
    //      return;
    //    }


    //    if (!has_privs_in_zone(ch, virt_nr / 1000, MODE_CREATE)) {
    //        send_to_char("You do not have creation privileges in that zone.\n\r", ch);
    //        return;
    //    }

    if (real_object(virt_nr) == -1) {
        if( IS_SWITCHED(ch) ) {
            cprintf(ch, "No such object #%d, can't create while switched.\n\r", virt_nr);
            return;
        }

        if (!has_privs_in_zone(ch, virt_nr / 1000, MODE_STR_EDIT)) {
            send_to_char("You do not have creation privileges in that " "zone.\n\r", ch);
            return;
        }
        add_new_item_in_creation(virt_nr);
    }

    if (!(obj = read_object(virt_nr, VIRTUAL))) {
        send_to_char("That object does not exist.\n\r", ch);
        return;
    }

    if( !IS_SWITCHED(ch)) {
        act("You call forth $p from the plane of Elemental Wind.", TRUE, ch, obj, 0, TO_CHAR);
        act("$n calls forth $p from the plane of Elemental Wind.", TRUE, ch, obj, 0, TO_ROOM);
    }

    if (obj->obj_flags.type != ITEM_WAGON)
        obj_to_char(obj, ch);
    else
        obj_to_room(obj, ch->in_room);
}


/* set an object's type */
void
set_otype(CHAR_DATA * ch, OBJ_DATA * obj, char *arg)
{
    int type, num, i;

    if (!*arg) {
        send_to_char("Syntax: oset <object> type <type>\n\r", ch);
        send_to_char("The following types are available:\n\r", ch);
        send_attribs_to_char(obj_type, ch);
        return;
    }

    num = get_attrib_num(obj_type, arg);
    if (num == -1) {
        send_to_char("That object type is invalid.\n\r", ch);
        send_to_char("The following types are available:\n\r", ch);
        send_attribs_to_char(obj_type, ch);
        return;
    }

    type = obj_type[num].val;

    if ((type == -1) || (obj_type[num].priv > GET_LEVEL(SWITCHED_PLAYER(ch)))) {
        send_to_char("The following types are available:\n\r", ch);
        send_attribs_to_char(obj_type, ch);
        return;
    }

    obj->obj_flags.type = type;

    for (i = 0; i < 6; i++)
        obj->obj_flags.value[i] = oval_defaults[type][i];

    send_to_char("Done.\n\r", ch);
}


/* set flags for objects: sflag */
void
set_osflag(CHAR_DATA * ch, OBJ_DATA * obj, char *arg)
{
    char tmp[256];
    int num, flag;

    if (!*arg) {
        send_to_char("Syntax: oset <object> sflag <flag>\n\r", ch);
        send_to_char("You may set the following 'state' flags:\n\r", ch);
        send_flags_to_char(obj_state_flag, ch);
        return;
    }

    num = get_flag_num(obj_state_flag, arg);

    if ((num == -1) || (obj_state_flag[num].priv > GET_LEVEL(SWITCHED_PLAYER(ch)))) {
        send_to_char("That 'state' flag does not exist.\n\r", ch);
        send_to_char("You may set the following 'state' flags:\n\r", ch);
        send_flags_to_char(obj_state_flag, ch);
        return;
    }

    flag = obj_state_flag[num].bit;

    if (IS_SET(obj->obj_flags.state_flags, (flag))) {
        sprintf(tmp, "You have removed the '%s' state flag on %s (#%d).\n\r",
                obj_state_flag[num].name, OSTR(obj, short_descr), obj_index[obj->nr].vnum);
        send_to_char(tmp, ch);
        REMOVE_BIT(obj->obj_flags.state_flags, flag);
    } else {
        sprintf(tmp, "You have set the '%s' state flag on %s (#%d).\n\r", obj_state_flag[num].name,
                OSTR(obj, short_descr), obj_index[obj->nr].vnum);
        send_to_char(tmp, ch);
        MUD_SET_BIT(obj->obj_flags.state_flags, flag);
    }
}

/* set flags for objects: color_flag */
void
set_ocflag(CHAR_DATA * ch, OBJ_DATA * obj, char *arg)
{
    char tmp[256];
    int num, flag;

    if (!*arg) {
        send_to_char("Syntax: oset <object> cflag <flag>\n\r", ch);
        send_to_char("You may set the following 'color' flags:\n\r", ch);
        send_flags_to_char(obj_color_flag, ch);
        return;
    }

    num = get_flag_num(obj_color_flag, arg);

    if ((num == -1) || (obj_color_flag[num].priv > GET_LEVEL(SWITCHED_PLAYER(ch)))) {
        send_to_char("That 'color' flag does not exist.\n\r", ch);
        send_to_char("You may set the following 'color' flags:\n\r", ch);
        send_flags_to_char(obj_color_flag, ch);
        return;
    }

    flag = obj_color_flag[num].bit;

    if (IS_SET(obj->obj_flags.color_flags, (flag))) {
        sprintf(tmp, "You have removed the '%s' color flag on %s (#%d).\n\r",
                obj_color_flag[num].name, OSTR(obj, short_descr), obj_index[obj->nr].vnum);
        send_to_char(tmp, ch);
        REMOVE_BIT(obj->obj_flags.color_flags, flag);
    } else {
        sprintf(tmp, "You have set the '%s' color flag on %s (#%d).\n\r", obj_color_flag[num].name,
                OSTR(obj, short_descr), obj_index[obj->nr].vnum);
        send_to_char(tmp, ch);
        MUD_SET_BIT(obj->obj_flags.color_flags, flag);
    }
}

/* set flags for objects: adverbial_flag */
void
set_oaflag(CHAR_DATA * ch, OBJ_DATA * obj, char *arg)
{
    char tmp[256];
    int num, flag;

    if (!*arg) {
        send_to_char("Syntax: oset <object> adverbial_flag <flag>\n\r", ch);
        send_to_char("You may set the following 'adverbial' flags:\n\r", ch);
        send_flags_to_char(obj_adverbial_flag, ch);
        return;
    }

    num = get_flag_num(obj_adverbial_flag, arg);

    if ((num == -1) || (obj_adverbial_flag[num].priv > GET_LEVEL(SWITCHED_PLAYER(ch)))) {
        send_to_char("That 'adverbial' flag does not exist.\n\r", ch);
        send_to_char("You may set the following 'adverbial' flags:\n\r", ch);
        send_flags_to_char(obj_adverbial_flag, ch);
        return;
    }

    flag = obj_adverbial_flag[num].bit;

    if (IS_SET(obj->obj_flags.adverbial_flags, (flag))) {
        sprintf(tmp, "You have removed the '%s' adverbial flag on %s (#%d).\n\r",
                obj_adverbial_flag[num].name, OSTR(obj, short_descr), obj_index[obj->nr].vnum);
        send_to_char(tmp, ch);
        REMOVE_BIT(obj->obj_flags.adverbial_flags, flag);
    } else {
        sprintf(tmp, "You have set the '%s' adverbial flag on %s (#%d).\n\r",
                obj_adverbial_flag[num].name, OSTR(obj, short_descr), obj_index[obj->nr].vnum);
        send_to_char(tmp, ch);
        MUD_SET_BIT(obj->obj_flags.adverbial_flags, flag);
    }
}

/* set flags for objects: wflag, eflag */
void
set_owflag(CHAR_DATA * ch, OBJ_DATA * obj, char *arg)
{
    char tmp[256];
    int num, flag;

    if (!*arg) {
        send_to_char("Syntax: oset <object> wflag <flag>\n\r", ch);
        send_to_char("You may set the following 'wear' flags:\n\r", ch);
        send_flags_to_char(obj_wear_flag, ch);
        return;
    }

    num = get_flag_num(obj_wear_flag, arg);

    if ((num == -1) || (obj_wear_flag[num].priv > GET_LEVEL(SWITCHED_PLAYER(ch)))) {
        send_to_char("That 'wear' flag does not exist.\n\r", ch);
        send_to_char("You may set the following 'wear' flags:\n\r", ch);
        send_flags_to_char(obj_wear_flag, ch);
        return;
    }

    flag = obj_wear_flag[num].bit;

    if (IS_SET(obj->obj_flags.wear_flags, (flag))) {
        sprintf(tmp, "You have removed the '%s' wear flag on %s (#%d).\n\r",
                obj_wear_flag[num].name, OSTR(obj, short_descr), obj_index[obj->nr].vnum);
        send_to_char(tmp, ch);
        REMOVE_BIT(obj->obj_flags.wear_flags, flag);
    } else {
        sprintf(tmp, "You have set the '%s' wear flag on %s (#%d).\n\r", obj_wear_flag[num].name,
                OSTR(obj, short_descr), obj_index[obj->nr].vnum);
        send_to_char(tmp, ch);
        MUD_SET_BIT(obj->obj_flags.wear_flags, flag);
    }
}

void
set_oeflag(CHAR_DATA * ch, OBJ_DATA * obj, char *arg)
{                               /* Set_Oeflag */
    char tmp[256];
    int num, flag;

    if (!*arg) {
        send_to_char("Syntax: oset <object> eflag <eflag>\n\r", ch);
        send_to_char("You may set the following 'extra' flags:\n\r", ch);
        send_flags_to_char(obj_extra_flag, ch);
        return;
    }

    num = get_flag_num(obj_extra_flag, arg);

    if ((num == -1) || (obj_extra_flag[num].priv > GET_LEVEL(SWITCHED_PLAYER(ch)))) {
        send_to_char("That 'extra' flag does not exist.\n\r", ch);
        send_to_char("You may set the following 'extra' flags:\n\r", ch);
        send_flags_to_char(obj_extra_flag, ch);
        return;
    }

    flag = obj_extra_flag[num].bit;

    if (IS_SET(obj->obj_flags.extra_flags, (flag))) {
        sprintf(tmp, "You have removed the '%s' extra flag from %s (#%d).\n\r",
                obj_extra_flag[num].name, OSTR(obj, short_descr), obj_index[obj->nr].vnum);
        send_to_char(tmp, ch);
        REMOVE_BIT(obj->obj_flags.extra_flags, flag);
    } else {
        sprintf(tmp, "You have set the '%s' extra flag on %s (#%d).\n\r", obj_extra_flag[num].name,
                OSTR(obj, short_descr), obj_index[obj->nr].vnum);
        send_to_char(tmp, ch);
        MUD_SET_BIT(obj->obj_flags.extra_flags, flag);
    }
}                               /* Set_Oeflag */

void
set_material(CHAR_DATA * ch, OBJ_DATA * obj, char *arg)
{
    char tmp[256];
    int num, mat;

    if (!*arg) {
        send_to_char("Syntax: oset <object> material <material>\n\r", ch);
        send_to_char("You may set the following material types:\n\r", ch);
        send_attribs_to_char(obj_material, ch);
        return;
    }

    num = get_attrib_num(obj_material, arg);

    if ((num == -1) || (obj_material[num].priv > GET_LEVEL(SWITCHED_PLAYER(ch)))) {
        send_to_char("That material type does not exist.\n\r", ch);
        send_to_char("You may set the following material types:\n\r", ch);
        send_attribs_to_char(obj_material, ch);
        return;
    }

    mat = obj_material[num].val;

    obj->obj_flags.material = mat;
    sprintf(tmp, "%s's (#%d) material has been set to '%s'.\n\r", OSTR(obj, short_descr),
            obj_index[obj->nr].vnum, obj_material[num].name);
    send_to_char(tmp, ch);
}


/* set object affects */
void
set_oaffect(CHAR_DATA * ch, OBJ_DATA * obj, const char *arg, int a)
{
    char buf1[256];             /*, buf2[256]; */
    int num, type, mod;
    CHAR_DATA *orig = SWITCHED_PLAYER(ch);

    if (!*arg) {
        send_to_char("Syntax: oset <object> (aff1 | aff2) <amount> <affect>\n\r", ch);
        send_to_char("The following affect types are available: (plus skills)\n\r", ch);
        send_attribs_to_char(obj_apply_types, ch);
        return;
    }

    arg = one_argument(arg, buf1, sizeof(buf1));

    num = get_attrib_num(obj_apply_types, arg);
    mod = atoi(buf1);

    /* also be able to set a skill type */
    if (num == -1) {
        int sk;

        type = -1;

        /* search for skill */
        for (sk = 1; sk < MAX_SKILLS; sk++)
            if (!strnicmp(skill_name[sk], arg, strlen(arg))) {
                type = sk + CHAR_APPLY_SKILL_OFFSET;
                break;
            }
    } else
        type = obj_apply_types[num].val;


    if (type == CHAR_APPLY_NONE) {
        obj->affected[a].location = 0;
        obj->affected[a].modifier = 0;
        send_to_char("Done, set aff to none.\n\r", ch);
        return;
    };


    /* also be able to set a skill type */


    if ((type == -1)
        || ((type < CHAR_APPLY_SKILL_OFFSET) && (obj_apply_types[num].priv > GET_LEVEL(orig)))) {
        /*      char msg[2000]; */
        cprintf(ch, "The skill or affect '%s' does not exist.\n\r", arg);
        send_to_char("The following affect types are available: (plus skills)\n\r", ch);
        send_attribs_to_char(obj_apply_types, ch);
        return;
    };

    if (!mod) {
        send_to_char("You must specify an amount to affect it by.\n\r", ch);
        return;
    }

    if ((mod > 100) || (mod < -100)) {
        send_to_char("That object affect is too high.\n\r", ch);
        send_to_char("Valid values for affects are -100 to 100.\n\r", ch);
        return;
    }

    obj->affected[a].location = type;
    obj->affected[a].modifier = mod;
    send_to_char("Done.\n\r", ch);
}

/* set object damage */
void
set_odamage(CHAR_DATA * ch, OBJ_DATA * obj, char *arg, int a)
{
    char buf1[256], buf2[256], buf3[256];
    int num, size, plus;
    int num_scan;

    if (!*arg) {
        send_to_char("Syntax: oset <object> damage XdY+Z\n\r", ch);
        send_to_char("Examples: 5d6+2, 2d1+0, 8d12+4, etc...\n\r", ch);
        return;
    };

    num_scan = sscanf(arg, "%dd%d+%d", &num, &size, &plus);

    if (num_scan != 3) {
        send_to_char("Invalid values for damage.\n\r", ch);
        send_to_char("Syntax: oset <object> damage XdY+Z\n\r", ch);
        send_to_char("Examples: 5d6+2, 2d1+0, 8d12+4, etc...\n\r", ch);
        return;
    };

    sprintf(buf1, "%dd%d+%d", obj->dam_num_dice, obj->dam_dice_size, obj->dam_plus);

    sprintf(buf2, "%dd%d+%d", num, size, plus);


    obj->dam_num_dice = num;
    obj->dam_dice_size = size;
    obj->dam_plus = plus;

    sprintf(buf3, "%s's (#%d) damage has changed from %s to %s.\n\r", OSTR(obj, short_descr),
            obj_index[obj->nr].vnum, buf1, buf2);
    send_to_char(buf3, ch);
}


/* set an object's extra description */
void
set_oedesc(CHAR_DATA * ch, OBJ_DATA * obj, char *keywds)
{
    struct extra_descr_data *tmp, *newdesc;

    if (!*keywds) {
        send_to_char("Syntax: oset <object> edesc <keywords>\n\r", ch);
        return;
    }

    for (tmp = obj->ex_description; tmp; tmp = tmp->next)
        if (!stricmp(tmp->keyword, keywds))
            break;

    if (!tmp) {
        global_game_stats.edescs++;
        CREATE(newdesc, struct extra_descr_data, 1);
        newdesc->description = (char *) 0;
        newdesc->next = obj->ex_description;
        obj->ex_description = newdesc;
        newdesc->keyword = strdup(keywds);
    } else {
        newdesc = tmp;
    }

    act("$n has entered CREATING mode (updating item edesc).", TRUE, ch, 0, 0, TO_ROOM);
    if (newdesc->description && strlen(newdesc->description) > 0) {
        send_to_char("Continue editing description, use '.c' to clear.\n\r", ch);
        string_append(ch, &newdesc->description, MAX_DESC);
    } else {
        send_to_char("Enter a new description.\n\r", ch);
        string_edit(ch, &newdesc->description, MAX_DESC);
    }
}

void
set_odesc(CHAR_DATA * ch, OBJ_DATA * obj)
{
    if (!obj->description) {
        if (OSTR(obj, description))
            obj->description = strdup(OSTR(obj, description));
        else
            obj->description = strdup("");
    }

    act("$n has entered CREATING mode (updating item description).", TRUE, ch, 0, 0, TO_ROOM);
    if (obj->description && strlen(obj->description) > 0) {
        send_to_char("Continue editing description, use '.c' to clear.\n\r", ch);
        string_append(ch, &(obj->description), MAX_DESC);
    } else {
        send_to_char("Enter a new description.\n\r", ch);
        string_edit(ch, &(obj->description), MAX_DESC);
    }
}

/* unset an object's extra description */
void
unset_oedesc(CHAR_DATA * ch, OBJ_DATA * obj, const char *argument)
{
    char keywd[MAX_INPUT_LENGTH];

    argument = one_argument(argument, keywd, sizeof(keywd));

    if (!*keywd) {
        send_to_char("Syntax: ounset <object> edesc <keywords>\n\r", ch);
        send_to_char("You must give a list of keywords.\n\r", ch);
        return;
    }

    if (!rem_obj_extra_desc_value(obj, keywd)) {
        send_to_char("That extra description does not exist.\n\r", ch);
        return;
    }
    send_to_char("Done.\n\r", ch);
}

char *
show_oval(OBJ_DATA * obj, int val)
{
    char buf[1024];
    int j;                      /* Counter */
    int power;                  /* power level */
    int *ovals = obj->obj_flags.value;


    switch (obj->obj_flags.type) {
    case ITEM_COMPONENT:
        switch (val) {
        case 0:                /* sphere of effect */
            if ((ovals[val] > -1) && (ovals[val] <= LAST_SPHERE))
                return strdup(Sphere[ovals[val]]);
            break;
        case 1:                /* relative power of the component */
            if ((ovals[val] > -1) && (ovals[val] <= LAST_POWER))
                return strdup(Power[ovals[val]]);
        }
        break;
    case ITEM_WAND:
    case ITEM_STAFF:
        switch (val) {
        case 1:                /* power-level */
            if ((ovals[val] > 0) && (ovals[val] <= LAST_POWER))
                return strdup(Power[ovals[val] - 1]);
            break;
        case 3:                /* spell */
            if ((ovals[val] > 0) && (ovals[val] < MAX_SKILLS))
                return strdup(skill_name[ovals[val]]);
        }
        break;
    case ITEM_WEAPON:
        switch (val) {
        case 3:                /* weapon type */
            sprint_attrib(ovals[val] + 300, weapon_type, buf);
            return strdup(buf);
        case 5:                /* poison type */
            if ((ovals[val] >= 0) && (ovals[val] <= MAX_POISON))
                return strdup(poison_types[ovals[val]]);
        }
        break;
    case ITEM_TOOL:
        switch (val) {
        case 0:                /* type of tool */
            if ((ovals[val] > 0) && (ovals[val] <= MAX_TOOLS))
                return strdup(tool_name[ovals[val]]);
        }
        break;
    case ITEM_MUSICAL:
        switch (val) {
        case 1:                /* instrument_type */
            sprint_attrib(ovals[val], instrument_type, buf);
            return strdup(buf);
        case 2:                /* flags */
            return show_flags(musical_flags, ovals[val]);
        }
        break;
    case ITEM_JEWELRY:
	switch (val) {
        case 1:                /* flags */
            return show_flags(jewelry_flags, ovals[val]);
        }
        break;
    case ITEM_POISON:
        switch (val) {
        case 0:                /* type of poison */
            if ((ovals[val] >= 0) && (ovals[val] <= MAX_POISON))
                return strdup(poison_types[ovals[val]]);
        }
        break;
    case ITEM_CURE:
        switch (val) {
        case 0:                /* type of poison (that it cures) */
            if ((ovals[val] >= 0) && (ovals[val] <= MAX_POISON))
                return strdup(poison_types[ovals[val]]);
            break;
        case 2:                /* type of poison (side effect) */
            if ((ovals[val] >= 0) && (ovals[val] <= MAX_POISON))
                return strdup(poison_types[ovals[val]]);
            break;
        case 3:                /* taste */
            if ((ovals[val] >= 0) && (ovals[val] <= MAX_CURE_TASTES))
                return strdup(cure_tastes[ovals[val]]);
        }
        break;
    case ITEM_TREASURE:
        switch (val) {
        case 0:                /* ttype - treasure type */
            sprint_attrib(ovals[val], treasure_type, buf);
            return strdup(buf);
        }
        break;
    case ITEM_MINERAL:
        switch (val) {
        case 0:                /* mineral_type */
            sprint_flag(ovals[val], mineral_type, buf);
            return strdup(buf);
        case 1:                /* edibility */
            if (ovals[val])
                return strdup("edible");
            else
                return strdup("inedible");
            break;
        case 2:                /* grinding */
            if (ovals[val])
                return strdup("grindable");
            else
                return strdup("not grindable");
        }
        break;
    case ITEM_POWDER:
        switch (val) {
        case 0:                /* powder_type */
            sprint_flag(ovals[val], powder_type, buf);
            return strdup(buf);
        case 1:                /* solubility */
            if (ovals[val])
                return strdup("soluble");
            else
                return strdup("not soluble");
        }
        break;
    case ITEM_BOW:
        switch (val) {
        case 2:
            if (ovals[val])
                return strdup("true");
            else
                return strdup("false");
        }
        break;
    case ITEM_POTION:
        switch (val) {
        case 0:                /* power */
            power = ovals[val];
            power = power - 1;
            if ((power >= 0) && (power <= LAST_POWER))
                return strdup(Power[power]);
            break;
        case 1:                /* spell1 */
        case 2:                /* spell2 */
        case 3:                /* spell3 */
            if ((ovals[val] > -1) && (ovals[val] < MAX_SKILLS))
                return strdup(skill_name[ovals[val]]);
        }
        break;
    case ITEM_SCROLL:
        switch (val) {
        case 0:                /* power */
            power = ovals[val];
            power = power - 1;
            if ((power >= 0) && (power <= LAST_POWER))
                return strdup(Power[power]);
            break;
        case 1:                /* spell1 */
        case 2:                /* spell2 */
        case 3:                /* spell3 */
            if ((ovals[val] > -1) && (ovals[val] < MAX_SKILLS))
                return strdup(skill_name[ovals[val]]);
            break;
        case 4:                /* source */
            break;
        case 5:                /* language */
            if ((ovals[val] > -1) && (ovals[val] < MAX_SKILLS))
                return strdup(skill_name[ovals[val]]);
        }
        break;
    case ITEM_FURNITURE:
        switch (val) {
        case 1:                /* flags */
            return show_flags(furniture_flags, ovals[val]);
        }
        break;
    case ITEM_PLAYABLE:
        switch (val) {
        case 0:                /* play_type - playable type */
            sprint_attrib(ovals[val], playable_type, buf);
            return strdup(buf);
        }
        break;
    case ITEM_WORN:
        switch (val) {
        case 1:                /* flags */
            return show_flags(worn_flags, ovals[val]);
        }
        break;
    case ITEM_CONTAINER:
        switch (val) {
        case 1:                /* flags */
            return show_flags(container_flags, ovals[val]);
        }
        break;
    case ITEM_DRINKCON:
        switch (val) {
        case 2:
            if ((ovals[val] > -1) && (ovals[val] < MAX_LIQ))
                return strdup(drinknames[ovals[val]]);
            break;
        case 3:
            return show_flags(liquid_flags, ovals[val]);
        case 4:                /* type */
            if ((ovals[val] > -1) && (ovals[val] < MAX_SKILLS))
                return strdup(skill_name[ovals[val]]);
        case 5:                /* poison type */
            if ((ovals[val] >= 0) && (ovals[val] <= MAX_POISON))
                return strdup(poison_types[ovals[val]]);
        }
        break;
    case ITEM_FOOD:
        switch (val) {
        case 1:
            if ((ovals[val] > -1) && (ovals[val] < MAX_FOOD))
                return strdup(food_names[ovals[val]]);
            break;
        case 3:                /* type of poison */
            if ((ovals[val] >= 0) && (ovals[val] <= MAX_POISON))
                return strdup(poison_types[ovals[val]]);
        case 5:                /* type */
            if ((ovals[val] > -1) && (ovals[val] < MAX_SKILLS))
                return strdup(skill_name[ovals[val]]);
        }
        break;
    case ITEM_WAGON:
        switch (val) {
        case 3:
            /* sprint_attrib (ovals[val], wagon_type, buf); 
             * return strdup (buf); */
            return show_flags(wagon_flags, ovals[val]);
        }
        break;
    case ITEM_MONEY:
        switch (val) {
        case 1:                /* type of coin */
            if ((ovals[val] > -1) && (ovals[val] < MAX_COIN_TYPE))
                return strdup(coin_name[ovals[val]]);
        }
        break;
    case ITEM_SPICE:
        switch (val) {
        case 1:                /* type */
            if ((ovals[val] > -1) && (ovals[val] < MAX_SKILLS))
                return strdup(skill_name[ovals[val]]);
        case 3:                /* poison type */
            if ((ovals[val] >= 0) && (ovals[val] <= MAX_POISON))
                return strdup(poison_types[ovals[val]]);
        }
        break;
    case ITEM_LIGHT:
        switch (val) {
        case 2:                /* light_type */
            sprint_attrib(ovals[val], light_type, buf);
            return strdup(buf);
        case 3:
            sprintf(buf, "%d", ovals[val]);
            return strdup(buf);
        case 4:                /* Light color */
            sprint_attrib(ovals[val], light_color, buf);
            return strdup(buf);
        case 5:                /* light flags */
            return show_flags(light_flags, ovals[val]);
        }
        break;
    case ITEM_HERB:
        switch (val) {
        case 0:                /* Herb type */
            for (j = 0; *herb_types[j] != '\n'; j++) {
                if (j == ovals[val])
                    return strdup(herb_types[j]);
            }
            break;
        case 2:                /* herb color */
            for (j = 0; *herb_colors[j] != '\n'; j++) {
                if (j == ovals[val])
                    return strdup(herb_colors[j]);
            }
            break;
        case 3:                /* type of poison */
            if ((ovals[val] >= 0) && (ovals[val] <= MAX_POISON))
                return strdup(poison_types[ovals[val]]);
        }
        break;
    case ITEM_NOTE:
        switch (val) {
        case 2:                /* Language   */
            if ((ovals[val] > -1) && (ovals[val] < MAX_SKILLS))
                return strdup(skill_name[ovals[val]]);
            break;
        case 3:                /* Note Flags */
            return show_flags(note_flags, ovals[val]);
        case 4:                /* SKILL */
	  if ((ovals[val] > -1) && (ovals[val] < MAX_SKILLS))
	    return strdup(skill_name[ovals[val]]);
        }
        break;
    }

    sprintf(buf, "%d", ovals[val]);
    return strdup(buf);
}

void
set_oval(OBJ_DATA * obj, char *arg, int val, char **msg)
{
    int j;
    /*  int counter; */
    CHAR_DATA *v;
    char tmp[256];
    char buf[MAX_STRING_LENGTH];

    *msg = NULL;

    switch (obj->obj_flags.type) {
    case ITEM_LIGHT:
        switch (val) {
        case 0:                /* duration */
            if ((atoi(arg) > 10000) || (atoi(arg) < -1)) {
                *msg = strdup("That light duration is invalid.\n\r");
                return;
            }
            obj->obj_flags.value[0] = atoi(arg);
            return;
        case 1:                /* max_amount */
            if ((atoi(arg) > 10000) || (atoi(arg) < -1)) {
                *msg = strdup("That max amount is invalid.\n\r");
                return;
            }
            obj->obj_flags.value[1] = atoi(arg);
            return;
        case 2:                /* light_type */
            j = get_attrib_num(light_type, arg);
            if (j == -1) {
                *msg = strdup("That is not a valid light type.\n\r" "Light type is one of:\n\r");
                return;
            }
            obj->obj_flags.value[2] = light_type[j].val;
            return;
        case 3:                /* range */
            if ((atoi(arg) > 3 || (atoi(arg) < 0))) {
                *msg = strdup("That range is invalid (must be 1-3).\n\r");
                return;
            }
            obj->obj_flags.value[3] = atoi(arg);
            break;
        case 4:                /* light color */
            j = get_attrib_num(light_color, arg);
            if (j == -1) {
                *msg = strdup("That is not a valid light color.\n\r" "Color is one of:\n\r");
                return;
            }
            obj->obj_flags.value[4] = light_color[j].val;
            return;
        case 5:                /* light flags */
            j = get_flag_num(light_flags, arg);
            if (j != -1) {
                if (IS_SET(obj->obj_flags.value[5], light_flags[j].bit)) {
                    REMOVE_BIT(obj->obj_flags.value[5], light_flags[j].bit);
                    *msg = strdup("Flag removed.\n\r");
                } else {
                    MUD_SET_BIT(obj->obj_flags.value[5], light_flags[j].bit);
                    *msg = strdup("Flag added.\n\r");
                }
            } else
                *msg = strdup("That light flag is invalid.\n\r");
            return;
        }
    case ITEM_COMPONENT:
        switch (val) {
        case 0:                /* sphere of effect */
            j = old_search_block(arg, 0, strlen(arg), Sphere, 0);
            if (j != -1)
                obj->obj_flags.value[0] = j - 1;
            else
                *msg = strdup("That Sphere of magick is invalid.\n\r");
            return;
        case 1:                /* relative power of the component */
            j = old_search_block(arg, 0, strlen(arg), Power, 0);
            if (j != -1)
                obj->obj_flags.value[1] = j - 1;
            else
                *msg = strdup("That Power is invalid.\n\r");
            return;
        }
        break;
    case ITEM_WAND:
    case ITEM_STAFF:
        switch (val) {
        case 0:                /* max-mana */
            j = atoi(arg);
            if (j < 0)
                *msg = strdup("You cannot put a negative max-mana.\n\r");
            else
                obj->obj_flags.value[0] = j;
            return;
        case 1:                /* power-level */
            j = (old_search_block(arg, 0, strlen(arg), Power, 0));
            if (j == -1)
                *msg = strdup("That power value is invalid.\n\r");
            else
                obj->obj_flags.value[1] = j;
            return;
        case 2:                /* mana */
            j = atoi(arg);
            if (j < 0)
                *msg = strdup("Cannot have negative mana\n\r");
            else
                obj->obj_flags.value[2] = j;
            return;
        case 3:                /* spell */
            j = (old_search_block(arg, 0, strlen(arg), skill_name, 0));
            if (j == -1) {
                *msg = strdup("There is no spell of that name.\n\r");
                return;
            }
            j--;
            if (!IS_SPELL(j)) {
                *msg = strdup("That's not a spell.\n\r");
                return;
            }
            obj->obj_flags.value[3] = j;
            return;
        }
        break;
    case ITEM_WEAPON:
        switch (val) {
        case 0:                /* plus */
            j = atoi(arg);
            if ((j >= -20) && (j <= 20))
                obj->obj_flags.value[0] = j;
            else
                *msg = strdup("That weapon damage is invalid (range -20 to 20).\n\r");
            return;
        case 1:                /* number of dice */
            j = atoi(arg);
            if ((j >= 1) && (j <= 99))
                obj->obj_flags.value[1] = j;
            else
                *msg = strdup("That number of dice is invalid (range 1 to 99).\n\r");
            return;
        case 2:                /* size of dice */
            j = atoi(arg);
            if ((j >= 1) && (j <= 99))
                obj->obj_flags.value[2] = j;
            else
                *msg = strdup("That size of dice is invalid (range 1 to 99).\n\r");
            return;
        case 3:                /* weapon type */
            j = get_attrib_num(weapon_type, arg);
            if (j == -1) {
                *msg = strdup("That is not a valid weapon type.\n\r");
                return;
            }
            obj->obj_flags.value[3] = weapon_type[j].val - 300;
            return;
        case 5:                /* Poison type */
            j = old_search_block(arg, 0, strlen(arg), poison_types, 0);
            if (j == -1)
                *msg = strdup("That is not a valid poison type.\n\r");
            else
                obj->obj_flags.value[5] = j - 1;
            break;
        }
        break;
    case ITEM_TOOL:
        switch (val) {
        case 0:                /* type of tool */
            j = old_search_block(arg, 0, strlen(arg), tool_name, 0);
            if (j != -1) {
                obj->obj_flags.value[0] = j - 1;
            } else
                *msg = strdup("That tooltype is invalid.\n\r");
            return;
        case 1:                /* quality of tool */
            j = atoi(arg);
            if ((j >= 0) && (j <= 10)) {
                obj->obj_flags.value[1] = j;
            } else
                *msg = strdup("That tool quality is invalid (range: 1 to 10).\n\r");
            return;
        }
        break;
    case ITEM_TREASURE:
        switch (val) {
        case 0:                /* ttype - treasure type */
            j = get_attrib_num(treasure_type, arg);
            if (j == -1) {
                *msg = strdup("That is not a valid treasure type, dammit.\n\r");
                return;
            }
            obj->obj_flags.value[0] = treasure_type[j].val;
            return;
        }
        break;
    case ITEM_BOW:
        switch (val) {
        case 0:                /* range of the bow */
            j = atoi(arg);
            if ((j > 0) && (j < 4))
                obj->obj_flags.value[0] = j;
            else
                *msg = strdup("That range is not valid (range 1 to 3).\n\r");
            return;
        case 1:                /* strength to pull the bow */
            j = atoi(arg);
            if ((j > 0) && (j < 25)) {
                obj->obj_flags.value[1] = j;
            } else
                *msg = strdup("That pull value is invalid (range 1 to 25).\n\r");
            return;
        case 2:
            obj->obj_flags.value[2] = strcmp(arg, "true") ? 0 : 1;
            return;
        }
        break;
    case ITEM_ARMOR:
        switch (val) {
        case 3:                /* max_points */
            j = atoi(arg);
            if ((j >= 0) && (j <= 100)) {
                obj->obj_flags.value[0] = j;
                obj->obj_flags.value[1] = j;
                obj->obj_flags.value[3] = j;
                sprintf(tmp,
                        "%s's (%d) max_points, points and arm_damage have been set " "to %d.\n\r",
                        OSTR(obj, short_descr), obj_index[obj->nr].vnum, j);
                *msg = strdup(tmp);
            } else
                *msg = strdup("That max_point value is invalid (range 0 to 100).\n\r");
            return;

        case 1:                /* points */
            j = atoi(arg);
            if ((j >= 0) && (j <= 100)) {
                /* watch for going over max_points */
                if (j > obj->obj_flags.value[3]) {
                    obj->obj_flags.value[3] = j;
                }
                obj->obj_flags.value[0] = j;
                obj->obj_flags.value[1] = j;
                sprintf(tmp, "%s's (%d) points and arm_damage have been set " "to %d.\n\r",
                        OSTR(obj, short_descr), obj_index[obj->nr].vnum, j);
                *msg = strdup(tmp);
            } else
                *msg = strdup("That point value is invalid (range 0 to 100).\n\r");
            return;
        case 0:                /* damage to armor */
            j = atoi(arg);
            if( j <= obj->obj_flags.value[1]) {
                obj->obj_flags.value[0] = j;
                sprintf(tmp, "%s's (%d) arm_damage has been set to %d.\n\r", OSTR(obj, short_descr),
                    obj_index[obj->nr].vnum, j);
                *msg = strdup(tmp);
                return;
            } else {
                sprintf(tmp, 
                 "%s's (%d) arm_damage must be between 0 and %d.\n\r",
                 OSTR(obj, short_descr),
                 obj_index[obj->nr].vnum, obj->obj_flags.value[1]);
                *msg = strdup(tmp);
            }
        case 2:                /* armor size */
            if (isdigit(*arg)) {        /* if arg is a number */
                j = atoi(arg);
                if ((j >= 1) && (j <= 100)) {
                    obj->obj_flags.value[2] = j;
                    sprintf(tmp, "%s's (%d) size has been set to %d.\n\r", OSTR(obj, short_descr),
                            obj_index[obj->nr].vnum, j);
                    *msg = strdup(tmp);
                } else
                    *msg = strdup("That size is invalid (range 1 to 100).\n\r");
            } else
                /* arg is a string */
            if ((v = get_char(arg))) {
                obj->obj_flags.value[2] = get_char_size(v);
                sprintf(tmp, "%s's (%d) size has been set to %s's size (%d)." "\n\r",
                        OSTR(obj, short_descr), obj_index[obj->nr].vnum, MSTR(v, short_descr),
                        get_char_size(v));
                *msg = strdup(tmp);

            } else
                *msg = strdup("That character does not exist.\n\r");
            return;
        }
        break;

    case ITEM_WORN:
        switch (val) {
        case 0:                /* clothing size */
            if (isdigit(*arg)) {        /* if arg is a number */
                j = atoi(arg);
                if ((j >= 1) && (j <= 100))
                    obj->obj_flags.value[0] = j;
                else
                    *msg = strdup("That size is invalid (range 1 to 100).\n\r");
            } else
                /* arg is a string */
            if ((v = get_char(arg)))
                obj->obj_flags.value[0] = get_char_size(v);
            else
                *msg = strdup("That character does not exist.\n\r");
            return;
        case 1:                /* flags */
            j = get_flag_num(worn_flags, arg);
            if (j != -1) {
                if (IS_SET(obj->obj_flags.value[1], worn_flags[j].bit)) {
                    REMOVE_BIT(obj->obj_flags.value[1], worn_flags[j].bit);
                    *msg = strdup("Flag removed.\n\r");
                } else {
                    MUD_SET_BIT(obj->obj_flags.value[1], worn_flags[j].bit);
                    *msg = strdup("Flag added.\n\r");
                }
            } else
                *msg = strdup("That worn flag is invalid.\n\r");
            return;
        }
        break;

    case ITEM_FURNITURE:       /* Furniture stuff added by Morg */
        switch (val) {
        case 1:                /* Flags */
            j = get_flag_num(furniture_flags, arg);
            if (j != -1) {
                if (IS_SET(obj->obj_flags.value[1], furniture_flags[j].bit)) {
                    REMOVE_BIT(obj->obj_flags.value[1], furniture_flags[j].bit);
                    *msg = strdup("Flag removed.\n\r");
                } else {
                    MUD_SET_BIT(obj->obj_flags.value[1], furniture_flags[j].bit);
                    *msg = strdup("Flag added.\n\r");
                }
            } else
                *msg = strdup("That furniture flag is invalid.\n\r");
            return;
        case 0:                /* Capacity */
            if (isdigit(*arg)) {        /* if arg is a number */
                j = atoi(arg);
                obj->obj_flags.value[0] = j;
                *msg = strdup("Capacity set.\n\r");
            } else
                *msg = strdup("Capacity must be a number.\n\r");
            return;
        case 2:                /* Occupancy */
            if (isdigit(*arg)) {        /* if arg is a number */
                j = atoi(arg);
                obj->obj_flags.value[2] = j;
                *msg = strdup("Occupancy set.\n\r");
            } else
                *msg = strdup("Occupancy must be a number.\n\r");
            return;
        }
        break;
    case ITEM_PLAYABLE:
        switch (val) {
        case 0:                /* play_type */
            j = get_attrib_num(playable_type, arg);
            if (j == -1) {
                *msg = strdup("That is not a valid playable type, dammit.\n\r");
                return;
            }
            obj->obj_flags.value[0] = playable_type[j].val;
            return;
        }
        break;
    case ITEM_SCROLL:
        switch (val) {
        case 0:                /* Power */
            j = (old_search_block(arg, 0, strlen(arg), Power, 0));
            if (j == -1)
                *msg = strdup("That power value is invalid.\n\r");
            else
                obj->obj_flags.value[0] = j;
            return;
        case 1:                /* spell1 */
        case 2:                /* spell2 */
        case 3:                /* spell3 */
            j = (old_search_block(arg, 0, strlen(arg), skill_name, 0));
            if (j == -1)
                *msg = strdup("There is no spell of that name.\n\r");
            else {
                j--;
                if (!IS_SPELL(j))
                    *msg = strdup("That's not a spell.\n\r");
                else
                    obj->obj_flags.value[val] = j;
            }
            return;
        case 4:                /* source */
            *msg = strdup("Unimplemented.\n\r");
            return;
        case 5:                /* language */
            for (j = 1; j < MAX_SKILLS; j++)
                if (!strnicmp(skill_name[j], arg, strlen(arg)))
                    break;
            if (j == MAX_SKILLS) {
                *msg = strdup("No such skill of that name.\n\r");
                return;
            } else {
                //      if (skill[j].sk_class != CLASS_LANG)
                //        *msg = strdup ("That is not a language skill.\n\r");
                if (strnicmp("RW ", arg, 3))
                    *msg = strdup("That is not a RW language skill.\n\r");
                else
                    obj->obj_flags.value[val] = j;
            }
            return;
        }
        break;
    case ITEM_POTION:
        switch (val) {
        case 0:                /* level */
            j = (old_search_block(arg, 0, strlen(arg), Power, 0));
            if (j == -1)
                *msg = strdup("That power level value is invalid.\n\r");
            else
                obj->obj_flags.value[0] = j;
            return;
        case 1:                /* spell */
        case 2:
        case 3:
            j = (old_search_block(arg, 0, strlen(arg), skill_name, 0));
            if (j == -1)
                *msg = strdup("There is no spell of that name.\n\r");
            else {
                j--;
                if (!IS_SPELL(j))
                    *msg = strdup("That's not a spell.\n\r");
                else
                    obj->obj_flags.value[val] = j;
            }
            return;
        }
        break;
    case ITEM_CONTAINER:
        switch (val) {
        case 0:                /* capacity */
            j = atoi(arg);
            if ((j < 0) || (j > 10000))
                *msg = strdup("That capacity is invalid (range 0-10000).\n\r");
            else
                obj->obj_flags.value[0] = j;
            return;
        case 1:                /* flags */
            j = get_flag_num(container_flags, arg);
            if (j != -1) {
                if (IS_SET(obj->obj_flags.value[1], container_flags[j].bit)) {
                    REMOVE_BIT(obj->obj_flags.value[1], container_flags[j].bit);
                    *msg = strdup("Flag removed.\n\r");
                } else {
                    MUD_SET_BIT(obj->obj_flags.value[1], container_flags[j].bit);
                    *msg = strdup("Flag added.\n\r");
                }
            } else
                *msg = strdup("That container flag is invalid.\n\r");
            return;
        case 2:                /* key */
            /* I took this out so that the function would work with asl -hal
             * if ((GET_LEVEL(ch) < HIGHLORD) && !has_privs_in_zone(ch,
             * (atoi(arg) / 1000), MODE_STR_EDIT))
             * *msg = strdup("The key must be a virtual object number in your zone");
             * else
             */
            obj->obj_flags.value[2] = atoi(arg);
            return;
        case 4:                /* max_item_weight */
            j = atoi(arg);
            if ((j < 0) || (j > 10000))
                *msg = strdup("That max_item_weight is invalid (range 0 to 10000).\n\r");
            else
                obj->obj_flags.value[4] = j;
            return;
        case 5:                /* size */
            j = atoi(arg);
            if ((j < 0) || (j > 1000))
                *msg = strdup("The size is invalid (range 0 to 1000).\n\r");
            else
                obj->obj_flags.value[5] = j;
            return;
        }
        break;
    case ITEM_NOTE:
        switch (val) {
        case 0:                /* number of pages in the note */
            j = atoi(arg);
            if ((j >= 1) && (j <= 12)) {
                obj->obj_flags.value[0] = j;
            } else
                *msg = strdup("That number of pages is invalid (range 1 to 12).\n\r");
            return;
        case 1:
            *msg = strdup("This value is handled by the code, do not modify.\n\r");
            return;
        case 2:                /* language */
            *msg = strdup("Not implemented yet.\n\r");
            return;
        case 3:                /* note flags */
            j = get_flag_num(note_flags, arg);
            if (j != -1) {
                if (IS_SET(obj->obj_flags.value[3], note_flags[j].bit)) {
                    REMOVE_BIT(obj->obj_flags.value[3], note_flags[j].bit);
                    *msg = strdup("Flag removed.\n\r");
                } else {
                    MUD_SET_BIT(obj->obj_flags.value[3], note_flags[j].bit);
                    *msg = strdup("Flag added.\n\r");
                }
            } else
                *msg = strdup("That note flag is invalid.\n\r");
            break;
        case 4:                /* skill */
	  j = (old_search_block(arg, 0, strlen(arg), skill_name, 0));
	  if (j == -1)
	    *msg = strdup("There is no skill of that name.\n\r");
	  else {
	    j--;
	    obj->obj_flags.value[4] = j;
	  }
	  return;
	  break;
        case 5:                /* MaxPerc */
            j = atoi(arg);
            if ((j < 0) || (j > 100))
                *msg = strdup("That capacity is invalid (range 1 to 100).\n\r");
            else
                obj->obj_flags.value[5] = j;
            return;
        }
        break;
    case ITEM_DRINKCON:
        switch (val) {
        case 0:                /* capacity */
            j = atoi(arg);
            if ((j < 0) || (j > 10000))
                *msg = strdup("That capacity is invalid (range 1 to 10000).\n\r");
            else
                obj->obj_flags.value[0] = j;
            return;
        case 1:                /* Amount */
            j = atoi(arg);
            if ((j < 0) || (j > 10000))
                *msg = strdup("That amount is invalid (range 1 to 10000).\n\r");
            else if (j > obj->obj_flags.value[0])
                *msg =
                    strdup("The amount of liquid can't be more than " "the liquid capacity.\n\r");
            else
                obj->obj_flags.value[1] = j;
            return;
        case 2:
            j = old_search_block(arg, 0, strlen(arg), drinknames, 0);
            if (j == -1)
                *msg = strdup("That is not a valid liquid type.\n\r");
            else
                obj->obj_flags.value[2] = j - 1;
            return;
        case 3:
            j = get_flag_num(liquid_flags, arg);
            if (j != -1) {
                if (IS_SET(obj->obj_flags.value[3], liquid_flags[j].bit)) {
                    REMOVE_BIT(obj->obj_flags.value[3], liquid_flags[j].bit);
                    *msg = strdup("Flag removed.\n\r");
                } else {
                    MUD_SET_BIT(obj->obj_flags.value[3], liquid_flags[j].bit);
                    *msg = strdup("Flag added.\n\r");
                }
            } else
                *msg = strdup("That drink container flag is invalid.\n\r");
            return;
        case 4:                /* spice num */
            j = (old_search_block(arg, 0, strlen(arg), skill_name, 0));
            if (j == -1)
                *msg = strdup("There is no spice type of that name.\n\r");
            else {
                j--;
                if (!IS_SPICE(j))
                    *msg = strdup("That's not a spice.\n\r");
                else
                    obj->obj_flags.value[4] = j;
            }
            return;
        case 5:                /* poison type */
            j = old_search_block(arg, 0, strlen(arg), poison_types, 0);
            if (j == -1)
                *msg = strdup("That is not a valid poison type.\n\r");
            else
                obj->obj_flags.value[5] = j - 1;
            break;
        }
        break;
    case ITEM_FOOD:
        switch (val) {
        case 0:                /* filling */
            j = atoi(arg);
            if ((j > 48) || (j < 1))
                *msg = strdup("That amount is invalid (range 1 to 48).\n\r");
            else
                obj->obj_flags.value[val] = j;
            return;
        case 1:                /* food type */
            j = old_search_block(arg, 0, strlen(arg), food_names, 0);
            if (j == -1)
                *msg = strdup("That is not a valid food type.\n\r");
            else
                obj->obj_flags.value[1] = j - 1;
            return;
        case 2:                /* food age */
            j = atoi(arg);
            if ((j > 100) || (j < 0))
                *msg = strdup("Age must be between 0-100.\n\r");
            else
                obj->obj_flags.value[2] = j;
            return;
        case 3:                /* Poison type */
            j = old_search_block(arg, 0, strlen(arg), poison_types, 0);
            if (j == -1)
                *msg = strdup("That is not a valid poison type.\n\r");
            else
                obj->obj_flags.value[3] = j - 1;
            break;
        case 5:                /* spice num */
            j = (old_search_block(arg, 0, strlen(arg), skill_name, 0));
            if (j == -1)
                *msg = strdup("There is no spice type of that name.\n\r");
            else {
                j--;
                if (!IS_SPICE(j))
                    *msg = strdup("That's not a spice.\n\r");
                else
                    obj->obj_flags.value[5] = j;
            }
            return;
        }
        break;
    case ITEM_POISON:
        switch (val) {
        case 0:                /* type */
            j = old_search_block(arg, 0, strlen(arg), poison_types, 0);
            if (j == -1)
                *msg = strdup("That is not a valid poison type.\n\r");
            else
                obj->obj_flags.value[0] = j - 1;
            break;
        case 1:                /* amount */
            if ((atoi(arg) < 0) || (atoi(arg) > 100))
                *msg = strdup("That amount is invalid (range 0 to 100).\n\r");
            else
                obj->obj_flags.value[1] = atoi(arg);
            break;
        }
        break;
    case ITEM_HERB:
        switch (val) {
        case 0:                /* type */
            j = old_search_block(arg, 0, strlen(arg), herb_types, 0);
            if (j == -1)
                *msg = strdup("That is not a valid herb type.\n\r");
            else
                obj->obj_flags.value[0] = j - 1;
            break;
        case 1:                /* Edibility */
            if ((atoi(arg) < 0) || (atoi(arg) > 1))
                *msg = strdup("That amount is invalid (range 0 to 1).\n\r");
            else
                obj->obj_flags.value[1] = atoi(arg);
            break;
        case 2:                /* color */
            j = old_search_block(arg, 0, strlen(arg), herb_colors, 0);
            if (j == -1)
                *msg = strdup("That is not a valid herb color.\n\r");
            else
                obj->obj_flags.value[2] = j - 1;
            break;
        case 3:                /* poison */
            j = old_search_block(arg, 0, strlen(arg), poison_types, 0);
            if (j == -1)
                *msg = strdup("That is not a valid poison.\n\r");
            else
                obj->obj_flags.value[3] = j - 1;
            break;
        }
        break;
    case ITEM_CURE:
        switch (val) {
        case 0:                /* type */
            j = old_search_block(arg, 0, strlen(arg), poison_types, 0);
            if (j == -1)
                *msg = strdup("That is not a valid poison cure.\n\r");
            else
                obj->obj_flags.value[0] = j - 1;
            break;
        case 1:                /* form */
            j = old_search_block(arg, 0, strlen(arg), cure_forms, 0);
            if (j == -1)
                *msg = strdup("That is not a valid cure form.\n\r");
            else
                obj->obj_flags.value[1] = j - 1;
            break;
        case 2:                /* poison */
            j = old_search_block(arg, 0, strlen(arg), poison_types, 0);
            if (j == -1)
                *msg = strdup("That is not a valid poison side-effect.\n\r");
            else
                obj->obj_flags.value[2] = j - 1;
            break;
        case 3:                /* taste */
            j = old_search_block(arg, 0, strlen(arg), cure_tastes, 0);
            if (j == -1)
                *msg = strdup("That is not a valid cure taste.\n\r");
            else
                obj->obj_flags.value[3] = j - 1;
            break;
        case 4:                /* Strength */
            if ((atoi(arg) < 0) || (atoi(arg) > 100))
                *msg = strdup("That amount is invalid (range is 0 to 100.\n\r");
            else
                obj->obj_flags.value[4] = atoi(arg);
            break;
        }
        break;
    case ITEM_MONEY:
        switch (val) {
        case 0:                /* amount */
            if ((atoi(arg) < 0) || (atoi(arg) > 1000000))
                *msg = strdup("That amount is invalid (range 1 to 1000000).\n\r");
            else
                obj->obj_flags.value[0] = atoi(arg);
            return;
        case 1:                /* type */
            j = old_search_block(arg, 0, strlen(arg), coin_name, 0);
            if (j != -1) {
                obj->obj_flags.value[1] = j - 1;
            } else
                *msg = strdup("That type of coin invalid.\n\r");
            return;
        }
        break;
    case ITEM_WAGON:
        switch (val) {
        case 0:                /* entrance */
            j = atoi(arg);
            if (!get_room_num(j))
                *msg = strdup("That room doesn't exist.\n\r");
            else
                obj->obj_flags.value[0] = j;
            return;
        case 1:                /* start room */
            j = atoi(arg);
            if (!get_room_num(j))
                *msg = strdup("That room doesn't exist.\n\r");
            else
                obj->obj_flags.value[1] = j;
            return;
        case 2:                /* front */
            j = atoi(arg);
            if (!get_room_num(j))
                *msg = strdup("That room doesn't exist.\n\r");
            else
                obj->obj_flags.value[2] = j;
            return;
        case 3:                /* wagon type */
            /*
             * j = get_attrib_num (wagon_type, arg);
             * if (j != -1)
             * obj->obj_flags.value[3] = wagon_type[j].val;
             * else
             * *msg = strdup ("That wagon type is invalid.\n\r");
             * return;
             * break;
             */
            j = get_flag_num(wagon_flags, arg);
            if (j != -1) {
                if (IS_SET(obj->obj_flags.value[3], wagon_flags[j].bit)) {
                    REMOVE_BIT(obj->obj_flags.value[3], wagon_flags[j].bit);
                    *msg = strdup("Flag removed.\n\r");
                } else {
                    MUD_SET_BIT(obj->obj_flags.value[3], wagon_flags[j].bit);
                    *msg = strdup("Flag added.\n\r");
                }
            } else
                *msg = strdup("That wagon flag is invalid.\n\r");
            return;
        case 4:                /* damage */
            j = atoi(arg);
            if ((j < 0) || (j > (calc_wagon_max_damage(obj)))) {
                sprintf(buf, "Damage must be between 0 and %d.\n\r", calc_wagon_max_damage(obj));
                *msg = strdup(buf);
/*
            *msg = strdup ("Damage must be between 0 and 40.\n\r");
*/
            } else
                obj->obj_flags.value[4] = j;
            return;
        case 5:                /* max_size */
            if ((atoi(arg) < 0) || (atoi(arg) > 0100))
                *msg = strdup("That amount is invalid (range 0 to 1000).\n\r");
            else
                obj->obj_flags.value[5] = atoi(arg);
            return;

        }
        break;
    case ITEM_SPICE:
        switch (val) {
        case 0:                /* level */
            j = atoi(arg);
            if (j <= 0)
                *msg = strdup("The level must be greater than 0.\n\r");
            else
                obj->obj_flags.value[0] = atoi(arg);
            return;
        case 1:                /* type */
            j = (old_search_block(arg, 0, strlen(arg), skill_name, 0));
            if (j == -1)
                *msg = strdup("There is no spell or spice type of that name.\n\r");
            else {
                j--;
                if (!IS_SPELL(j) && !IS_SPICE(j))
                    *msg = strdup("That's not a spell or a spice.\n\r");
                else
                    obj->obj_flags.value[1] = j;
            }
            return;
        case 2:                /* smoke */
            *msg = strdup("Not setable, the code takes care of it.\n\r");
            break;
        case 3:                /* poison */
            j = old_search_block(arg, 0, strlen(arg), poison_types, 0);
            if (j == -1)
                *msg = strdup("That is not a valid poison side-effect.\n\r");
            else
                obj->obj_flags.value[3] = j - 1;
            break;
        }
        break;
    case ITEM_MINERAL:
        switch (val) {
        case 0:                /* mineral type */
            j = get_flag_num(mineral_type, arg);
            if (j == -1) {
                *msg = strdup("That is not a valid mineral type, dammit.\n\r");
                return;
            }
            obj->obj_flags.value[0] = mineral_type[j].bit;
            return;
        }
        break;
    case ITEM_MUSICAL:
        switch (val) {
        case 0:                /* quality of musical item */
            j = atoi(arg);
            if ((j >= 0) && (j <= 10)) {
                obj->obj_flags.value[0] = j;
            } else
                *msg = strdup("That musical quality is invalid (range: 1 to 10).\n\r");
            return;

        case 1:                /* instrument type */
            j = get_attrib_num(instrument_type, arg);
            if (j == -1) {
                *msg = strdup("That is not a valid type.\n\r");
                return;
            }
            obj->obj_flags.value[1] = instrument_type[j].val;

            sprintf(tmp, "You have set %s (#%d) to instrument_type %s.\n\r", OSTR(obj, short_descr),
                    obj_index[obj->nr].vnum, instrument_type[j].name);
            *msg = strdup(tmp);

            return;
        case 2:                /* flags */
            j = get_flag_num(musical_flags, arg);
            if (j != -1) {
                if (IS_SET(obj->obj_flags.value[2], musical_flags[j].bit)) {
                    REMOVE_BIT(obj->obj_flags.value[2], musical_flags[j].bit);
                    *msg = strdup("Flag removed.\n\r");
                } else {
                    MUD_SET_BIT(obj->obj_flags.value[2], musical_flags[j].bit);
                    *msg = strdup("Flag added.\n\r");
                }
            } else
                *msg = strdup("That musical flag is invalid.\n\r");
            return;
        }
        break;

    case ITEM_JEWELRY:
        switch (val) {
        case 0:                /* quality of jewelry */
            j = atoi(arg);
            if ((j >= 0) && (j <= 10)) {
                obj->obj_flags.value[0] = j;
            } else
                *msg = strdup("That jewelry quality is invalid (range: 1 to 10).\n\r");
            return;
        case 1:                /* flags */
            j = get_flag_num(jewelry_flags, arg);
            if (j != -1) {
                if (IS_SET(obj->obj_flags.value[1], jewelry_flags[j].bit)) {
                    REMOVE_BIT(obj->obj_flags.value[1], jewelry_flags[j].bit);
                    *msg = strdup("Flag removed.\n\r");
                } else {
                    MUD_SET_BIT(obj->obj_flags.value[1], jewelry_flags[j].bit);
                    *msg = strdup("Flag added.\n\r");
                }
            } else
                *msg = strdup("That jewelry flag is invalid.\n\r");
            return;
        }
        break;

    case ITEM_RUMOR_BOARD:
        switch (val) {
        case 0:                /* board# */
            if (is_number(arg)) {
                j = atoi(arg);
                if (j >= 0 && j < MAX_BOARDS)
                    obj->obj_flags.value[0] = j;
                else {
                    *msg = strdup("Board# must be a number between 0 and 500.\n\r");
                }
            } else {
                *msg = strdup("Board# must be a number.\n\r");
            }
        }
        break;


    case ITEM_SCENT:
        switch (val) {
        case 0:                /* smell */
            *msg = strdup("That's not a valid smell, type show s for a list.\n\r");
            return;
        case 1:                /* current */
            if ((j = atoi(arg)) < 0 || j > obj->obj_flags.value[2]) {
                *msg =
                    strdup("The current amount must be less than"
                           " the max and greater than 0.\n\r");
                return;
            }
            obj->obj_flags.value[1] = j;
            return;
        case 2:                /* max */
            if ((j = atoi(arg)) < 0) {
                *msg = strdup("The max amount must be greater than 0\n\r");
                return;
            }

            obj->obj_flags.value[2] = j;

            /* make sure current <= max */
            if (obj->obj_flags.value[1] > obj->obj_flags.value[2])
                obj->obj_flags.value[1] = obj->obj_flags.value[2];
            return;

        }
        break;
    }
}

/* the big one...here comes OSET */

int
is_spice_f(int i)
{
    return (IS_SPICE(i));
}

int
is_spell_f(int i)
{
    return (IS_SPELL(i));
}

void
cmd_oset(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{                               /* Cmd_Oset */
    char arg1[256], arg2[256], arg3[256];
    char buf[256];
    /*  char logmsg[256]; */
    int i, j, new_int, old_int, inter = 0, tmpint;
    float floater, floater2;
    bool had_pulse = FALSE;

    OBJ_DATA *obj;
    struct extra_descr_data *tmpexd;
    CHAR_DATA *orig = SWITCHED_PLAYER(ch);

    static const char * const generic_field[] = {
        "name",
        "sdesc",
        "ldesc",
        "desc",
        "edesc",
        "type",
        "eflag",
        "wflag",
        "aff1",
        "aff2",
        "weight",
        "cost",
        "material",
        "limit",
        "program",
        "val1",
        "val2",
        "val3",
        "val4",
        "val5",
        "val6",
        "damage",
        "direction",
        "sflag",
        "color",
        "adverbial",
        "\n"
    };

    static char *generic_desc[] = {
        "(\"sword long spiked\")",
        "(\"a spiked long sword\")",
        "(\"You see a spiked long sword here.\")",
        "(object description)",
        "(object extra description)",
        "(object type)",
        "(extra flag)",
        "(wear flag)",
        "(first affect)",
        "(second affect)",
        "(object weight)",
        "(object cost)",
        "(object material)",
        "(object limit)",
        "(program)",
        "(object value 1 [Ovl only])",
        "(object value 2 [Ovl only])",
        "(object value 3 [Ovl only])",
        "(object value 4 [Ovl only])",
        "(object value 5 [Ovl only])",
        "(object value 6 [Ovl only])",
        "(damage, in form XdY+Z)",
        "(direction, if a wagon object (NOT IN USE)",
        "(state flag)",
        "(color flag)",
        "(adverbial flag)",
        "\n"
    };


    char *msg, *c;

    argument = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));
    strcpy(arg3, argument);

    obj = get_obj_vis(ch, arg1);
    if (!obj) {
        send_to_char("That object does not exist.\n\r", ch);
        return;
    }

    if (!has_privs_in_zone(ch, obj_index[obj->nr].vnum / 1000, MODE_STR_EDIT)) {
        send_to_char("You do not have creation privs in that zone.\n\r", ch);
        return;
    }

    if (!*arg2) {
        for (i = 0; *generic_field[i] != '\n'; i++) {
            sprintf(buf, "%-15s%s\n\r", generic_field[i], generic_desc[i]);
            send_to_char(buf, ch);
        }
        for (i = 0; i < MAX_OVALS; i++)
            if (strcmp(oset_field[(int) obj->obj_flags.type].description[i], "")) {
                sprintf(buf, "%-15s%s\n\r", oset_field[(int) obj->obj_flags.type].set[i],
                        oset_field[(int) obj->obj_flags.type].description[i]);
                send_to_char(buf, ch);
            }
        return;
    }

    i = old_search_block(arg2, 0, strlen(arg2), generic_field, 0) - 1;
    switch (i) {
    case 0:                    /* name */
        if (!*arg3) {
            send_to_char("Syntax: oset <object> name <namestring>.\n\r", ch);
            send_to_char("An object can have several names, such as 'hammer "
                         "stone carved' would let people use it with any of "
                         "those names.\n\r The first name in the list should "
                         "always be the most commonly used, in this case " "'hammer'.\n\r", ch);
            return;
        }

        for (tmpexd = obj->ex_description; tmpexd; tmpexd = tmpexd->next) {
            c = tmpexd->keyword;

            while (isspace(*c))
                c++;

            if (!stricmp(c, OSTR(obj, name)))
                break;
        }

        if (tmpexd) {
            free(tmpexd->keyword);
            tmpexd->keyword = strdup(arg3);
        }

        cprintf(ch, "%s's (#%d) name has been changed from '%s' to '%s'.\n\r",
                OSTR(obj, short_descr), obj_index[obj->nr].vnum, OSTR(obj, name), arg3);

        free((char *) obj->name);
        obj->name = strdup(arg3);

        send_to_char("Done.\n\r", ch);

        return;
    case 1:                    /* sdesc */
        if (!*arg3) {
            send_to_char("Syntax: oset <object> sdesc <sdescstring>.\n\r", ch);
            send_to_char("Example: The carved, stone hammer\n\r", ch);
            return;
        }


        cprintf(ch, "%s's (#%d) sdesc has been changed from '%s' to '%s'.\n\r",
                OSTR(obj, short_descr), obj_index[obj->nr].vnum, OSTR(obj, short_descr), arg3);

        free((char *) obj->short_descr);
        obj->short_descr = strdup(arg3);
        send_to_char("Done.\n\r", ch);
        return;
    case 2:                    /* ldesc */
        if (!*arg3) {
            send_to_char("Syntax: oset <object> ldesc <ldescstring>.\n\r", ch);
            send_to_char("Example: The carved, stone hammer lies here.\n\r", ch);
            return;
        }

        cprintf(ch, "%s's (#%d) ldesc has been changed from\n\r" "'%s'\n\r" "to\n\r" "'%s'\n\r",
                OSTR(obj, short_descr), obj_index[obj->nr].vnum, OSTR(obj, long_descr), arg3);

        free((char *) obj->long_descr);
        obj->long_descr = strdup(arg3);

        send_to_char("Done.\n\r", ch);
        return;
    case 3:                    /* desc */
        set_odesc(ch, obj);
        return;
    case 4:                    /* edesc */
        set_oedesc(ch, obj, arg3);
        return;
    case 5:                    /* type */
        set_otype(ch, obj, arg3);
        return;
    case 6:                    /* eflag */
        set_oeflag(ch, obj, arg3);
        return;
    case 7:                    /* wflag */
        set_owflag(ch, obj, arg3);
        return;
    case 8:                    /* aff1 */
        set_oaffect(ch, obj, arg3, 0);
        return;
    case 9:                    /* aff2 */
        set_oaffect(ch, obj, arg3, 1);
        return;
    case 10:                   /* weight */
        if (!(*arg3)) {
            send_to_char("Syntax: oset <object> weight <amount>\n\r", ch);
            send_to_char("Valid values for weight are 0 to 100000.\n\r", ch);
            return;
        }

        floater = atof(arg3);
        tmpint = floater * 100;
        //      sprintf(logmsg, "Floater: %.2f, Tmpint: %d", floater, tmpint);
        //      gamelog(logmsg);


        if ((tmpint != 0) && (tmpint != 10) && (tmpint != 25) && (tmpint != 50) && (tmpint != 75)
            && (tmpint < 100)) {
            send_to_char("That weight is invalid.\n\r", ch);
            send_to_char("Valid values for weight are 0 to 100000.\n\r", ch);
            send_to_char("Values less than 1 must be .1, .25, .50, or .75.\n\r", ch);
            return;
        }

        /* This is to round down values when people try and set decimal
         * values for numbers 1 and higher */
        if (floater >= 1.0)
            inter = floater;

        floater *= 100;
        inter = floater;
        floater2 = obj->obj_flags.weight;
        floater2 /= 100;

        cprintf(ch, "%s's (#%d) weight has been changed from %.2f to %.2f.\n\r",
                OSTR(obj, short_descr), obj_index[obj->nr].vnum, floater2, floater / 100);

        obj->obj_flags.weight = floater;

        send_to_char("Done.\n\r", ch);
        return;
    case 11:                   /* cost */
        if (!(*arg3)) {
            send_to_char("Syntax: oset <object> cost <amount>\n\r", ch);
            send_to_char("Valid values for cost are 0 to 100000.\n\r", ch);
            return;
        }

        if ((atoi(arg3) < 0) || (atoi(arg3) > 100000)) {
            send_to_char("That cost is invalid.\n\r", ch);
            send_to_char("Valid values for cost are 0 to 100000.\n\r", ch);
            return;
        }
        cprintf(ch, "%s's (#%d) cost has been changed from %d to %d.\n\r", OSTR(obj, short_descr),
                obj_index[obj->nr].vnum, obj->obj_flags.cost, atoi(arg3));

        obj->obj_flags.cost = atoi(arg3);
        send_to_char("Done.\n\r", ch);
        return;
    case 12:                   /* material */
        set_material(ch, obj, arg3);
        return;
    case 13:                   /* limit */
        if (!(*arg3)) {
            send_to_char("Syntax: oset <object> limit <amount>\n\r", ch);
            send_to_char("Valid values for limit are (\"none\" or -1 to 20)." "\n\r", ch);
            return;
        }
        if (!strcmp("none", arg3))
            new_int = -1;
        else {
            if ((atoi(arg3) < -1) || (atoi(arg3) > 20)) {
                send_to_char("That limit is invalid.\n\r", ch);
                send_to_char("Valid values for limit are (\"none\" or -1 " "to 20).\n\r", ch);
                return;
            } else
                new_int = atoi(arg3);
        };


        old_int = obj_index[obj->nr].limit;
        obj_index[obj->nr].limit = new_int;
        cprintf(ch, "%s's (#%d) limit has been changed from %d to %d .\n\r", OSTR(obj, short_descr),
                obj_index[obj->nr].vnum, old_int, new_int);
        return;
    case 14:                   /* program */
        had_pulse = has_special_on_cmd(obj->programs, NULL, CMD_PULSE);
        obj->programs = set_prog(ch, obj->programs, arg3);
        if (!had_pulse && has_special_on_cmd(obj->programs, NULL, CMD_PULSE))
            new_event(EVNT_OBJ_PULSE, PULSE_OBJECT, 0, 0, obj, 0, 0);
        return;
    case 15:                   /* val1 */
        if (GET_LEVEL(orig) < OVERLORD) {
            send_to_char("You don't have permission to set that.\n\r", ch);
            return;
        }

        if (!(*arg3)) {
            send_to_char("Syntax: oset <object> val1 <amount>\n\r", ch);
            return;
        }

        cprintf(ch, "%s's (%d) Val1 was changed from %d to %d.\n\r", OSTR(obj, short_descr),
                obj_index[obj->nr].vnum, obj->obj_flags.value[0], atoi(arg3));

        obj->obj_flags.value[0] = atoi(arg3);
        send_to_char("Done.\n\r", ch);
        return;
    case 16:                   /* val2 */
        if (GET_LEVEL(orig) < OVERLORD) {
            send_to_char("You don't have permission to set that.\n\r", ch);
            return;
        }
        if (!(*arg3)) {
            send_to_char("Syntax: oset <object> val2 <amount>\n\r", ch);
            return;
        }

        cprintf(ch, "%s's (%d) Val2 was changed from %d to %d.\n\r", OSTR(obj, short_descr),
                obj_index[obj->nr].vnum, obj->obj_flags.value[1], atoi(arg3));

        obj->obj_flags.value[1] = atoi(arg3);
        send_to_char("Done.\n\r", ch);
        return;
    case 17:                   /* val3 */
        if (GET_LEVEL(orig) < OVERLORD) {
            send_to_char("You don't have permission to set that.\n\r", ch);
            return;
        }
        if (!(*arg3)) {
            send_to_char("Syntax: oset <object> val3 <amount>\n\r", ch);
            return;
        }

        cprintf(ch, "%s's (%d) Val3 was changed from %d to %d.\n\r", OSTR(obj, short_descr),
                obj_index[obj->nr].vnum, obj->obj_flags.value[2], atoi(arg3));

        obj->obj_flags.value[2] = atoi(arg3);
        send_to_char("Done.\n\r", ch);
        return;
    case 18:                   /* val4 */
        if (GET_LEVEL(orig) < OVERLORD) {
            send_to_char("You don't have permission to set that.\n\r", ch);
            return;
        }
        if (!(*arg3)) {
            send_to_char("Syntax: oset <object> val4 <amount>\n\r", ch);
            return;
        }

        cprintf(ch, "%s's (%d) Val4 was changed from %d to %d.\n\r", OSTR(obj, short_descr),
                obj_index[obj->nr].vnum, obj->obj_flags.value[3], atoi(arg3));

        obj->obj_flags.value[3] = atoi(arg3);
        send_to_char("Done.\n\r", ch);
        return;

    case 19:                   /* val5 */
        if (GET_LEVEL(orig) < OVERLORD) {
            send_to_char("You don't have permission to set that.\n\r", ch);
            return;
        }

        if (!(*arg3)) {
            send_to_char("Syntax: oset <object> val5 <amount>\n\r", ch);
            return;
        }

        cprintf(ch, "%s's (%d) Val5 was changed from %d to %d.\n\r", OSTR(obj, short_descr),
                obj_index[obj->nr].vnum, obj->obj_flags.value[4], atoi(arg3));

        obj->obj_flags.value[4] = atoi(arg3);
        send_to_char("Done.\n\r", ch);
        return;

    case 20:                   /* val6 */
        if (GET_LEVEL(orig) < OVERLORD) {
            send_to_char("You don't have permission to set that.\n\r", ch);
            return;
        }

        if (!(*arg3)) {
            send_to_char("Syntax: oset <object> val6 <amount>\n\r", ch);
            return;
        }

        cprintf(ch, "%s's (%d) Val6 was changed from %d to %d.\n\r", OSTR(obj, short_descr),
                obj_index[obj->nr].vnum, obj->obj_flags.value[5], atoi(arg3));


        obj->obj_flags.value[5] = atoi(arg3);
        send_to_char("Done.\n\r", ch);
        return;
    case 21:
        set_odamage(ch, obj, arg3, 1);
        return;
    case 22:
        if (obj->obj_flags.type != ITEM_WAGON) {
            send_to_char("Object must be a wagon.\n\r", ch);
            return;
        }
        if (!(*arg3)) {
            send_to_char("Syntax: oset <object> direction <dir>\n\r", ch);
            return;
        }
        if (is_number(arg3)) {
            i = atoi(arg3);
            if ((i < 0) || (i > 5)) {
                send_to_char("Direction must be between 0(null) and 5(down)\n\r", ch);
                return;
            }
            obj->obj_flags.temp = i;
            return;
        }
        i = old_search_block(arg3, 0, strlen(arg3), dirs, 0);
        if (i)
            obj->obj_flags.temp = i;
        else
            send_to_char("Direction should be in the form of north/south/east/west" "/up/down.\n\r",
                         ch);
        send_to_char("Done.\n\r", ch);
        return;

    case 23:                   /* sflag */
        set_osflag(ch, obj, arg3);
        return;
    case 24:                   /* color flag */
        set_ocflag(ch, obj, arg3);
        return;
    case 25:                   /* adverbial flag */
        set_oaflag(ch, obj, arg3);
        return;
    }

    for (i = 0; i < 6; i++)
        if (!strnicmp(oset_field[(int) obj->obj_flags.type].set[i], arg2, strlen(arg2)))
            break;
    if (i == 6) {
        send_to_char("That field is invalid.  Type OSET <object> with no"
                     " arguments for more info.\n\r", ch);
        return;
    }

    if (!*arg3) {
        sprintf(buf, "OSET <obj> %s\n\r\n\r", oset_field[(int) obj->obj_flags.type].set[i]);
        send_to_char(buf, ch);
        send_to_char((char *) oset_field[(int) obj->obj_flags.type].extra_desc[i], ch);
        switch (obj->obj_flags.type) {
        case ITEM_LIGHT:
            switch (i) {
            case 2:            /* light types */
                send_attribs_to_char(light_type, ch);
                break;
            case 4:            /* light colors */
                send_attribs_to_char(light_color, ch);
                break;
            case 5:            /* light flags */
                send_flags_to_char(light_flags, ch);
                break;
            }
            break;

        case ITEM_TREASURE:
            switch (i) {
            case 0:
                send_attribs_to_char(treasure_type, ch);
                break;
            };
            break;
        case ITEM_PLAYABLE:
            switch (i) {
            case 0:
                send_attribs_to_char(playable_type, ch);
                break;
            };
            break;
        case ITEM_SPICE:
            switch (i) {
            case 1:
                send_list_to_char_f(ch, skill_name, is_spell_f);
                send_list_to_char_f(ch, skill_name, is_spice_f);
                break;
            case 3:            /* poison type */
                for (j = 0; *poison_types[j] != '\n'; j++) {
                    sprintf(buf, "\t%s\n\r", poison_types[j]);
                    send_to_char(buf, ch);
                }
            }
            break;
        case ITEM_POTION:
            switch (i) {
            case 1:
            case 2:
            case 3:
                send_list_to_char_f(ch, skill_name, is_spell_f);
                break;
            };
            break;

        case ITEM_WAND:
        case ITEM_STAFF:
            switch (i) {
            case 1:
                send_list_to_char_f(ch, Power, 0);
                break;
            case 3:
                send_list_to_char_f(ch, skill_name, is_spell_f);
                break;
            };
            break;

        case ITEM_COMPONENT:
            switch (i) {
            case 0:
                send_list_to_char_f(ch, Sphere, 0);
                break;
            case 1:
                send_list_to_char_f(ch, Power, 0);
                break;
            };
            break;
        case ITEM_WEAPON:
            switch (i) {
            case 3:            /* weapon type */
                send_attribs_to_char(weapon_type, ch);
                break;
            case 5:            /* poison type */
                for (j = 0; *poison_types[j] != '\n'; j++) {
                    sprintf(buf, "\t%s\n\r", poison_types[j]);
                    send_to_char(buf, ch);
                }
            }
            break;
        case ITEM_TOOL:
            switch (i) {
            case 0:
                send_list_to_char_f(ch, tool_name, 0);
                break;
            }
            break;
        case ITEM_WORN:
            switch (i) {
            case 1:
                send_flags_to_char(worn_flags, ch);
                break;
            }
            break;
        case ITEM_MUSICAL:
            switch (i) {
            case 2:
                send_flags_to_char(musical_flags, ch);
                break;
            }
            break;
        case ITEM_JEWELRY:
            switch (i) {
            case 1:
                send_flags_to_char(jewelry_flags, ch);
                break;
            }
            break;
        case ITEM_FURNITURE:
            switch (i) {
            case 1:
                send_flags_to_char(furniture_flags, ch);
                break;
            }
            break;

        case ITEM_MONEY:
            switch (i) {
            case 1:
                send_list_to_char_f(ch, coin_name, 0);
                break;
            }
            break;

        case ITEM_CONTAINER:
            switch (i) {
            case 1:
                send_flags_to_char(container_flags, ch);
                break;
            }
            break;
        case ITEM_HERB:
            switch (i) {
            case 0:            /* herb type */
                for (j = 0; *herb_types[j] != '\n'; j++) {
                    sprintf(buf, "\t%s\n\r", herb_types[j]);
                    send_to_char(buf, ch);
                }
                break;
            case 2:            /* color */
                for (j = 0; *herb_colors[j] != '\n'; j++) {
                    sprintf(buf, "\t%s\n\r", herb_colors[j]);
                    send_to_char(buf, ch);
                }
                break;
            case 3:            /* poison */
                for (j = 0; *poison_types[j] != '\n'; j++) {
                    sprintf(buf, "\t%s\n\r", poison_types[j]);
                    send_to_char(buf, ch);
                }
            }
            break;
        case ITEM_POISON:
            switch (i) {
            case 0:            /* poison type */
                for (j = 0; *poison_types[j] != '\n'; j++) {
                    sprintf(buf, "\t%s\n\r", poison_types[j]);
                    send_to_char(buf, ch);
                }
            }
            break;
        case ITEM_CURE:
            switch (i) {
            case 0:            /* poison to cure */
                for (j = 0; *poison_types[j] != '\n'; j++) {
                    sprintf(buf, "\t%s\n\r", poison_types[j]);
                    send_to_char(buf, ch);
                }
                break;
            case 2:            /* poison to set */
                for (j = 0; *poison_types[j] != '\n'; j++) {
                    sprintf(buf, "\t%s\n\r", poison_types[j]);
                    send_to_char(buf, ch);
                }
                break;
            case 3:            /* taste */
                for (j = 0; *cure_tastes[j] != '\n'; j++) {
                    sprintf(buf, "\t%s\n\r", cure_tastes[j]);
                    send_to_char(buf, ch);
                }
            }
            break;
        case ITEM_DRINKCON:
            switch (i) {
            case 2:
                sprintf(buf, " %-20s  : %-7s %-7s %-7s %-6s %-6s\n\r", "name", "thirst", "hunger",
                        "drunk", "burn", "race");
                send_to_char(buf, ch);

                for (j = 0; *drinknames[j] != '\n'; j++) {

                    sprintf(buf, " %-20s  : %3d/%-3d %3d/%-3d %3d/%-3d %3d %-6s\n\r", drinknames[j],    /* drink name */
                            drink_aff[j][2], drink_aff[j][7],   /* thirst */
                            drink_aff[j][1], drink_aff[j][6],   /* hunger */
                            drink_aff[j][0], drink_aff[j][5],   /* drunk  */
                            drink_aff[j][3],    /* burn   */
                            race[drink_aff[j][4]].name);        /* race   */

                    send_to_char(buf, ch);
                }
                break;
            case 3:
                send_flags_to_char(liquid_flags, ch);
                break;
            case 4:
                send_list_to_char_f(ch, skill_name, is_spice_f);
                break;
            case 5:            /* poison type */
                for (j = 0; *poison_types[j] != '\n'; j++) {
                    sprintf(buf, "\t%s\n\r", poison_types[j]);
                    send_to_char(buf, ch);
                }
            }
            break;
        case ITEM_FOOD:
            switch (i) {
            case 1:
                sprintf(buf, " %-20s  : %-6s %-6s %-6s\n\r", "name", "thirst", "hunger", "serving");
                send_to_char(buf, ch);
                send_to_char("---------------------------------------------\n\r", ch);
                for (j = 0; *food_names[j] != '\n'; j++) {
                    sprintf(buf, " %-20s  : %6d %6d %6d\n\r", food_names[j], food_aff[j][2],
                            food_aff[j][1], food_aff[j][0]);

                    send_to_char(buf, ch);
                }
                break;
            case 3:            /* poison type */
                for (j = 0; *poison_types[j] != '\n'; j++) {
                    sprintf(buf, "\t%s\n\r", poison_types[j]);
                    send_to_char(buf, ch);
                }

                //            case 3:
                //              send_flags_to_char (food_flags, ch);
                //              break;
                break;
            case 5:
                send_list_to_char_f(ch, skill_name, is_spice_f);
                break;
            }
        case ITEM_WAGON:
            switch (i) {
            case 1:
                /* send_attribs_to_char (wagon_type, ch); */
                gamelog("Sending flags to char");
                send_flags_to_char(wagon_flags, ch);
                break;
            }
            break;
        }
        return;
    }

    set_oval(obj, arg3, i, &msg);

    if (msg) {
        send_to_char(msg, ch);
        free(msg);              /* free that which is allocated -Morg 4/2/98 */
    } else
        send_to_char("Done.\n\r", ch);

    return;
}

void
ounset_prog(CHAR_DATA *ch, OBJ_DATA * obj, const char *args)
{
    obj->programs = unset_prog(ch, obj->programs, args);
}

/* unset whatever oset did */
void
cmd_ounset(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char arg1[MAX_INPUT_LENGTH], arg2[MAX_INPUT_LENGTH];
    char tmp1[MAX_INPUT_LENGTH], tmp2[MAX_INPUT_LENGTH];
    int loc, i, j, spaces;
    OBJ_DATA *obj;

    const char * const field[] = {
        "edesc",
        "program",
        "\n"
    };

    char *desc[] = {
        "(remove an extra description)",
        "(remove a program)",
        "\n"
    };

    /* bleh it didn't like how I tried an array of function pointers */
    struct func_data
    {
        void (*func) (CHAR_DATA * character, OBJ_DATA * obj, const char *argument);
    };

    struct func_data func[] = {
        {unset_oedesc},
        {ounset_prog},
        {0}
    };

    argument = two_arguments(argument, arg1, sizeof(arg1), arg2, sizeof(arg2));

    if (!(obj = get_obj_vis(ch, arg1))) {
        send_to_char("No such object.\n\r", ch);
        return;
    }

    if (!*arg2) {
        for (i = 0; *field[i] != '\n'; i++) {
            spaces = 15 - strlen(field[i]);
            for (j = 0; j < spaces; j++)
                tmp2[j] = ' ';
            tmp2[j] = '\0';
            sprintf(tmp1, "%s%s%s\n\r", field[i], tmp2, desc[i]);
            send_to_char(tmp1, ch);
        }
        return;
    }

    loc = (old_search_block(arg2, 0, strlen(arg2), field, 0));

    if (loc == -1) {
        send_to_char("That field does not exist.\n\r", ch);
        return;
    }
    if (!has_privs_in_zone(ch, obj_index[obj->nr].vnum / 1000, MODE_STR_EDIT)) {
        send_to_char("You do not have creation privs in that zone.\n\r", ch);
        return;
    }

    (*(func[loc - 1].func)) (ch, obj, argument);

    send_to_char("Done.\n\r", ch);
}


/* update an object to the online database */
void
cmd_oupdate(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    OBJ_DATA *obj;
    char buf[256];

    SWITCH_BLOCK(ch);

    if (!ch->desc)
        return;

    argument = one_argument(argument, buf, sizeof(buf));
    if (!(obj = get_obj_vis(ch, buf))) {
        send_to_char("There is no object of that name.\n\r", ch);
        return;
    }

    if (obj_index[obj->nr].vnum == 0)
        if (ch->player.level < 4) {
            send_to_char("Do not update object #0.\n\r", ch);
            return;
        }

    if (!has_privs_in_zone(ch, obj_index[obj->nr].vnum / 1000, MODE_STR_EDIT)) {
        send_to_char("You do not have creation privs in that zone.\n\r", ch);
        return;
    }

    obj_update(obj);
    send_to_char("Done.\n\r", ch);
}


/* save the online database to disk for a zone */
void
cmd_osave(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256], err[256];
    int zone;

    SWITCH_BLOCK(ch);

    argument = one_argument(argument, buf, sizeof(buf));

    if (!is_number(buf)) {
        sprintf(err, "Osave requires a number. '%s' is not a number.\n\r", buf);
        send_to_char(err, ch);
        return;
    }

    zone = atoi(buf);

    if (zone < 0) {
        send_to_char("Zone numbers must be positive.\n\r", ch);
        return;
    }

    if (!*buf)
        zone = ch->in_room->zone;
    else if (!stricmp(buf, "all") && (GET_LEVEL(ch) >= HIGHLORD)) {
        send_to_char("Object save in progress...\n\r", ch);
        for (zone = 0; zone < top_of_zone_table; zone++)
            save_objects(zone);
        send_to_char("Object save completed.\n\r", ch);
    }

    if (!has_privs_in_zone(ch, zone, MODE_STR_EDIT)) {
        send_to_char("You don't have creation privs in that zone.\n\r", ch);
        return;
    }

    send_to_char("Object save in progress...\n\r", ch);
    save_objects(zone);
    send_to_char("Object save completed.\n\r", ch);
}


/* delete an object */
void
cmd_odelete(CHAR_DATA * ch, const char *arg, Command cmd, int count)
{
    /* getting really tired of calix crashing us with this command.
     * -Nessalin Jan, 18, 2000 */
    send_to_char("Broken, leave it alone.\n\r", ch);

    SWITCH_BLOCK(ch);
    return;
}

void
string_safe_cat(char *original, const char *add, int maxlen)
{
    int a;
    int b;
    a = strlen(original);
    b = strlen(add);
    if (a + b >= maxlen) {
        if (strlen("TEXT too long, beginning cut off\n\r") >= maxlen)
            strcpy(original, "");
        else
            strcpy(original, "TEXT too long, beginning cut off\n\r");
    } else
        strcat(original, add);
}

/* show the statistics of an object */



void
stat_object(OBJ_DATA * obj, CHAR_DATA * ch)
{
    int r, s_count, rnd_count;
    struct skin_data *skin;

    char buf[MAX_STRING_LENGTH], output[MAX_STRING_LENGTH];
    char *obuf;
    /*char logmsg[256]; */
    int flag_check, col;
    int i, j, num;
    float floater;
    struct extra_descr_data *e_tmp;
    struct guard_type *curr_guard;

    strcpy(output, "");

    if (obj->nr > -1) {
        sprintf(buf, "Object number %d, of zone %d (%s).\n\r\n\r", obj_index[obj->nr].vnum,
                obj_index[obj->nr].vnum / 1000, zone_table[obj_index[obj->nr].vnum / 1000].name);
    } else
        sprintf(buf, "Object number -1, of zone -1 (None).\n\r\n\r");

    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    display_specials(buf, ARRAY_LENGTH(buf), obj->programs, 1);
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    sprintf(buf, "Keyword list:      '%s'\n\r", OSTR(obj, name));
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    sprintf(buf, "Short description: '%s'\n\r", OSTR(obj, short_descr));
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    sprintf(buf, "Plural Short description: '%s'\n\r", pluralize(OSTR(obj, short_descr)));
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    sprintf(buf, "Long description:  '%s'\n\r", OSTR(obj, long_descr));
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    sprintf(buf, "Description:\n\r%s\n\r", OSTR(obj, description));
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    switch (obj->obj_flags.type) {
    case ITEM_LIGHT:
        sprintf(buf, "Type: LIGHT\n\r");
        break;
    case ITEM_COMPONENT:
        sprintf(buf, "Type: COMPONENT\n\r");
        break;
    case ITEM_WAND:
        sprintf(buf, "Type: WAND\n\r");
        break;
    case ITEM_STAFF:
        sprintf(buf, "Type: STAFF\n\r");
        break;
    case ITEM_WEAPON:
        sprintf(buf, "Type: WEAPON\n\r");
        break;
    case ITEM_ART:
        sprintf(buf, "Type: ART\n\r");
        break;
    case ITEM_TOOL:
        sprintf(buf, "Type: TOOL\n\r");
        break;
    case ITEM_TREASURE:
        sprintf(buf, "Type: TREASURE\n\r");
        break;
    case ITEM_ARMOR:
        sprintf(buf, "Type: ARMOR\n\r");
        break;
    case ITEM_POTION:
        sprintf(buf, "Type: POTION\n\r");
        break;
    case ITEM_WORN:
        sprintf(buf, "Type: WORN\n\r");
        break;
    case ITEM_OTHER:
        sprintf(buf, "Type: OTHER\n\r");
        break;
    case ITEM_RUMOR_BOARD:
        sprintf(buf, "Type: RUMOR_BOARD\n\r");
        break;
    case ITEM_SKIN:
        sprintf(buf, "Type: SKIN\n\r");
        break;
    case ITEM_SCROLL:
        sprintf(buf, "Type: SCROLL\n\r");
        break;
    case ITEM_CONTAINER:
        sprintf(buf, "Type: CONTAINER\n\r");
        break;
    case ITEM_NOTE:
        sprintf(buf, "Type: NOTE\n\r");
        break;
    case ITEM_DRINKCON:
        sprintf(buf, "Type: DRINKCON\n\r");
        break;
    case ITEM_KEY:
        sprintf(buf, "Type: KEY\n\r");
        break;
    case ITEM_FOOD:
        sprintf(buf, "Type: FOOD\n\r");
        break;
    case ITEM_MONEY:
        sprintf(buf, "Type: MONEY\n\r");
        break;
    case ITEM_PEN:
        sprintf(buf, "Type: PEN\n\r");
        break;
    case ITEM_BOAT:
        sprintf(buf, "Type: BOAT\n\r");
        break;
    case ITEM_WAGON:
        sprintf(buf, "Type: WAGON\n\r");
        break;
    case ITEM_DART:
        sprintf(buf, "Type: DART\n\r");
        break;
    case ITEM_SPICE:
        sprintf(buf, "Type: SPICE\n\r");
        break;
    case ITEM_CHIT:
        sprintf(buf, "Type: CHIT\n\r");
        break;
    case ITEM_POISON:
        sprintf(buf, "Type: POISON\n\r");
        break;
    case ITEM_BOW:
        sprintf(buf, "Type: BOW\n\r");
        break;
    case ITEM_TELEPORT:
        sprintf(buf, "Type: TELEPORT\n\r");
        break;
    case ITEM_FIRE:
        sprintf(buf, "Type: FIRE\n\r");
        break;
    case ITEM_MASK:
        sprintf(buf, "Type: MASK\n\r");
        break;
    case ITEM_FURNITURE:
        sprintf(buf, "Type: FURNITURE\n\r");
        break;
    case ITEM_HERB:
        sprintf(buf, "Type: HERB\n\r");
        break;
    case ITEM_CURE:
        sprintf(buf, "Type: CURE\n\r");
        break;
    case ITEM_SCENT:
        sprintf(buf, "Type: SCENT\n\r");
        break;
    case ITEM_PLAYABLE:
        sprintf(buf, "Type: PLAYABLE\n\r");
        break;
    case ITEM_MINERAL:
        sprintf(buf, "Type: MINERAL\n\r");
        break;
    case ITEM_JEWELRY:
        sprintf(buf, "Type: JEWELRY\n\r");
        break;
    case ITEM_MUSICAL:
        sprintf(buf, "Type: MUSICAL\n\r");
        break;

    default:
        sprintf(buf, "Type: UNKNOWN\n\r");
        break;
    }
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    floater = (float) obj->obj_flags.weight;
    floater /= 100;

    sprintf(buf, "Weight: %.2f stones (%.2f current)  Value: %d Coins  Damage:%dd%d+%d\n\r", floater, GET_OBJ_WEIGHT(obj),
            obj->obj_flags.cost, obj->dam_num_dice, obj->dam_dice_size, obj->dam_plus);
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    for (num = 0; *obj_material[num].name; num++)
        if (obj_material[num].val == obj->obj_flags.material)
            break;

    if (!*obj_material[num].name || (obj->obj_flags.material == MATERIAL_NONE))
        sprintf(buf, "Material: Unknown   ");
    else
        sprintf(buf, "Material: %s   ", obj_material[num].name);
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    i = obj_index[obj->nr].limit;
    if (i == -1)
        sprintf(buf, "Limit: None.\n\r");
    else
        sprintf(buf, "Limit: %d\n\r", i);
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    sprintf(buf, "Hit Points: %d   Max Hit Points: %d\n\r", obj->hit_points, obj->max_hit_points);
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    if (obj->carried_by) {
        sprintf(buf, "Carried by %s in room %s [%d].\n\r", MSTR(obj->carried_by, name),
                obj->carried_by->in_room ? obj->carried_by->in_room->name : "<null room>",
                obj->carried_by->in_room ? obj->carried_by->in_room->number : 0);
        string_safe_cat(output, buf, MAX_STRING_LENGTH);
    }

    if (obj->equipped_by) {
        sprintf(buf, "Equipped by %s in room %s [%d].\n\r", MSTR(obj->equipped_by, name),
                obj->equipped_by->in_room ? obj->equipped_by->in_room->name : "<null room>",
                obj->equipped_by->in_room ? obj->equipped_by->in_room->number : 0);
        string_safe_cat(output, buf, MAX_STRING_LENGTH);
    }

    for (r = 1; r < MAX_RACES; r++) {
        skin = race[r].skin;
        while (skin) {
            if (obj_index[obj->nr].vnum == skin->item) {
                sprintf(buf, "Can be skinned from race '%s' with a %d modifier.\n\r", race[r].name,
                        skin->bonus);
                string_safe_cat(output, buf, MAX_STRING_LENGTH);
            }
            skin = skin->next;
        }
    }


    for (s_count = 0; s_count < max_shops; s_count++) {
        for (rnd_count = 0; rnd_count < MAX_NEW; rnd_count++) {
            if ((shop_index[s_count].new_item[rnd_count] == obj_index[obj->nr].vnum)
                && (obj_index[obj->nr].vnum != 0)) {
                sprintf(buf, "Loads as a random item on merchant NPC #%d.\n\r",
                        npc_index[shop_index[s_count].merchant].vnum);
                string_safe_cat(output, buf, MAX_STRING_LENGTH);
            }
        }
        for (rnd_count = 0; rnd_count < MAX_NEW; rnd_count++) {
            if ((shop_index[s_count].producing[rnd_count] > 0)) /* TEN 2004/04 */
                if ((obj_index[shop_index[s_count].producing[rnd_count]].vnum ==
                     obj_index[obj->nr].vnum) && (obj_index[obj->nr].vnum != 0)) {
                    sprintf(buf, "Loads as a infinite item on merchant NPC #%d.\n\r",
                            npc_index[shop_index[s_count].merchant].vnum);
                    string_safe_cat(output, buf, MAX_STRING_LENGTH);
                }
        }
    }
    add_forage_info_to_buf(output, sizeof(output), obj_index[obj->nr].vnum);

    if (obj->in_obj) {
        OBJ_DATA *in_obj = obj->in_obj;

        if (GET_ITEM_TYPE(in_obj) == ITEM_FURNITURE && IS_SET(in_obj->obj_flags.value[1], FURN_PUT)) {
            sprintf(buf, "On %s [%d]", OSTR(in_obj, short_descr), obj_index[in_obj->nr].vnum);
            string_safe_cat(output, buf, MAX_STRING_LENGTH);
        } else {
            sprintf(buf, "In %s [%d]", OSTR(in_obj, short_descr), obj_index[in_obj->nr].vnum);
            string_safe_cat(output, buf, MAX_STRING_LENGTH);
        }

        if (in_obj->in_room) {
            sprintf(buf, ", in %s [%d]", in_obj->in_room ? in_obj->in_room->name : "<null room>",
                    in_obj->in_room ? in_obj->in_room->number : 0);
            string_safe_cat(output, buf, MAX_STRING_LENGTH);
        }
        string_safe_cat(output, ".\n\r", MAX_STRING_LENGTH);
    }

    if (obj->in_room) {
        sprintf(buf, "In room %s [%d].\n\r", obj->in_room ? obj->in_room->name : "<null room>",
                obj->in_room ? obj->in_room->number : 0);
        string_safe_cat(output, buf, MAX_STRING_LENGTH);
    }

    if (obj->ex_description) {
        string_safe_cat(output, "Extra Descriptions:\n\r", MAX_STRING_LENGTH);
        for (e_tmp = obj->ex_description; e_tmp; e_tmp = e_tmp->next) {
            string_safe_cat(output, (e_tmp->def_descr ? "  -  " : "     "), MAX_STRING_LENGTH);
            string_safe_cat(output, e_tmp->keyword, MAX_STRING_LENGTH);
            string_safe_cat(output, "\n\r", MAX_STRING_LENGTH);
        }
    }

    /* ---------------------- New eflag checker ------------------------
     * -Nessalin 7/20/97 */

    *buf = '\0';

    sprintf(buf, "EFLAGS:");
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    for (flag_check = 0, col = 0; *obj_extra_flag[flag_check].name; flag_check++)
        if (IS_SET(obj->obj_flags.extra_flags, obj_extra_flag[flag_check].bit)) {
            sprintf(buf, "%s %-15s", (!col && flag_check) ? "\n\r" : "",
                    obj_extra_flag[flag_check].name);

            string_safe_cat(output, buf, MAX_STRING_LENGTH);

            col++;

            if (col > 3)
                col = 0;
        }

    string_safe_cat(output, "\n\r", MAX_STRING_LENGTH);

    /* ---------------------- New wflag checker ------------------------
     * -Nessalin 7/26/97 */

    *buf = '\0';

    sprintf(buf, "WFLAGS:\n\r");
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    for (flag_check = 0, col = 0; *obj_wear_flag[flag_check].name; flag_check++)
        if (IS_SET(obj->obj_flags.wear_flags, obj_wear_flag[flag_check].bit)) {
            sprintf(buf, "%s %-15s", (!col && flag_check) ? "\n\r" : "",
                    obj_wear_flag[flag_check].name);

            string_safe_cat(output, buf, MAX_STRING_LENGTH);

            col++;

            if (col > 3)
                col = 0;
        }

    string_safe_cat(output, "\n\r", MAX_STRING_LENGTH);
    *buf = '\0';

    /* ---------------------- End new wflag checker --------------------- */

    /* ---------------------- New sflag checker ------------------------ 
     * -Tiernan 7/25/01 */

    *buf = '\0';

    sprintf(buf, "SFLAGS:\n\r");
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    for (flag_check = 0, col = 0; *obj_state_flag[flag_check].name; flag_check++)
        if (IS_SET(obj->obj_flags.state_flags, obj_state_flag[flag_check].bit)) {
            sprintf(buf, "%s %-15s", (!col && flag_check) ? "\n\r" : "",
                    obj_state_flag[flag_check].name);

            string_safe_cat(output, buf, MAX_STRING_LENGTH);

            col++;

            if (col > 3)
                col = 0;
        }

    string_safe_cat(output, "\n\r", MAX_STRING_LENGTH);
    *buf = '\0';

    /* ---------------------- End new sflag checker --------------------- */

    /* ---------------------- New cflag checker ------------------------ 
     * -Nessalin 1/11/03 */

    *buf = '\0';

    sprintf(buf, "COLOR FLAGS:\n\r");
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    for (flag_check = 0, col = 0; *obj_color_flag[flag_check].name; flag_check++)
        if (IS_SET(obj->obj_flags.color_flags, obj_color_flag[flag_check].bit)) {
            sprintf(buf, "%s %-15s", (!col && flag_check) ? "\n\r" : "",
                    obj_color_flag[flag_check].name);

            string_safe_cat(output, buf, MAX_STRING_LENGTH);

            col++;

            if (col > 3)
                col = 0;
        }

    string_safe_cat(output, "\n\r", MAX_STRING_LENGTH);
    *buf = '\0';

    /* ---------------------- End new sflag checker --------------------- */


    /* ---------------------- New cflag checker ------------------------ 
     * -Nessalin 1/11/03 */

    *buf = '\0';

    sprintf(buf, "ADVERBIAL FLAGS:\n\r");
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    for (flag_check = 0, col = 0; *obj_adverbial_flag[flag_check].name; flag_check++)
        if (IS_SET(obj->obj_flags.adverbial_flags, obj_adverbial_flag[flag_check].bit)) {
            sprintf(buf, "%s %-15s", (!col && flag_check) ? "\n\r" : "",
                    obj_adverbial_flag[flag_check].name);

            string_safe_cat(output, buf, MAX_STRING_LENGTH);

            col++;

            if (col > 3)
                col = 0;
        }

    string_safe_cat(output, "\n\r", MAX_STRING_LENGTH);
    *buf = '\0';

    /* ---------------------- End new sflag checker --------------------- */


    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    if ((obj->obj_flags.type == ITEM_WAGON) && (obj->obj_flags.value[2] && obj->obj_flags.value[0])) {
        sprintf(buf, "Wagon weight: %d (Empty: %d)\n\r", calc_wagon_weight(obj),
                calc_wagon_weight_empty(obj));
        string_safe_cat(output, buf, MAX_STRING_LENGTH);
        sprintf(buf, "Maximum wagon damage: %d\n\r", calc_wagon_max_damage(obj));
        string_safe_cat(output, buf, MAX_STRING_LENGTH);
        if (obj->obj_flags.temp) {
            sprintf(buf, "The wagon is facing: %s\n\r", dirs[(obj->obj_flags.temp - 1)]);
            string_safe_cat(output, buf, MAX_STRING_LENGTH);
        }
    }

    sprintf(buf, "Affects characters with:\n\r");
    string_safe_cat(output, buf, MAX_STRING_LENGTH);



    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
        if (obj->affected[i].location > 0) {
            for (j = 0; *obj_apply_types[j].name; j++)
                if (obj_apply_types[j].val == obj->affected[i].location)
                    break;
            if (*obj_apply_types[j].name != '\0') {
                sprintf(buf, "     [%d] a %d %s to %s.\n\r", i, obj->affected[i].modifier,
                        (obj->affected[i].modifier > 0) ? "bonus" : "penalty",
                        obj_apply_types[j].name);
            } else {
                int loc;

                loc = obj->affected[i].location;

                if ((loc >= CHAR_APPLY_SKILL_OFFSET)
                    && (loc < (CHAR_APPLY_SKILL_OFFSET + MAX_SKILLS))) {
                    int skill;
                    skill = loc - CHAR_APPLY_SKILL_OFFSET;

                    sprintf(buf, "     [%d] a %d %s to %s.\n\r", i, obj->affected[i].modifier,
                            (obj->affected[i].modifier > 0) ? "bonus" : "penalty",
                            skill_name[skill]);

                }
            }
        } else {
            sprintf(buf, "     [%d] nothing.\n\r", i);
        }
        string_safe_cat(output, buf, MAX_STRING_LENGTH);

        sprintf(buf, "  loc   %d, mod  %d\n\r", obj->affected[i].location,
                obj->affected[i].modifier);

        string_safe_cat(output, buf, MAX_STRING_LENGTH);
    }

    if (obj->obj_flags.type == ITEM_KEY) {
        int rm_num = 0;
        int dir = 0;
        ROOM_DATA *room = NULL;

        for (rm_num = 0; rm_num <= 77999; rm_num++) {
            room = get_room_num(rm_num);
            if (room) {
                for (dir = 0; dir < 6; dir++) {
                    if (room->direction[dir]) {
                        if (room->direction[dir]->key == obj_index[obj->nr].vnum) {
                            sprintf(buf, "Opens the %s exit in room %d.\n\r", dirs[dir], rm_num);
                            string_safe_cat(output, buf, MAX_STRING_LENGTH);
                        }
                    }
                }
            }
        }
    }


    if ((obj->obj_flags.type < MAX_ITEM_TYPE) && (obj->obj_flags.type > 0)) {
        for (i = 0; i < 6; i += 2) {
            obuf = show_oval(obj, i);

            sprintf(buf, "val%d(%10s)-%-15s", i + 1, oset_field[(int) obj->obj_flags.type].set[i],
                    obuf);
            free(obuf);

            string_safe_cat(output, buf, MAX_STRING_LENGTH);

            obuf = show_oval(obj, i + 1);
            sprintf(buf, "val%d(%10s)-%s\n\r", i + 2,
                    oset_field[(int) obj->obj_flags.type].set[i + 1], obuf);
            free(obuf);
            string_safe_cat(output, buf, MAX_STRING_LENGTH);
        }
    };
    heap_print(buf, obj, event_obj_cmp);
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    if (obj->guards) {
        sprintf(buf, "guards:\n\r ");
        string_safe_cat(output, buf, MAX_STRING_LENGTH);


        for (curr_guard = obj->guards; curr_guard; curr_guard = curr_guard->next) {     /* being loop searching room for guard */
            sprintf(buf, "      %s\n\r ", GET_NAME(curr_guard->guard));
            string_safe_cat(output, buf, MAX_STRING_LENGTH);
        }
    };

    page_string(ch->desc, output, 1);
}

void
cmd_ostat(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{                               /* Cmd_Ostat */
    char arg1[MAX_STRING_LENGTH];
    OBJ_DATA *obj;

    argument = one_argument(argument, arg1, sizeof(arg1));

    if (!(obj = get_obj_vis(ch, arg1))) {
        send_to_char("No object by that name exists.\n\r", ch);
        return;
    }

    stat_object(obj, ch);
}

void
cmd_oindex(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[MAX_STRING_LENGTH];
    char old_arg[MAX_INPUT_LENGTH];
    char extraCom[MAX_INPUT_LENGTH];
    char remainder[MAX_INPUT_LENGTH];
    int virt_nr;
    OBJ_DATA *obj;

    argument = one_argument(argument, buf, sizeof(buf));
    strcpy(old_arg, argument);
    argument = one_argument(argument, extraCom, sizeof(buf));
    virt_nr = atoi(buf);

    if (virt_nr < 0) {
        send_to_char("You can't do that.\n\r", ch);
        return;
    }

    if (!has_privs_in_zone(ch, virt_nr / 1000, MODE_CREATE)) {
        send_to_char("You do not have creation privileges in that zone.\n\r", ch);
        return;
    }

    if (real_object(virt_nr) == -1) {
        if( IS_SWITCHED_IMMORTAL(ch) ) {
            cprintf( ch, "No such object #%d, can't create while switched.\n\r", virt_nr);
            return;
        }

        if (!has_privs_in_zone(ch, virt_nr / 1000, MODE_STR_EDIT)) {
            send_to_char("You do not have creation privileges in that " "zone.\n\r", ch);
            return;
        }
        add_new_item_in_creation(virt_nr);
    }

    if (!(obj = read_object(virt_nr, VIRTUAL))) {
        send_to_char("That object does not exist.\n\r", ch);
        return;
    }

    /* If they didn't specify anything else */
    if (!*old_arg) {
        stat_object(obj, ch);
    } else {                    /* see if they want an edesc */

        char *edesc;

        if (!strcmp(extraCom, "edesc")) {
            strcpy(remainder, argument);
        } else {
            strcpy(remainder, old_arg);
        }

        edesc = find_ex_description_all_words(remainder, obj->ex_description, TRUE);

        if (edesc != NULL && *edesc != '\0') {
            sprintf(buf, "%s [o%d]'s edesc %s:\n\r%s\n\r", OSTR(obj, short_descr), virt_nr,
                    remainder, edesc);
            send_to_char(buf, ch);
        } else {
            sprintf(buf, "Unable to find an edesc '%s' on %s [o%d].\n\r", remainder,
                    OSTR(obj, short_descr), virt_nr);
            send_to_char(buf, ch);
        }
    }

    extract_obj(obj);
}

