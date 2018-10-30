/*
 * File: pc_file_game.c
 * Usage: implementation of routines to save pc's to disk
 *
 * Copyright (C) 1996 all original source by Jim Tripp (Tenebrius), Mark
 * Tripp (Nessalin), Hal Roberts (Bram), and Mike Zeman (Azroen).  May 
 * favourable winds fill the sails of their skimmers.
 *
 *   /\    ____                    PART OF          __     ____   |\   |
 *  /__\   |___|   /\  /\    /\    __  ___  +--\   |  \   /    \  | \  |
 * /    \  |   \  /  \/  \  /--\  /    [_   |   \  |   |  |    |  |  \ |
 * \    /  |   /  \      /  \  /  \_]  [__  |___/  |__/   \____/  |   \|
 *
 *                       Version 4.0  Generation 2 
 * ---------------------------------------------------------------------
 *
 * Original DikuMUD 1.0 courtesy of Katja Nyboe, Tom Madsen, Hans
 * Staerfeldt, Michael Seifert, and Sebastian Hammer.  Kudos.
 */

/* Revision History:
 * 05/17/2000  Commented out the part that was making mounts acquire 
 *             their rider's clan.  -Sanvean
 */

#include <stdio.h>
#include <ctype.h>
#include <stdarg.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>

#include "constants.h"
#include "structs.h"
#include "clan.h"
#include "guilds.h"
#include "cities.h"
#include "utils.h"
#include "utility.h"
#include "limits.h"
#include "db.h"
#include "event.h"
#include "pc_file.h"
#include "handler.h"
#include "parser.h"
#include "character.h"
#include "creation.h"
#include "watch.h"

#define SAVE_OBJ_EDESCS 1

/* begin higher level character saving routines */

/* returns a string which is a list of all the flags of 'flags' that
   are set in 'vector' in the form of 
   "flag1 / flag2 / flag3 / ... /" */
char *
str_from_flags(struct flag_data *flags, int vector)
{

    static char list[MAX_LINE_LENGTH];
    int i;

    list[0] = '\0';
    for (i = 0; strlen(flags[i].name); i++)
        if (IS_SET(flags[i].bit, vector)) {
            strcat(list, flags[i].name);
            strcat(list, " / ");
        }

    return list;
}

/* writes character affects */
void
write_char_affects(int ref, struct affected_type *af)
{

    int i = 0;

    char field[64];
    char location[128];

/* old affect output 
  while (af) {
    sprintf(field, "AFFECT %d", i++);
    sprint_attrib(af->location, obj_apply_types, location);
    write_char_field_f("CHAR", ref, field, "%d %d %s %s %s", 
		       af->duration, af->modifier,
		       location, str_from_flags(affect_flag, af->bitvector),
		       skill_name[af->type]);
    af = af->next;
  } */

    while (af) {
        sprintf(field, "AFFECT_DURATION %d", i);
        write_char_field_f("CHAR", ref, field, "%d", af->duration);

        sprintf(field, "AFFECT_MOD %d", i);
        write_char_field_f("CHAR", ref, field, "%d", (int) af->modifier);

        sprintf(field, "AFFECT_LOC %d", i);
        sprint_attrib(af->location, obj_apply_types, location);
        write_char_field_f("CHAR", ref, field, "%s", location);

        sprintf(field, "AFFECT_FLAGS %d", i);
        write_char_field_f("CHAR", ref, field, "%s", str_from_flags(affect_flag, af->bitvector));

        sprintf(field, "AFFECT_SKILL %d", i);

        /* check bounderies, a -1 is getting in there somehow -Ten */
        if (af->type >= 0)
            write_char_field_f("CHAR", ref, field, "%s", skill_name[af->type]);
        else
            write_char_field_f("CHAR", ref, field, "%s", skill_name[0]);



        sprintf(field, "AFFECT_POWER %d", i);
        if (((af->power - 1) >= POWER_WEK) && ((af->power - 1) <= POWER_MON))
            write_char_field_f("CHAR", ref, field, "%s", Power[af->power - 1]);
        else
            write_char_field_f("CHAR", ref, field, "%s", Power[0]);

        sprintf(field, "AFFECT_MAGICKTYPE %d", i);
        if ((af->magick_type >= 0) && (af->magick_type < 5))
            write_char_field_f("CHAR", ref, field, "%s", MagickType[af->magick_type]);
        else
            write_char_field_f("CHAR", ref, field, "%s", MagickType[0]);

        af = af->next;
        i++;
    }

    return;
}

/* writes character edescs */
void
write_char_edescs(int ref, struct extra_descr_data *extra_desc)
{
    int i = 0;
    char field[64];

    for (; extra_desc; extra_desc = extra_desc->next) {
        if (strstr(extra_desc->keyword, "__tmp__")) continue;

        sprintf(field, "EXTRA_DESC_KEYWORDS %d", i);
        write_char_field_f("CHAR", ref, field, "%s", extra_desc->keyword);

        sprintf(field, "EXTRA_DESC_DESCRIPTION %d", i);
        write_char_field_f("CHAR", ref, field, "%s", extra_desc->description);

        i++;
    }

    return;
}

/* writes object edescs */
void
write_obj_edescs(int ref, struct extra_descr_data *extra_desc)
{
    int i = 0;
    char field[64];

    for (; extra_desc; extra_desc = extra_desc->next) {
        if (strstr(extra_desc->keyword, "__tmp__")) continue;

        sprintf(field, "EXTRA_DESC_KEYWORDS %d", i);
        write_char_field_f("OBJECT", ref, field, "%s", extra_desc->keyword);
#define CHECK_WRITE_OBJ_EDESC_LEN
        /* added 5/29/2000 -Tenebrius, remove other code after 8/29/2000 if no problems */
#ifdef CHECK_WRITE_OBJ_EDESC_LEN
        /* edescs can be pretty long */
        if (extra_desc->description) {
            char edesc[(MAX_LINE_LENGTH * 2) + 10];

            sprintf(field, "EXTRA_DESC_DESCRIPTION %d", i);

            if (strlen(extra_desc->description) > ((MAX_LINE_LENGTH * 2) - 3)) {
                memcpy(edesc, extra_desc->description, MAX_LINE_LENGTH * 2);
                edesc[(MAX_LINE_LENGTH * 2) - 2] = 0;   /* terminmate early */

                gamelog("write_obj_edesc: edesc too long, truncating ");

                write_char_field_f("OBJECT", ref, field, "%s", edesc);
            } else {
                write_char_field_f("OBJECT", ref, field, "%s", extra_desc->description);
            }
        } else {
            sprintf(field, "EXTRA_DESC_DESCRIPTION %d", i);
            write_char_field_f("OBJECT", ref, field, "%s", "");
        }
#else
        sprintf(field, "EXTRA_DESC_DESCRIPTION %d", i);
        write_char_field_f("OBJECT", ref, field, "%s", extra_desc->description);
#endif
        i++;
    }

    return;
}

/* writes object affects */
void
write_obj_affects(int ref, OBJ_DATA * obj)
{

    int i;

    char field[64];
    char location[128];

    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
        sprintf(field, "AFFECT %d", i);

        if (obj->affected[i].location >= CHAR_APPLY_SKILL_OFFSET) {
            int skill;
            skill = obj->affected[i].location - CHAR_APPLY_SKILL_OFFSET;

            if ((skill >= 0) && (skill <= MAX_SKILLS))
                sprintf(location, "%s", skill_name[skill]);
            else
                strcpy(location, "");

        } else {
            sprint_attrib(obj->affected[i].location, obj_apply_types, location);
        };
        write_char_field_f("OBJECT", ref, field, "%s %d", location, obj->affected[i].modifier);
    }

    return;
}

/* writes skills */
void
write_char_skills(int ref, CHAR_DATA * ch)
{

    char name[64];
    int i;

    for (i = 0; i < MAX_SKILLS; i++)
        if (ch->skills[i] && strlen(skill_name[i])) {
            sprintf(name, "SKILL %s", skill_name[i]);
            write_char_field_f("CHAR", ref, name, "%d %d %d %d", ch->skills[i]->learned,
                               ch->skills[i]->rel_lev, ch->skills[i]->taught_flags,
                               (int) ch->skills[i]->last_gain);
        }

    return;
}

/* writes attached programs */
void
write_char_programs(char *label, int ref, CHAR_DATA * ch)
{

    int i;
    char field[80];
    struct special *prog;

    if (!ch->specials.programs)
        return;

    for (i = 0; i < ch->specials.programs->count; i++) {
        sprintf(field, "PROGRAM %d", i);
        prog = &ch->specials.programs->specials[i];
        switch (prog->type) {
        case (SPECIAL_CODED):
            write_char_field_f(label, ref, field, "%s %s %s", "C", command_name[prog->cmd],
                               prog->special.cspec->name);
            break;
        case (SPECIAL_DMPL):
            write_char_field_f(label, ref, field, "%s %s %s", "dmpl", command_name[prog->cmd],
                               prog->special.name);
            break;
        case (SPECIAL_JAVASCRIPT):
            write_char_field_f(label, ref, field, "%s %s %s", "javascript", command_name[prog->cmd],
                               prog->special.name);
            break;
        }
    }
}


/* write a single object */
void
write_char_obj(OBJ_DATA * obj)
{
    int i, ref;
    OBJ_DATA *content;
    char buf[MAX_STRING_LENGTH];

    if (obj->nr < 1 || IS_SET(obj->obj_flags.extra_flags, OFL_NOSAVE))
        return;

    ref = add_obj_to_table(obj);
/*  write_char_label("OBJECT", (ref = add_obj_to_table(obj)));*/

    write_char_field_f("OBJECT", ref, "VIRTUAL NUMBER", "%d", obj_index[obj->nr].vnum);
    if (obj->equipped_by) {
        for (i = 0; i < MAX_WEAR; i++)
            if (obj->equipped_by->equipment[i] == obj)
                break;
        if (i == MAX_WEAR) {
            gamelog("write_char_obj: Unable to find equipped object location");
            write_char_field_f("OBJECT", ref, "LOCATION", "%s of %d", "inventory",
                               ref_from_char(current.ch));
        }
        write_char_field_f("OBJECT", ref, "LOCATION", "%s of %d", wear_name[i],
                           ref_from_char(obj->equipped_by));
    } else if (obj->carried_by) {
        write_char_field_f("OBJECT", ref, "LOCATION", "%s of %d", "inventory",
                           ref_from_char(obj->carried_by));
    } else if (obj->in_obj)
        write_char_field_f("OBJECT", ref, "LOCATION", "%s of %d", "content",
                           ref_from_obj(obj->in_obj));
    else {
        gamelog("write_char_obj: Object is neither in inventory, equipped, or in "
                "another object.");
        write_char_field_f("OBJECT", ref, "LOCATION", "%s of %d", "inventory",
                           ref_from_char(current.ch));
    }

    /*
     * write_char_field_f("OBJECT", ref, "VALUES", "%d %d %d %d", 
     * obj->obj_flags.value[0],
     * obj->obj_flags.value[1], obj->obj_flags.value[2],
     * obj->obj_flags.value[3]); */

    /* lets try this again, but with a bit more freedom */
    for (i = 0; i < 6; i++) {
        sprintf(buf, "VALUE %d", i);
        write_char_field_f("OBJECT", ref, buf, "%d", obj->obj_flags.value[i]);
    }

    write_char_field_f("OBJECT", ref, "EXTRA FLAGS", "%s",
                       str_from_flags(obj_extra_flag, obj->obj_flags.extra_flags));
    write_char_field_f("OBJECT", ref, "CHAR FLAGS", "%s",
                       str_from_flags(char_flag, obj->obj_flags.bitvector));
    write_char_field_f("OBJECT", ref, "STATE FLAGS", "%s",
                       str_from_flags(obj_state_flag, obj->obj_flags.state_flags));
    write_char_field_f("OBJECT", ref, "COLOR FLAGS", "%s",
                       str_from_flags(obj_color_flag, obj->obj_flags.color_flags));
    write_char_field_f("OBJECT", ref, "ADVERBIAL FLAGS", "%s",
                       str_from_flags(obj_adverbial_flag, obj->obj_flags.adverbial_flags));

    write_obj_affects(ref, obj);

#ifdef SAVE_OBJ_EDESCS
    if (obj->ex_description)
        write_obj_edescs(ref, obj->ex_description);
#endif

    if (obj->obj_flags.type == ITEM_NOTE)
        save_note(obj);

    for (content = obj->contains; content; content = content->next_content)
        write_char_obj(content);

}

/* write all of a character's objects */
void
write_char_objects(CHAR_DATA * ch)
{

    OBJ_DATA *obj;
    int i;

    for (obj = ch->carrying; obj; obj = obj->next_content)
        write_char_obj(obj);

    for (i = 0; i < MAX_WEAR; i++)
        if (ch->equipment[i])
            write_char_obj(ch->equipment[i]);

    return;
}


/* recursive writer to save a character's clans */
void
write_char_clans(CLAN_DATA * pClan, int ref, int level)
{
    char buf[MAX_STRING_LENGTH];
    char name[MAX_STRING_LENGTH];

    if (!pClan)
        return;

    sprintf(name, "CLAN %d", level);

    if (pClan->clan <= 0 || pClan->clan >= MAX_CLAN)
        sprintf(buf, "%d", pClan->clan);
    else
        sprintf(buf, "%s", clan_table[pClan->clan].name);

    write_char_field_f("CHAR", ref, name, "%s %s %d", buf,
                       str_from_flags(clan_job_flags, pClan->flags), pClan->rank);

    /* recursive call to next clan */
    if (pClan->next)
        write_char_clans(pClan->next, ref, level + 1);
}


/* write a single mount */
void
write_char_mnt(CHAR_DATA * mnt, int is_mount)
{

    int ref;

    if (IS_AFFECTED(mnt, CHAR_AFF_SUMMONED))
        return;

    ref = add_char_to_table(mnt);
    /*write_char_label("CHAR", (ref = add_char_to_table(mnt))); */

    write_char_field_f("CHAR", ref, "VIRTUAL NUMBER", "%d", npc_index[mnt->nr].vnum);

    write_char_field_f("CHAR", ref, "LOCATION", "%s", (is_mount) ? "mount" : "hitched");
    write_char_field_f("CHAR", ref, "KEYWORDS", "%s", mnt->player.extkwds);
    write_char_field_f("CHAR", ref, "SEX", "%s",
                       (mnt->player.sex ==
                        SEX_MALE) ? "male" : ((mnt->player.sex ==
                                               SEX_FEMALE) ? "female" : "neuter"));


    write_char_field_f("CHAR", ref, "WEIGHT", "%d", mnt->player.weight);
    write_char_field_f("CHAR", ref, "HEIGHT", "%d", mnt->player.height);
    write_char_field_f("CHAR", ref, "THIRST", "%d", mnt->specials.conditions[THIRST]);
    write_char_field_f("CHAR", ref, "FULL", "%d", mnt->specials.conditions[FULL]);
    write_char_field_f("CHAR", ref, "DRUNK", "%d", mnt->specials.conditions[DRUNK]);
    write_char_field_f("CHAR", ref, "CFLAGS", "%s", str_from_flags(char_flag, mnt->specials.act));
    write_char_field_f("CHAR", ref, "BRIEF", "%s", str_from_flags(brief_flag, mnt->specials.brief));
    write_char_field_f("CHAR", ref, "NOSAVE", "%s",
                       str_from_flags(nosave_flag, mnt->specials.nosave));
    write_char_field_f("CHAR", ref, "MERCY", "%s",
                       str_from_flags(mercy_flag, mnt->specials.mercy));
    write_char_field_f("CHAR", ref, "AFLAGS", "%s",
                       str_from_flags(affect_flag, mnt->specials.affected_by));

    write_char_field_f("CHAR", ref, "STR", "%d", mnt->abilities.str);
    write_char_field_f("CHAR", ref, "END", "%d", mnt->abilities.end);
    write_char_field_f("CHAR", ref, "AGL", "%d", mnt->abilities.agl);
    write_char_field_f("CHAR", ref, "WIS", "%d", mnt->abilities.wis);
    write_char_field_f("CHAR", ref, "MANA", "%d", mnt->points.mana);
    write_char_field_f("CHAR", ref, "HITS", "%d", mnt->points.hit);
    write_char_field_f("CHAR", ref, "MAX HITS", "%d", mnt->points.max_hit);
    write_char_field_f("CHAR", ref, "MOVES", "%d", mnt->points.move);
    write_char_field_f("CHAR", ref, "STUN", "%d", mnt->points.stun);
    write_char_field_f("CHAR", ref, "MAX STUN", "%d", mnt->points.max_stun);

    write_char_affects(ref, mnt->affected);
    write_char_edescs(ref, mnt->ex_description);

    write_char_objects(mnt);

}

/* write a character's hitched mounts */
void
write_char_hitched(CHAR_DATA * ch)
{
    struct follow_type *hitch;
    CHAR_DATA *riding = NULL;

    if (ch->specials.riding)
        riding = ch->specials.riding;

    for (hitch = ch->followers; hitch; hitch = hitch->next) {
        if (IS_SET(hitch->follower->specials.act, CFL_MOUNT) && ch->in_room
            && hitch->follower->in_room && (ch->in_room == hitch->follower->in_room)
            && riding != hitch->follower) {
            write_char_mnt(hitch->follower, 0);
            write_char_hitched(hitch->follower);
        }
    }
}

/* write all of a character's mounts (ridden and hitched) */
void
write_char_mounts(CHAR_DATA * ch)
{
    if (ch->specials.riding) {
        write_char_mnt(ch->specials.riding, 1);
        write_char_hitched(ch->specials.riding);
    }
    write_char_hitched(ch);

    return;
}

/* save a character in a text file */
int
save_char_text_dir(CHAR_DATA * ch, char *dir)
{
    OBJ_DATA *char_eq[MAX_WEAR];
    struct affected_type *af;
    char buf[MAX_LINE_LENGTH];
    int i, ref;
    EXTRA_CMD *pCmd;

    /*shhlog("pfile_write:start"); */

    if (!open_char_file(ch, dir))
        return 0;

    for (i = 0; i < MAX_WEAR; i++) {
        if (ch->equipment[i])
            char_eq[i] = unequip_char(ch, i);
        else
            char_eq[i] = 0;
    }

    affect_total(ch);

    /* Remove all affects on the pc so their non-adjusted values are written */
    for (af = ch->affected; af != NULL; af = af->next)
        affect_modify(ch, af->location, af->modifier, af->bitvector, FALSE);

    ref = add_char_to_table(ch);
    /*write_char_label("CHAR", (ref = add_char_to_table(ch))); */

    /* shhlog("pfile_write:name"); */
    write_char_field_f("CHAR", ref, "NAME", "%s", ch->name);
    write_char_field_f("CHAR", ref, "SDESC", "%s", ch->short_descr);
    write_char_field_f("CHAR", ref, "DESC", "%s", ch->description);
    write_char_field_f("CHAR", ref, "TDESC", "%s", ch->temp_description);
    write_char_field_f("CHAR", ref, "KEYWORDS", "%s", ch->player.extkwds);
    write_char_field_f("CHAR", ref, "PREVIOUS CHARACTER", "%s", ch->player.dead);

    for (i = 0; i < 15; i++) {
        sprintf(buf, "INFO %d", i);
        if (!ch->player.info[i])
            ch->player.info[i] = strdup("");
        write_char_field_f("CHAR", ref, buf, "%s", ch->player.info[i]);
    }

    /* shhlog("pfile_write:background"); */
    write_char_field_f("CHAR", ref, "BACKGROUND", "%s", ch->background);

    write_char_field_f("CHAR", ref, "APPLICATIONSTATUS", "%d", ch->application_status);

    if (ch->player.deny)
        write_char_field_f("CHAR", ref, "DENY", "%s", ch->player.deny);

    /* shhlog("pfile_write:sex"); */
    write_char_field_f("CHAR", ref, "SEX", "%s",
                       (ch->player.sex ==
                        SEX_MALE) ? "male" : ((ch->player.sex ==
                                               SEX_FEMALE) ? "female" : "neuter"));

    /* shhlog("pfile_write:guild"); */
    write_char_field_f("CHAR", ref, "GUILD", "%s", guild[(int) (ch->player.guild)].name);

    /* shhlog("pfile_write:guild"); */
    write_char_field_f("CHAR", ref, "SUB-GUILD", "%s",
                       sub_guild[(int) (ch->player.sub_guild)].name);

    /* shhlog("pfile_write:sex"); */
    write_char_field_f("CHAR", ref, "RACE", "%s", race[(int) ch->player.race].name);

    /* shhlog("pfile_write:level"); */
    if ((ch->player.level > 0) && (ch->player.level < 6))
        write_char_field_f("CHAR", ref, "LEVEL", "%s", level_name[(int) ch->player.level]);
    else
        write_char_field_f("CHAR", ref, "LEVEL", "%s", "mortal");


    /* shhlog("pfile_write:granted_commands"); */
    i = 0;
    for (pCmd = ch->granted_commands; pCmd; pCmd = pCmd->next, i++) {
        sprintf(buf, "GCMD%d", i);
        write_char_field_f("CHAR", ref, buf, "%s", command_name[pCmd->cmd]);
        sprintf(buf, "GCMDL%d", i);
        write_char_field_f("CHAR", ref, buf, "%d", pCmd->level);
    }


    /* shhlog("pfile_write:revoked_commands"); */
    i = 0;
    for (pCmd = ch->revoked_commands; pCmd; pCmd = pCmd->next, i++) {
        sprintf(buf, "RCMD%d", i);
        write_char_field_f("CHAR", ref, buf, "%s", command_name[pCmd->cmd]);
        sprintf(buf, "RCMDL%d", i);
        write_char_field_f("CHAR", ref, buf, "%d", pCmd->level);
    }


    /* shhlog("pfile_write:hometowns"); */
    if ((ch->player.hometown > 0) && (ch->player.hometown < MAX_CITIES))
        write_char_field_f("CHAR", ref, "HOMETOWN", "%s", city[ch->player.hometown].name);
    else
        write_char_field_f("CHAR", ref, "HOMETOWN", "%s", city[0].name);

    /* shhlog("pfile_write:startcity"); */
    /* writes theif startcity.  If they don;t have one it will make their start/home the same */
    if ((ch->player.start_city > 0) && (ch->player.start_city < MAX_CITIES))
        write_char_field_f("CHAR", ref, "STARTCITY", "%s", city[ch->player.start_city].name);
    else
        write_char_field_f("CHAR", ref, "STARTCITY", "%s", city[ch->player.hometown].name);


    if (ch->account)
        write_char_field_f("CHAR", ref, "ACCOUNT", "%s", ch->account);

    /* shhlog("pfile_write:tribe");   */
    if ((ch->player.tribe > 0) && (ch->player.tribe < MAX_CLAN))
        write_char_field_f("CHAR", ref, "CLAN", "%s", clan_table[ch->player.tribe].name);
    else
        write_char_field_f("CHAR", ref, "CLAN", "%s", clan_table[0].name);

    if (ch->pagelength)
        write_char_field_f("CHAR", ref, "PAGELEN", "%d", ch->pagelength);

    if (ch->clan)
        write_char_clans(ch->clan, ref, 0);

    write_char_field_f("CHAR", ref, "LUCK", "%d", ch->player.luck);
    write_char_field_f("CHAR", ref, "WEIGHT", "%d", ch->player.weight);
    write_char_field_f("CHAR", ref, "HEIGHT", "%d", ch->player.height);
    write_char_field_f("CHAR", ref, "LANGUAGE", "%s", skill_name[GET_SPOKEN_LANGUAGE(ch)]);
    write_char_field_f("CHAR", ref, "ACCENT", "%s", skill_name[GET_ACCENT(ch)]);
    write_char_field_f("CHAR", ref, "THIRST", "%d", ch->specials.conditions[THIRST]);
    write_char_field_f("CHAR", ref, "FULL", "%d", ch->specials.conditions[FULL]);
    write_char_field_f("CHAR", ref, "DRUNK", "%d", ch->specials.conditions[DRUNK]);
    write_char_field_f("CHAR", ref, "ECO", "%d", ch->specials.eco);
    write_char_field_f("CHAR", ref, "CFLAGS", "%s", str_from_flags(char_flag, ch->specials.act));
    write_char_field_f("CHAR", ref, "QFLAGS", "%s", str_from_flags(quit_flag, ch->quit_flags));
    write_char_field_f("CHAR", ref, "BRIEF", "%s", str_from_flags(brief_flag, ch->specials.brief));
    write_char_field_f("CHAR", ref, "NOSAVE", "%s",
                       str_from_flags(nosave_flag, ch->specials.nosave));
    write_char_field_f("CHAR", ref, "MERCY", "%s",
                       str_from_flags(mercy_flag, ch->specials.mercy));
    write_char_field_f("CHAR", ref, "AFLAGS", "%s",
                       str_from_flags(affect_flag, ch->specials.affected_by));
    write_char_field_f("CHAR", ref, "UID", "%d", ch->specials.uid);
    write_char_field_f("CHAR", ref, "GROUP", "%d", ch->specials.group);
    write_char_field_f("CHAR", ref, "POOFIN", "%s", ch->player.poofin);
    write_char_field_f("CHAR", ref, "POOFOUT", "%s", ch->player.poofout);
    write_char_field_f("CHAR", ref, "PROMPT", "%s", ch->player.prompt);
    for (i = 0; i < MAX_ALIAS; i++) {
        sprintf(buf, "ALIAS %d", i);
        write_char_field_f("CHAR", ref, buf, "%s %s", ch->alias[i].alias, ch->alias[i].text);
    }

    /* shhlog("pfile_write:str");   */

    write_char_field_f("CHAR", ref, "STR", "%d", ch->abilities.str);
    write_char_field_f("CHAR", ref, "END", "%d", ch->abilities.end);
    write_char_field_f("CHAR", ref, "AGL", "%d", ch->abilities.agl);
    write_char_field_f("CHAR", ref, "WIS", "%d", ch->abilities.wis);

    write_char_field_f("CHAR", ref, "MANA", "%d", ch->points.mana);
    write_char_field_f("CHAR", ref, "HITS", "%d", ch->points.hit);
    write_char_field_f("CHAR", ref, "MAX HITS", "%d", ch->points.max_hit);
    write_char_field_f("CHAR", ref, "STUN", "%d", ch->points.stun);
    write_char_field_f("CHAR", ref, "MAX STUN", "%d", ch->points.max_stun);
    write_char_field_f("CHAR", ref, "MOVES", "%d", ch->points.move);
    write_char_field_f("CHAR", ref, "ARMOR", "%d", ch->tmpabilities.armor);
    write_char_field_f("CHAR", ref, "OBSIDIAN", "%d", ch->points.obsidian);
    write_char_field_f("CHAR", ref, "IN BANK", "%d", ch->points.in_bank);
    write_char_field_f("CHAR", ref, "OFFENSE", "%d", ch->tmpabilities.off);
    write_char_field_f("CHAR", ref, "OFFENSE LAST GAIN", "%d", (int) ch->abilities.off_last_gain);
    write_char_field_f("CHAR", ref, "DEFENSE", "%d", ch->tmpabilities.def);
    write_char_field_f("CHAR", ref, "DEFENSE LAST GAIN", "%d", (int) ch->abilities.def_last_gain);
    write_char_field_f("CHAR", ref, "DAMAGE", "%d", ch->tmpabilities.damroll);
    write_char_field_f("CHAR", ref, "INVIS LEVEL", "%d", ch->specials.il);
    write_char_field_f("CHAR", ref, "QUIET LEVEL", "%d", ch->specials.quiet_level);


    /* shhlog("pfile_write:room number");   */

    if (ch->in_room) {
        write_char_field_f("CHAR", ref, "LOAD ROOM", "%d", ch->in_room->number);
    } else {
        write_char_field_f("CHAR", ref, "LOAD ROOM", "%d", 1003);
    }

    write_char_field_f("CHAR", ref, "LAST LOGON", "%d", (int) ch->player.time.logon);
    write_char_field_f("CHAR", ref, "STARTING TIME", "%d", (int) ch->player.time.starting_time);
    write_char_field_f("CHAR", ref, "STARTING AGE", "%d", ch->player.time.starting_age);
    write_char_field_f("CHAR", ref, "AGE", "%d", GET_AGE(ch));
    write_char_field_f("CHAR", ref, "PLAYING TIME", "%d",
                       ch->player.time.played + (int) (time(0) - ch->player.time.logon));

    /* shhlog("pfile_write:skills");   */
    write_char_skills(ref, ch);

    write_char_affects(ref, ch->affected);

    /* shhlog("pfile_write:edescs");   */
    write_char_edescs(ref, ch->ex_description);

    write_char_programs("CHAR", ref, ch);

    /* shhlog("pfile_write:affects");   */
    /* Add back the affects we pulled earlier */
    for (af = ch->affected; af != NULL; af = af->next) 
        affect_modify(ch, af->location, af->modifier, af->bitvector, TRUE);

    /* shhlog("pfile_write:equp");   */

    for (i = 0; i < MAX_WEAR; i++) {
        if (char_eq[i]
            )
            equip_char(ch, char_eq[i], i);
    }

    affect_total(ch);

    /* shhlog("pfile_write:objects");   */
    write_char_objects(ch);
    /* shhlog("pfile_write:mounts");   */
    write_char_mounts(ch);

    /* shhlog("pfile_write:writing");   */
    write_char_file();

    close_char_file();
    /* shhlog("pfile_write:done");   */

    return 1;
}

int
save_char_text(CHAR_DATA * ch)
{
    int result;
    char buf[256];

    sprintf(buf, "%s/%s", ACCOUNT_DIR, ch->account);

    result = save_char_text_dir(ch, buf);

    return result;
}

/* end character saving routines */

/* begin higher-level character loading routines */

/* return vector integer from string of flags as written by 
   string_from_flags */
int
vector_from_flags(struct flag_data *flag, char *str)
{

    int i;
    int vector = 0;
    char *pos;

    /* cycle through the flag names, looking for each one in the given str */
    for (i = 0; strlen(flag[i].name); i++) {
        pos = str;
        /* while we keep finding substrings that match flag[i].name */
        while ((pos = strstr(pos, flag[i].name))) {
            /* if the subtring is immediately surrounded by '/'s */
            if (((pos == str) || (*(pos - 2) == '/'))
                && (*(pos + strlen(flag[i].name) + 1) == '/')) {
                vector += flag[i].bit;
                pos = str + strlen(str);
            } else
                pos++;
        }
    }

    return vector;
}

/* return the integer corresponding to a string attribute */
int
int_from_attrib(struct attribute_data *att, char *str)
{

    int i;

    for (i = 0; strlen(att[i].name) && strcmp(att[i].name, str); i++);

    return att[i].val;
}

void
link_text_obj(OBJ_DATA * obj, char *location, int ref)
{

    int i;

    CHAR_DATA *ch;
    OBJ_DATA *obj_in;

    if (!strcmp(location, "content")) {
        if (!(obj_in = obj_from_ref(ref))) {
            gamelogf("link_text_obj: Unable to find obj for ref %d", ref);
            obj_to_char(obj, char_from_ref(1));
            return;
        } else
            obj_to_obj(obj, obj_in);

        return;
    }

    if (!(ch = char_from_ref(ref))) {
        gamelogf("link_text_obj: Unable to find char for ref %d", ref);
        obj_to_char(obj, char_from_ref(1));
        return;
    }

    for (i = 0; (i < MAX_WEAR) && (strcmp(wear_name[i], location)); i++);


    if (i < MAX_WEAR) {
        equip_char(ch, obj, i);
    } else if (!strcmp(location, "inventory"))
        obj_to_char(obj, ch);
    else {
        gamelogf("link_text_obj: Unknown object location %s", location);
        obj_to_char(obj, ch);
    }

    return;
}

/* reads obj extra descs*/
void
read_obj_edescs(char *label, int ref)
{
    int i = 0;
    int read = 1;
    char field[64];
    char *tmpkey = 0, *tmpdesc = 0;
    OBJ_DATA *obj;

    if (!(obj = obj_from_ref(ref))) {
        gamelogf("read_obj_edescs: Unable to find obj for ref %d", ref);
        return;
    }

    free_edesc_list(obj->ex_description);
    obj->ex_description = NULL;

    while (read) {
        sprintf(field, "EXTRA_DESC_KEYWORDS %d", i);
        read = read_char_field_f(label, ref, field, "%s", &tmpkey);

        if (read) {
            sprintf(field, "EXTRA_DESC_DESCRIPTION %d", i);
            read = read_char_field_f(label, ref, field, "%s", &tmpdesc);
            set_obj_extra_desc_value(obj, tmpkey, tmpdesc);
        }
        free(tmpkey); tmpkey = 0;
        free(tmpdesc); tmpdesc = 0;
        i++;
    }

    return;
}

/* reads object affects */
void
read_obj_affects(int ref)
{

    int i = 0;
    int read = 1;
    int modifier;
    char field[64];
    char location[MAX_LINE_LENGTH];

    OBJ_DATA *obj;

    obj = obj_from_ref(ref);

    for (i = 0; i < MAX_OBJ_AFFECT; i++) {
        sprintf(field, "AFFECT %d", i);
        read = read_char_field_f("OBJECT", ref, field, "%a %d", location, &modifier);
        if (read) {
            int skillnum;

            obj->affected[i].modifier = modifier;
            obj->affected[i].location = 0;
            for (skillnum = 1; *skill_name[skillnum] != '\n'; skillnum++)
                if (!strcmp(skill_name[skillnum], location))
                    obj->affected[i].location = CHAR_APPLY_SKILL_OFFSET + skillnum;

            if (obj->affected[i].location == 0)
                obj->affected[i].location = int_from_attrib(obj_apply_types, location);

            /*
             * obj->affected[i].modifier = modifier;
             * obj->affected[i].location  = int_from_attrib(obj_apply_types, location);
             */
        } else {
            obj->affected[i].modifier = 0;
            obj->affected[i].location = 0;
        }
    }

    return;
}

/* read a single object */
int
read_char_obj(int ref)
{
    int i, i1, i2, i3;
    char buf[MAX_LINE_LENGTH];
    OBJ_DATA *obj;
    bool hooded = 0;
    int max_amount = 0;
    int max_points = 0;
    int csz, asz =  0;
    int light_flags = 0;
    bool bogus_obj = 0;

    if (!find_field("OBJECT", ref, "VIRTUAL NUMBER"))
        return 0;

    i = read_char_field_num("OBJECT", ref, "VIRTUAL NUMBER");

    /* It would be nice, at some point, to make it so the immortal
     * entering the world sees a message along the lines of "You were
     * carrying object (objectnumber), which had not been saved."
     */

    if (!(obj = read_object(i, VIRTUAL))) {
        errorlogf("read_char_obj: Unable to create obj num %d", i);
        CREATE(obj, OBJ_DATA, 1);
        clear_object(obj);
        bogus_obj = 1;
    }

    add_obj_to_table(obj);

    if ((obj->obj_flags.type == ITEM_WORN || obj->obj_flags.type == ITEM_CONTAINER)
        && IS_SET(obj->obj_flags.value[1], CONT_HOODED))
        hooded = 1;

    if (GET_ITEM_TYPE(obj) == ITEM_LIGHT) {
        light_flags = obj->obj_flags.value[5] & LIGHT_FLAGS;
        max_amount = obj->obj_flags.value[1];
    }

    if (GET_ITEM_TYPE(obj) == ITEM_ARMOR) {
        max_points = obj->obj_flags.value[3];
        asz = obj->obj_flags.value[2];
    }

    if (GET_ITEM_TYPE(obj) == ITEM_WORN) {
        asz = obj->obj_flags.value[0];
    }

    if (read_char_field_f("OBJECT", ref, "VALUES", "%d %d %d %d", &i, &i1, &i2, &i3)) {
        obj->obj_flags.value[0] = i;
        obj->obj_flags.value[1] = i1;
        obj->obj_flags.value[2] = i2;
        obj->obj_flags.value[3] = i3;
    }

    for (i = 0; i < 6; i++) {
        sprintf(buf, "VALUE %d", i);
        if (read_char_field_f("OBJECT", ref, buf, "%d", &i1))
            obj->obj_flags.value[i] = i1;
    }

    // after loading values, get the new size
    if (GET_ITEM_TYPE(obj) == ITEM_WORN)
        csz = obj->obj_flags.value[0];
    else if( GET_ITEM_TYPE(obj) == ITEM_ARMOR)
        csz = obj->obj_flags.value[2];

    if (GET_ITEM_TYPE(obj) == ITEM_ARMOR && obj->obj_flags.value[3] == 0) {
        obj->obj_flags.value[3] = max_points;
    }

    // adjust fields for size change
    if( asz > 0 && asz != csz ) {
        obj->obj_flags.weight = obj->obj_flags.weight * csz / asz;
    }

    if (hooded && !IS_SET(obj->obj_flags.value[1], CONT_HOODED))
        MUD_SET_BIT(obj->obj_flags.value[1], CONT_HOODED);

    // Restore light_flags that might've been wiped by reading "VALUE 5" above
    obj->obj_flags.value[5] |= light_flags; // light_flags will be 0, if this wasn't a light source item
    if (GET_ITEM_TYPE(obj) == ITEM_LIGHT && max_amount > 0 && obj->obj_flags.value[1] <= 0)
        obj->obj_flags.value[1] = max_amount;


    read_char_field_f("OBJECT", ref, "EXTRA FLAGS", "%a", buf);
    obj->obj_flags.extra_flags = vector_from_flags(obj_extra_flag, buf);
    read_char_field_f("OBJECT", ref, "CHAR FLAGS", "%a", buf);
    obj->obj_flags.bitvector = vector_from_flags(char_flag, buf);
    read_char_field_f("OBJECT", ref, "STATE FLAGS", "%a", buf);
    obj->obj_flags.state_flags = vector_from_flags(obj_state_flag, buf);
    read_char_field_f("OBJECT", ref, "COLOR FLAGS", "%a", buf);
    obj->obj_flags.color_flags = vector_from_flags(obj_color_flag, buf);
    read_char_field_f("OBJECT", ref, "ADVERBIAL FLAGS", "%a", buf);
    obj->obj_flags.adverbial_flags = vector_from_flags(obj_adverbial_flag, buf);

    read_obj_affects(ref);

#ifdef SAVE_OBJ_EDESCS
    read_obj_edescs("OBJECT", ref);
#endif

    // Remove anti_mortal flag from any existing objects
    if (IS_SET(obj->obj_flags.extra_flags, OFL_ANTI_MORT))
        REMOVE_BIT(obj->obj_flags.extra_flags, OFL_ANTI_MORT);

    if (obj->obj_flags.type == ITEM_NOTE)
        read_note(obj);

    /* do this last, so that the affects different from the standard ones will not screw up
     * characters 
     */
    read_char_field_f("OBJECT", ref, "LOCATION", "%a of %d", buf, &i);

    if (bogus_obj)
        free_obj(obj);
    else
        link_text_obj(obj, buf, i);
    return 1;
}

void
read_char_objects(void)
{

    int ref = 1;

    while (read_char_obj(ref++));

    return;
}

/* return the int that corresponds to the sex stored as a string */
int
read_char_sex(char *label, int ref)
{

    char buf[32];

    read_char_field_f(label, ref, "SEX", "%a", buf);
    if (!strcmp(buf, "male"))
        return SEX_MALE;
    else if (!strcmp(buf, "female"))
        return SEX_FEMALE;
    else
        return SEX_NEUTER;

}

/* return the int that corresponds to the race stored as a string */
int
read_char_race(char *label, int ref)
{

    int i;
    char buf[64];

    read_char_field_f(label, ref, "RACE", "%a", buf);
    for (i = MAX_RACES - 1; i > 0; i--)
        if (!strcmp(race[i].name, buf))
            break;

    return i;
}

/* reads character affects */
void
read_char_affects(char *label, int ref)
{

    int i = 0;
    int read = 1;
    int duration, modifier;
    char field[64];
    char location[MAX_LINE_LENGTH];
    char flags[MAX_LINE_LENGTH];
    char skill[MAX_LINE_LENGTH];
    char power[MAX_LINE_LENGTH];
    char magick_type[MAX_LINE_LENGTH];

    struct affected_type *af;
    CHAR_DATA *ch;

    if (!(ch = char_from_ref(ref))) {
        gamelogf("read_char_affects: Unable to find char for ref %d", ref);
        return;
    }
    ch->affected = NULL;

    /* read in the old affects */
    while (read) {
        sprintf(field, "AFFECT %d", i++);
        read =
            read_char_field_f(label, ref, field, "%d %d %a %a %a", &duration, &modifier, location,
                              flags, skill);
        if (read) {
            af = (struct affected_type *) malloc(sizeof *af);
            af->duration = duration;
            af->power = 0;
            af->magick_type = 0;
            af->modifier = modifier;
            af->location = int_from_attrib(obj_apply_types, location);
            af->bitvector = vector_from_flags(affect_flag, flags);
            af->type = old_search_block(skill, 0, strlen(skill), skill_name, 0) - 1;
            affect_to_char(ch, af);
            free(af);           /* Always clean up our malloc */
        }
    }                           /* DEPRICATED: This was a holdover during the conversion. I doubt there
                                 * are many PCs who have this style saved in their pfile. -Tiernan */

    /* now turn around and read in the new affects */
    read = TRUE;
    i = 0;
    while (read) {
        af = (struct affected_type *) malloc(sizeof *af);
        sprintf(field, "AFFECT_DURATION %d", i);
        read = read && read_char_field_f(label, ref, field, "%d", &duration);
        if (!read)
            duration = -1;

        af->duration = duration;

        sprintf(field, "AFFECT_MOD %d", i);
        read = read && read_char_field_f(label, ref, field, "%d", &modifier);
        if (!read)
            modifier = 0;
        af->modifier = modifier;

        sprintf(field, "AFFECT_LOC %d", i);
        read = read && read_char_field_f(label, ref, field, "%a", location);
        if (!read)
            location[0] = '\0';
        af->location = int_from_attrib(obj_apply_types, location);

        sprintf(field, "AFFECT_FLAGS %d", i);
        read = read && read_char_field_f(label, ref, field, "%a", flags);
        if (!read)
            flags[0] = '\0';
        af->bitvector = vector_from_flags(affect_flag, flags);

        sprintf(field, "AFFECT_SKILL %d", i);
        read = read_char_field_f(label, ref, field, "%a", skill);
        if (!read)
            skill[0] = '\0';
        af->type = old_search_block(skill, 0, strlen(skill), skill_name, 0) - 1;

        sprintf(field, "AFFECT_POWER %d", i);
        read = read_char_field_f(label, ref, field, "%a", power);
        if (!read)
            power[0] = '\0';
        af->power = old_search_block(power, 0, strlen(power), Power, 0);

        sprintf(field, "AFFECT_MAGICKTYPE %d", i);
        read = read_char_field_f(label, ref, field, "%a", magick_type);
        if (!read)
            magick_type[0] = '\0';
        af->magick_type = old_search_block(magick_type, 0, strlen(magick_type), MagickType, 0) - 1;

        if (read)
            affect_to_char(ch, af);
        free(af);               /* Always clean up our malloc */
        i++;
    }

    return;
}

/* reads character extra descs*/
void
read_char_edescs(char *label, int ref)
{
    int i = 0;
    int read = 1;
    char field[64];
    char keyword[MAX_LINE_LENGTH];
    char description[MAX_STRING_LENGTH];
    CHAR_DATA *ch;

    if (!(ch = char_from_ref(ref))) {
        gamelogf("read_char_affects: Unable to find char for ref %d", ref);
        return;
    }


    ch->ex_description = NULL;

    while (read) {

        sprintf(field, "EXTRA_DESC_KEYWORDS %d", i);
        read = read_char_field_f(label, ref, field, "%a", keyword);

        if (read) {
            sprintf(field, "EXTRA_DESC_DESCRIPTION %d", i);
            read = read_char_field_f(label, ref, field, "%a", description);
            set_char_extra_desc_value(ch, keyword, description);
        }
        i++;
    }

    return;
}

/* read a single mount and put it behind or under the character */
int
read_char_mnt(int ref)
{

    int i, tmpi;
    char buf[MAX_LINE_LENGTH];
    char buf2[MAX_LINE_LENGTH];
    char tmp[MAX_LINE_LENGTH];
    CHAR_DATA *mnt;
    CHAR_DATA *ch = current.ch;

    if (!ch->in_room) {
        gamelogf("read_char_mounts: Character %s is not in a room", MSTR(ch, name));
        return 0;
    }

    if (!find_field("CHAR", ref, "VIRTUAL NUMBER"))
        return 0;

    i = read_char_field_num("CHAR", ref, "VIRTUAL NUMBER");

    if (!(mnt = read_mobile(i, VIRTUAL))) {
        gamelogf("read_char_mount: Unable to create mount num %d", i);
        return 1;
    }

    add_char_to_table(mnt);

    char_to_room(mnt, ch->in_room);
    GET_POS(mnt) = POSITION_STANDING;
    read_char_field_f("CHAR", ref, "LOCATION", "%a", buf);
    if (!strcmp(buf, "mount"))
        ch->specials.riding = mnt;
    else
        add_follower(mnt, ch);

    if (mnt->player.extkwds)
        free(mnt->player.extkwds);
    read_char_field_f("CHAR", ref, "KEYWORDS", "%s", &mnt->player.extkwds);

    mnt->player.sex = read_char_sex("CHAR", ref);

    read_char_field_f("CHAR", ref, "ACCOUNT", "%s", &mnt->account);
    mnt->player.tribe = read_char_field_num("CHAR", ref, "TRIBE");

    /* read in the new clans */
    i = 0;
    sprintf(tmp, "CLAN %d", i);
    while (read_char_field_f("CHAR", 1, tmp, "%a %a %d", buf, buf2, &tmpi)) {
        CLAN_DATA *pClan, *pClan2;

        pClan = new_clan_data();
        pClan->clan = lookup_clan_by_name(buf);
        /* -2 signifies no clan# was found for the specified name, which implies
         * that the clanname IS the clan# (eg. tribe 77000) */
        if (pClan->clan == -1)
            pClan->clan = atoi(buf);
        pClan->flags = vector_from_flags(clan_job_flags, buf2);
        pClan->rank = MIN(tmpi, clan_ranks[pClan->clan].num_ranks);

        if (!mnt->clan) {
            pClan->next = mnt->clan;
            mnt->clan = pClan;
        } else {
            /* skip to end */
            for (pClan2 = mnt->clan; pClan2->next; pClan2 = pClan2->next);
            pClan->next = pClan2->next;
            pClan2->next = pClan;
        }

        i++;
        sprintf(tmp, "CLAN %d", i);
    }

    mnt->player.weight = read_char_field_num("CHAR", ref, "WEIGHT");
    mnt->player.height = read_char_field_num("CHAR", ref, "HEIGHT");

    mnt->specials.conditions[THIRST] = read_char_field_num("CHAR", ref, "THIRST");
    mnt->specials.conditions[FULL] = read_char_field_num("CHAR", ref, "FULL");
    mnt->specials.conditions[DRUNK] = read_char_field_num("CHAR", ref, "DRUNK");
    read_char_field_f("CHAR", ref, "CFLAGS", "%a", buf);
    mnt->specials.act = vector_from_flags(char_flag, buf);

    if (read_char_field_f("CHAR", ref, "BRIEF", "%a", buf))
        mnt->specials.brief = vector_from_flags(brief_flag, buf);
    else
        mnt->specials.brief = 0;

    /* convert them to the new system if old */
    if (!IS_TRIBE(mnt, mnt->player.tribe) && mnt->player.tribe) {
        add_clan(mnt, mnt->player.tribe);

        /* convert the CFL_CLANLEADER flag to CJOB_LEADER for mnt->tribe */
        if (IS_SET(mnt->specials.act, CFL_CLANLEADER))
            set_clan_flag(mnt->clan, mnt->player.tribe, CJOB_LEADER);
    }

    if (read_char_field_f("CHAR", ref, "AFLAGS", "%a", buf))
        mnt->specials.affected_by = vector_from_flags(affect_flag, buf);
    else
        mnt->specials.affected_by = 0;

    mnt->abilities.str = read_char_field_num("CHAR", ref, "STR");
    mnt->abilities.end = read_char_field_num("CHAR", ref, "END");
    mnt->abilities.agl = read_char_field_num("CHAR", ref, "AGL");
    mnt->abilities.wis = read_char_field_num("CHAR", ref, "WIS");

    mnt->points.mana = read_char_field_num("CHAR", ref, "MANA");
    mnt->points.hit = read_char_field_num("CHAR", ref, "HITS");
    mnt->points.max_hit = read_char_field_num("CHAR", ref, "MAX HITS");
    mnt->points.stun = read_char_field_num("CHAR", ref, "STUN");
    mnt->points.max_stun = read_char_field_num("CHAR", ref, "MAX STUN");
    mnt->points.move = read_char_field_num("CHAR", ref, "MOVES");

    read_char_affects("CHAR", ref);

    /* Now setting this for all mobiles on a need only basis -Hal
     * if (IS_SET(mnt->specials.act, CFL_MOUNT))
     * new_event(EVNT_REST, (PULSE_MOBILE + number(-10, 10)) * 4, mnt, 0, 0,0,0);
     */
    return 1;
}


/* read all of the mounts */
void
read_char_mounts(void)
{

    int ref = 2;

    /* HACK ALERT - put in for testing 
     * while (current.ch->followers)
     * extract_char(current.ch->followers->follower);
     * if (ch->specials.riding)
     * extract_char(current.ch->specials.riding); */

    while (read_char_mnt(ref++));

    return;
}

/* reads skills */
void
read_char_skills(char *label, int ref)
{

    char name[64];
    int i, taught, learned, rel_lev, last_gain;
    CHAR_DATA *ch;

    if (!(ch = char_from_ref(ref))) {
        gamelogf("read_char_skills::Unable to find char ref %d", ref);
        return;
    }

    for (i = 0; i < MAX_SKILLS; i++) {
        sprintf(name, "SKILL %s", skill_name[i]);
        if (read_char_field_f
            (label, ref, name, "%d %d %d %d", &learned, &rel_lev, &taught, &last_gain)) {
            ch->skills[i] = (struct char_skill_data *) malloc(sizeof *ch->skills[i]);
            ch->skills[i]->learned = learned;
            ch->skills[i]->rel_lev = rel_lev;
            ch->skills[i]->taught_flags = taught;
            ch->skills[i]->last_gain = last_gain;
        }
    };

    return;
}

void
read_char_programs(char *label, int ref)
{

    int i;
    char ptype[256], pcmd[256], pname[256], field[80];
    CHAR_DATA *ch;

    if (!(ch = char_from_ref(ref))) {
        gamelogf("read_char_programs::Unable to find char ref %d", ref);
        return;
    }

    ch->specials.programs = NULL;
    for (i = 0;; i++) {
        sprintf(field, "PROGRAM %d", i);
        if (!read_char_field_f(label, ref, field, "%a %a %a", ptype, pcmd, pname))
            break;

        snprintf(field, sizeof(field), "%s %s %s", ptype, pcmd, pname);
        ch->specials.programs = set_prog(NULL, ch->specials.programs, field);
    }
}

void
init_char_vars(CHAR_DATA * ch)
{

    int i;
    char buf[256];
    char cwd[256];

    ch->tmpabilities = ch->abilities;

    ch->specials.carry_items = 0;

    stop_watching_raw(ch);

    ch->specials.dir_guarding = -1;
    ch->specials.guarding = (CHAR_DATA *) 0;

    // FIXME:  None of this zero-ing out nonsense should ever be necessary, the
    // char_data was allocated using CREATE, which is calloc().
    ch->specials.harnessed_to = (OBJ_DATA *) 0;
    ch->specials.obj_guarding = (OBJ_DATA *) 0;
    ch->obj_master = NULL;
    ch->guards = 0;
    ch->jsobj = 0;


    /* Commenting out since they are read in from the pfile and may already
     * have some affects which have altered their values. -Tiernan 4/26/03
     ch->tmpabilities.armor = race[(int)GET_RACE(ch)].natural_armor;
     ch->abilities.armor = race[(int)GET_RACE(ch)].natural_armor;
     ch->tmpabilities.damroll = 0;
     ch->abilities.damroll = 0;
     */


    if (ch->desc) {
        getcwd(cwd, 256);
        if (GET_NAME(ch)) {
            for (i = 0; *(GET_NAME(ch) + i); i++)
                buf[i] = tolower(*(GET_NAME(ch) + i));
            buf[i] = '\0';

            if (IS_IMMORTAL(ch))
                sprintf(ch->desc->path, "%s/immortals/users/%s", cwd, buf);
            else
                strcpy(ch->desc->path, "");
        } else if (IS_IMMORTAL(ch))
            sprintf(ch->desc->path, "%s/immortals/users/%s", cwd, buf);
        else
            strcpy(ch->desc->path, "");
    }

    ch->long_descr = NULL;

    // must age stats before resetting login time
    age_all_stats(ch, -1);

    // update drunkeness for time off the game
    gain_condition(ch, DRUNK, -mud_hours_passed(ch->player.time.logon, time(0)));

    // ok, now we can set the logon time
    ch->player.time.logon = time(0);

    ch->in_room = (struct room_data *) 0;

    ch->specials.act_wait = 0;
    init_saves(ch);

    affect_total(ch);
}


int
load_char_text(char *name, CHAR_DATA * ch)
{
    int i, tmpi, j;
    int temp_age;
    char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    char tmp[MAX_STRING_LENGTH];
    char field[128];
    int ref;

    ch->name = strdup(name);

    if (ch->desc)
        sprintf(buf, "%s/%s", ACCOUNT_DIR, ch->desc->player_info->name);
    else
        sprintf(buf, "%s/%s", ACCOUNT_DIR, ch->player.info[0]);

    if (!open_char_file(ch, buf))
        return -1;

    if (!read_char_file()) {
        close_char_file();
        return -2;
    }

    ref = add_char_to_table(ch);
    ch->player.luck = read_char_field_num("CHAR", 1, "LUCK");
    free(ch->name);
    read_char_field_f("CHAR", 1, "NAME", "%s", &ch->name);
    read_char_field_f("CHAR", 1, "SDESC", "%s", &ch->short_descr);
    read_char_field_f("CHAR", 1, "DESC", "%s", &ch->description);
    read_char_field_f("CHAR", 1, "TDESC", "%s", &ch->temp_description);
    read_char_field_f("CHAR", 1, "KEYWORDS", "%s", &ch->player.extkwds);
    read_char_field_f("CHAR", 1, "PREVIOUS CHARACTER", "%s", &ch->player.dead);

    read_char_field_f("CHAR", 1, "ACCOUNT", "%s", &ch->account);

    if (ch->player.dead == NULL) {
        ch->player.dead = strdup("born");
    }

    for (i = 0; i < 15; i++) {
        sprintf(field, "INFO %d", i);
        read_char_field_f("CHAR", 1, field, "%s", &ch->player.info[i]);
    }

    ch->pagelength = 0;
    ch->pagelength = read_char_field_num("CHAR", 1, "PAGELEN");

    if (ch->pagelength == 0 && !ch->desc)
        ch->pagelength = 24;
    else if (ch->pagelength == 0 && ch->desc)
        ch->pagelength = ch->desc->pagelength;

    read_char_field_f("CHAR", 1, "BACKGROUND", "%s", &ch->background);

    ch->application_status = APPLICATION_NONE;
    ch->application_status = read_char_field_num("CHAR", 1, "APPLICATIONSTATUS");

    read_char_field_f("CHAR", 1, "DENY", "%s", &ch->player.deny);

    ch->player.sex = read_char_sex("CHAR", 1);

    read_char_field_f("CHAR", 1, "GUILD", "%a", buf);
    for (i = MAX_GUILDS - 1; i > 0; i--)
        if (!strcmp(guild[i].name, buf))
            break;
    ch->player.guild = i;

    read_char_field_f("CHAR", 1, "SUB-GUILD", "%a", buf);
    for (j = MAX_SUB_GUILDS - 1; j > 0; j--)
        if (!strcmp(sub_guild[j].name, buf))
            break;
    ch->player.sub_guild = j;

    ch->player.race = read_char_race("CHAR", 1);

    read_char_field_f("CHAR", 1, "LEVEL", "%a", buf);
    ch->player.level = old_search_block(buf, 0, strlen(buf), level_name, 0) - 1;

    /* Read in granted commands */
    i = 0;
    sprintf(tmp, "GCMD%d", i);
    while (read_char_field_f("CHAR", 1, tmp, "%a", buf)) {
        EXTRA_CMD *pCmd;

        CREATE(pCmd, EXTRA_CMD, 1);
        pCmd->cmd = get_command(buf);
        sprintf(tmp, "GCMDL%d", i);
        pCmd->level = read_char_field_num("CHAR", 1, tmp);
        pCmd->next = ch->granted_commands;
        ch->granted_commands = pCmd;

        i++;
        sprintf(tmp, "GCMD%d", i);
    }

    /* Read in revoked commands */
    i = 0;
    sprintf(tmp, "RCMD%d", i);
    while (read_char_field_f("CHAR", 1, tmp, "%a", buf)) {
        EXTRA_CMD *pCmd;

        CREATE(pCmd, EXTRA_CMD, 1);
        pCmd->cmd = get_command(buf);
        sprintf(tmp, "RCMDL%d", i);
        pCmd->level = read_char_field_num("CHAR", 1, tmp);
        pCmd->next = ch->revoked_commands;
        ch->revoked_commands = pCmd;
        i++;
        sprintf(tmp, "RCMD%d", i);
    }

    read_char_field_f("CHAR", 1, "HOMETOWN", "%a", buf);
    for (i = MAX_CITIES - 1; i > 0; i--)
        if (city[i].name && !strcmp(city[i].name, buf))
            break;
    ch->player.hometown = i;

    if (read_char_field_f("CHAR", 1, "STARTCITY", "%a", buf)) {
        for (i = MAX_CITIES - 1; i > 0; i--)
            if (city[i].name && !strcmp(city[i].name, buf))
                break;
        ch->player.start_city = i;
    } else {
        for (i = MAX_CITIES - 1; i > 0; i--)
            if (city[i].name && !strcmp(city[i].name, buf))
                break;
        ch->player.hometown = i;
    }

    read_char_field_f("CHAR", 1, "CLAN", "%a", buf);
    ch->player.tribe = lookup_clan_by_name(buf);

    /* read in the clans */
    i = 0;
    sprintf(tmp, "CLAN %d", i);

    while (read_char_field_f("CHAR", 1, tmp, "%a %a %d", buf, buf2, &tmpi)) {
        CLAN_DATA *pClan, *pClan2;

        pClan = new_clan_data();
        pClan->clan = lookup_clan_by_name(buf);
        /* -2 signifies no clan# was found for the specified name, which implies
         * that the clanname IS the clan# (eg. tribe 77000) */
        if (pClan->clan == -1)
            pClan->clan = atoi(buf);
        pClan->flags = vector_from_flags(clan_job_flags, buf2);
        pClan->rank = MIN(tmpi, clan_ranks[pClan->clan].num_ranks);

        if (!ch->clan) {
            pClan->next = ch->clan;
            ch->clan = pClan;
        } else {
            /* skip to end */
            for (pClan2 = ch->clan; pClan2->next; pClan2 = pClan2->next);
            pClan->next = pClan2->next;
            pClan2->next = pClan;
        }

        i++;
        sprintf(tmp, "CLAN %d", i);
    }

    ch->player.luck = read_char_field_num("CHAR", 1, "LUCK");
    ch->player.weight = read_char_field_num("CHAR", 1, "WEIGHT");
    ch->player.height = read_char_field_num("CHAR", 1, "HEIGHT");
    read_char_field_f("CHAR", 1, "LANGUAGE", "%a", buf);

    ch->specials.language = old_search_block(buf, 0, strlen(buf), skill_name, 0) - 1;

    /* Because of the "old_search_block" we'll -always- get a skill # instead
     * of the index to the table, so we need to do this all the time */
    for (i = 0; i < MAX_TONGUES; i++)
        if (language_table[i].spoken == (int) ch->specials.language)
            ch->specials.language = i;

    /* If, for any reason, the index is outside the range of the table, set it
     * to 0 (sirihish) */
    if (ch->specials.language > MAX_TONGUES || ch->specials.language < 0)
        ch->specials.language = 0;

    buf[0] = '\0';
    read_char_field_f("CHAR", 1, "ACCENT", "%a", buf);
    ch->specials.accent = old_search_block(buf, 0, strlen(buf), skill_name, 0) - 1;

    if (ch->specials.accent < 0)
        ch->specials.accent = LANG_NO_ACCENT;

    ch->specials.conditions[THIRST] = read_char_field_num("CHAR", 1, "THIRST");
    ch->specials.conditions[FULL] = read_char_field_num("CHAR", 1, "FULL");
    ch->specials.conditions[DRUNK] = read_char_field_num("CHAR", 1, "DRUNK");
    ch->specials.eco = read_char_field_num("CHAR", 1, "ECO");

    read_char_field_f("CHAR", 1, "CFLAGS", "%a", buf);
    ch->specials.act = vector_from_flags(char_flag, buf);

    read_char_field_f("CHAR", 1, "QFLAGS", "%a", buf);
    ch->quit_flags = vector_from_flags(quit_flag, buf);

    if (read_char_field_f("CHAR", 1, "BRIEF", "%a", buf)) {
        ch->specials.brief = vector_from_flags(brief_flag, buf);
    } else {
        ch->specials.brief = 0;
    }

    // kill brief the cflag
    if (IS_SET(ch->specials.act, CFL_BRIEF)) {
        MUD_SET_BIT(ch->specials.brief, BRIEF_ROOM);
        REMOVE_BIT(ch->specials.act, CFL_BRIEF);
    }

    if (read_char_field_f("CHAR", 1, "NOSAVE", "%a", buf)) {
        ch->specials.nosave = vector_from_flags(nosave_flag, buf);
    } else {
        ch->specials.nosave = 0;
    }

    if (read_char_field_f("CHAR", 1, "MERCY", "%a", buf)) {
        ch->specials.mercy = vector_from_flags(mercy_flag, buf);
    } else {
        char *message = find_ex_description("[MERCY_FLAG]", ch->ex_description, TRUE);
        ch->specials.mercy = 0;
        if (message) {
           ch->specials.mercy = 1;
        }
    }


    // kill brief the cflag
    if (IS_SET(ch->specials.act, CFL_NOSAVE)) {
        MUD_SET_BIT(ch->specials.nosave, NOSAVE_ARREST);
        REMOVE_BIT(ch->specials.act, CFL_NOSAVE);
    }

    /* convert them to the new system if old */
    if (!IS_TRIBE(ch, ch->player.tribe) && ch->player.tribe) {
        add_clan(ch, ch->player.tribe);
        /* convert the CFL_CLANLEADER flag to CJOB_LEADER for ch->tribe */
        if (IS_SET(ch->specials.act, CFL_CLANLEADER))
            set_clan_flag(ch->clan, ch->player.tribe, CJOB_LEADER);
    }

    if (read_char_field_f("CHAR", 1, "AFLAGS", "%a", buf))
        ch->specials.affected_by = vector_from_flags(affect_flag, buf);
    else
        ch->specials.affected_by = 0;

    ch->specials.uid = read_char_field_num("CHAR", 1, "UID");
    ch->specials.group = read_char_field_num("CHAR", 1, "GROUP");
    read_char_field_f("CHAR", 1, "POOFIN", "%s", &ch->player.poofin);
    read_char_field_f("CHAR", 1, "POOFOUT", "%s", &ch->player.poofout);
    read_char_field_f("CHAR", 1, "PROMPT", "%s", &ch->player.prompt);

    for (i = 0; i < MAX_ALIAS; i++) {
        sprintf(field, "ALIAS %d", i);
        read_char_field_f("CHAR", 1, field, "%a %a", ch->alias[i].alias, ch->alias[i].text);
    }

    ch->abilities.str = read_char_field_num("CHAR", 1, "STR");
    ch->abilities.end = read_char_field_num("CHAR", 1, "END");
    ch->abilities.agl = read_char_field_num("CHAR", 1, "AGL");
    ch->abilities.wis = read_char_field_num("CHAR", 1, "WIS");

    ch->points.mana = read_char_field_num("CHAR", 1, "MANA");
    ch->points.hit = read_char_field_num("CHAR", 1, "HITS");
    ch->points.max_hit = read_char_field_num("CHAR", 1, "MAX HITS");
    ch->points.stun = read_char_field_num("CHAR", 1, "STUN");
    ch->points.max_stun = read_char_field_num("CHAR", 1, "MAX STUN");
    ch->points.move = read_char_field_num("CHAR", 1, "MOVES");
    ch->points.obsidian = read_char_field_num("CHAR", 1, "OBSIDIAN");
    ch->points.in_bank = read_char_field_num("CHAR", 1, "IN BANK");

    ch->specials.il = read_char_field_num("CHAR", 1, "INVIS LEVEL");
    ch->specials.quiet_level = read_char_field_num("CHAR", 1, "QUIET LEVEL");


    ch->tmpabilities.off = ch->abilities.off = read_char_field_num("CHAR", 1, "OFFENSE");
    ch->tmpabilities.off_last_gain = ch->abilities.off_last_gain = read_char_field_num("CHAR", 1, "OFFENSE LAST GAIN");
    ch->tmpabilities.def = ch->abilities.def = read_char_field_num("CHAR", 1, "DEFENSE");
    ch->tmpabilities.def_last_gain = ch->abilities.def_last_gain = read_char_field_num("CHAR", 1, "DEFENSE LAST GAIN");
    ch->tmpabilities.damroll = ch->abilities.damroll = read_char_field_num("CHAR", 1, "DAMAGE");
    ch->tmpabilities.armor = ch->abilities.armor = read_char_field_num("CHAR", 1, "ARMOR");


    ch->specials.was_in_room = get_room_num(read_char_field_num("CHAR", 1, "LOAD ROOM"));
    ch->player.time.logon = read_char_field_num("CHAR", 1, "LAST LOGON");
    ch->player.time.starting_time = read_char_field_num("CHAR", 1, "STARTING TIME");
    ch->player.time.starting_age = read_char_field_num("CHAR", 1, "STARTING AGE");

    temp_age = read_char_field_num("CHAR", 1, "AGE");

    ch->player.time.played = read_char_field_num("CHAR", 1, "PLAYING TIME");

    read_char_affects("CHAR", ref);

#define READ_BACK_IN_EDESCS 1
#ifdef READ_BACK_IN_EDESCS
    read_char_edescs("CHAR", ref);
#endif

    read_char_skills("CHAR", ref);

    read_char_programs("CHAR", ref);

    init_char_vars(ch);


    /* check to see if they aged since the last login */

    if (temp_age) {
        for (; temp_age < GET_AGE(ch); temp_age++) {
            /* Azroen, do whatever you need to do here
             * to age the character
             * -Tenebrius */
            //shhlog("ageing a character");
        }

    };


    close_char_file();

    // Hook to load in character info from an XML file
    load_character_file_old(name, ch);

    return 0;
}

int
load_char(char *name, CHAR_DATA *ch) {
   int result = load_char_text(name, ch);

   // if file load was ok
   if (result == 0) {
      // go ahead and load from the db
//      load_char_db(name, ch);
   }

   return result;
}

int
load_char_mnts_and_objs(CHAR_DATA * ch)
{

    char buf[MAX_STRING_LENGTH];

    if (ch->desc)
        sprintf(buf, "%s/%s", ACCOUNT_DIR, ch->desc->player_info->name);
    else
        sprintf(buf, "%s/%s", ACCOUNT_DIR, ch->account);

    if (!open_char_file(ch, buf))
        return -1;

    if (!read_char_file()) {
        close_char_file();
        return -1;
    }

    add_char_to_table(ch);

    read_char_mounts();
    read_char_objects();

    close_char_file();

    return 0;
}


/* end high-level character loading routines */


/* begin high-level character field routines */

int
get_char_file_password(char *name, char *account, char *pass)
{
    CHAR_DATA ch;
    char buf[MAX_STRING_LENGTH];

    ch.name = name;

    sprintf(buf, "%s/%s", ACCOUNT_DIR, account);
    if (!open_char_file(&ch, buf))
        return 0;

    if (!read_char_file()) {
        close_char_file();
        return 0;
    }

    add_char_to_table(&ch);

    read_char_field_f("CHAR", 1, "PASSWORD", "%a", pass);

    close_char_file();

    return 1;
}

char *
get_char_file_pinfo(CHAR_DATA * ch, int num)
{
    char *pinfo;
    char field[256];

    char buf[MAX_STRING_LENGTH];

    if (!ch->name || !*ch->name) {
        gamelogf("get_char_file_pinfo: Tried to read nameless character");
        return 0;
    }

    if (!ch->desc)
        return NULL;
    else if (!ch->desc->player_info)
        return NULL;
    else if (!ch->desc->player_info->name)
        return NULL;

    sprintf(buf, "%s/%s", ACCOUNT_DIR, ch->desc->player_info->name);

    if (!open_char_file(ch, buf))
        return 0;

    if (!read_char_file()) {
        close_char_file();
        return 0;
    }

    add_char_to_table(ch);


    snprintf(field, sizeof(field), "INFO %d", num);
    read_char_field_f("CHAR", 1, field, "%s", &pinfo);

    close_char_file();

    return pinfo;
}

char *
save_char_file_pinfo(CHAR_DATA * ch, int num, char *pinfo)
{

    char field[256];

    char buf[256];

    if (!ch->name || !*ch->name) {
        gamelogf("save_char_file_pinfo: tried to save nameless character");
        return NULL;
    }

    if (!ch->desc)
        return NULL;
    else if (!ch->desc->player_info)
        return NULL;
    else if (!ch->desc->player_info->name)
        return NULL;

    sprintf(buf, "%s/%s", ACCOUNT_DIR, ch->desc->player_info->name);

    if (!open_char_file(ch, buf))
        return 0;
    if (!read_char_file()) {
        close_char_file();
        return 0;
    }

    add_char_to_table(ch);

    snprintf(field, sizeof(field), "INFO %d", num);
    write_char_field_f("CHAR", 1, field, "%s", pinfo);

    write_char_file();
    close_char_file();

    return pinfo;
}

