/*
 * File: MCREATION.C
 * Usage: Routines and commands for online mobile creation.
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
#include <unistd.h>

#include "constants.h"
#include "structs.h"
#include "utils.h"
#include "utility.h"
#include "comm.h"
#include "parser.h"
#include "handler.h"
#include "db.h"
#include "skills.h"
#include "limits.h"
#include "guilds.h"
#include "clan.h"
#include "cities.h"
#include "event.h"
#include "modify.h"
#include "creation.h"
#include "player_accounts.h"
#include "watch.h"
#include "reporting.h"
#include "info.h"
#include "special.h"

int add_new_npc_in_creation(int virt_nr);

extern int get_max_condition(CHAR_DATA * ch, int cond);
extern void add_to_hates(struct char_data *ch, struct char_data *mem);
extern void add_to_fears(struct char_data *ch, struct char_data *mem);
extern void add_to_likes(struct char_data *ch, struct char_data *mem);
extern bool does_hate(struct char_data *ch, struct char_data *mem);
extern bool does_fear(struct char_data * ch, struct char_data * mem);
extern bool does_like(struct char_data * ch, struct char_data * mem);
extern void remove_from_hates(struct char_data *ch, struct char_data *mem);
extern void remove_from_fears(struct char_data *ch, struct char_data *mem);
extern void remove_from_likes(struct char_data *ch, struct char_data *mem);

void
set_mskillflag(CHAR_DATA * ch, CHAR_DATA * vict, const char *buf)
{
    char flagname[MAX_INPUT_LENGTH];
    char skillname[MAX_INPUT_LENGTH];
    int loc;
    long flag;
    int sk;
    char tmp[200];

    if (!*buf) {
        send_to_char("Usage: cset <char> skillflag <flag> skills\n\rflags are:\n\r", ch);
        send_flags_to_char(skill_flag, ch);
        return;
    }
    buf = one_argument(buf, flagname, sizeof(flagname));
    strcpy(skillname, buf);

    loc = get_flag_num(skill_flag, flagname);
    if (loc == -1) {
        send_to_char("That flag doesn't exist.\n\r", ch);
        return;
    }

    if (skill_flag[loc].priv > GET_LEVEL(ch)) {
        send_to_char("You are not high enough level to set that flag.\n\r", ch);
        return;
    }
    flag = affect_flag[loc].bit;

    /* search for skill */
    for (sk = 1; sk < MAX_SKILLS; sk++)
        if (!strnicmp(skill_name[sk], skillname, strlen(skillname)))
            break;
    if (sk == MAX_SKILLS) {
        send_to_char("No such skill of that name.\n\r", ch);
        return;
    }


    if (!has_skill(vict, sk))
        /*   if ( !vict->skills[sk] ) */
    {
        send_to_char("They do not have that skill.\n\r", ch);
        return;
    };

    if (IS_SET(vict->skills[sk]->taught_flags, (flag))) {
        sprintf(tmp, "You remove the '%s' skill flag on $N for skill %s.\n\r", skill_flag[loc].name,
                skill_name[sk]);
        REMOVE_BIT(vict->skills[sk]->taught_flags, ((flag)));
    } else {
        sprintf(tmp, "You set the '%s' affect flag on $N for skill %s.\n\r", skill_flag[loc].name,
                skill_name[sk]);
        MUD_SET_BIT(vict->skills[sk]->taught_flags, ((flag)));
    }

    act(tmp, FALSE, ch, 0, vict, TO_CHAR);


};


void
unset_mskillflag(CHAR_DATA * ch, CHAR_DATA * vict, char *keywds)
{
    send_to_char("not working just yet.\n\r", ch);
    return;
};

void
unset_medesc(CHAR_DATA * ch, CHAR_DATA * npc, const char *argument)
{
    char keywd[MAX_INPUT_LENGTH];

    argument = one_argument(argument, keywd, sizeof(keywd));

    if (!*keywd) {
        send_to_char("You must give a list of keywords.\n\r", ch);
        return;
    }

    if (!rem_char_extra_desc_value(npc, keywd)) {
        send_to_char("That extra description does not exist.\n\r", ch);
        return;
    }

    send_to_char("Done.\n\r", ch);
}

void
cmd_mcopy(struct char_data *ch, const char *argument, Command cmd, int count)
{
    char buf1[256], buf2[256];
    char buf[512];

    struct npc_default_data *source;
    struct npc_default_data *target;

    int vnum_source, vnum_target;
    int rnum_source, rnum_target;
    CLAN_DATA *clan;
    /*   int j;  */

    /*flag = FALSE, virt_nr, real_nr = 0, i; */

    SWITCH_BLOCK(ch);

    argument = two_arguments(argument, buf1, sizeof(buf1), buf2, sizeof(buf2));
    if (!*buf1) {
        send_to_char("Usage:mcopy <source#> <target#>\n\r", ch);
        return;
    }

    if (!*buf2) {
        send_to_char("Usage:mcopy <source#> <target#>\n\r", ch);
        return;
    }

    vnum_source = atoi(buf1);
    vnum_target = atoi(buf2);

    sprintf(buf, "Trying to copy from npc #%d to #%d.\n\r", vnum_source, vnum_target);

    send_to_char(buf, ch);


    // FIXME Xygax:  Should be vnum_target, OR should be MODE_LOAD ???  Or we need
    // a MODE_READ
    if (!has_privs_in_zone(ch, vnum_source / 1000, MODE_MODIFY)) {
        sprintf(buf, "You are not authorized to read item from zone %d.(MODE_MODIFY)\n\r",
                vnum_source / 1000);
        send_to_char(buf, ch);
        return;
    };

    if (!has_privs_in_zone(ch, vnum_target / 1000, MODE_CREATE)) {
        sprintf(buf, "You do not have string edit privileges in zone %d.(MODE_CREATE)\n\r",
                vnum_target / 100);
        send_to_char(buf, ch);
        return;
    };

    if (real_mobile(vnum_target) != -1) {
        sprintf(buf, "A mobile already exists with number %d.\n\r", vnum_target);
        send_to_char(buf, ch);
        return;
    };

    add_new_npc_in_creation(vnum_target);

    rnum_source = real_mobile(vnum_source);
    rnum_target = real_mobile(vnum_target);

    if (rnum_target < 0) {
        sprintf(buf, "A vnum didnt translate right for target. error");
        send_to_char(buf, ch);
    };

    if (rnum_source < 0) {
        sprintf(buf, "A vnum didnt translate right for source. error");
        send_to_char(buf, ch);
    };

    if (rnum_source < 0 || rnum_target < 0)
        return;

    target = &(npc_default[rnum_target]);
    source = &(npc_default[rnum_source]);

    if (target->name)
        free((char *) target->name);

    if (source->name)
        target->name = strdup(source->name);
    else
        target->name = strdup("");

    if (target->short_descr)
        free((char *) target->short_descr);

    if (source->short_descr)
        target->short_descr = strdup(source->short_descr);
    else
        target->short_descr = strdup("");

    if (target->long_descr)
        free((char *) target->long_descr);

    if (source->long_descr)
        target->long_descr = strdup(source->long_descr);
    else
        target->long_descr = strdup("");

    if (target->description)
        free((char *) target->description);

    if (source->description)
        target->description = strdup(source->description);
    else
        target->description = strdup("");

    if (target->background)
        free((char *) target->background);

    if (source->background)
        target->background = strdup(source->background);
    else
        target->background = strdup("");


    free_edesc_list(target->exdesc);
    target->exdesc = copy_edesc_list(source->exdesc);

    target->obsidian = source->obsidian;
    target->gp_mod = source->gp_mod;
    target->armor = source->armor;
    target->off = source->off;
    target->def = source->def;
    target->dam = source->dam;
    target->affected_by = source->affected_by;
    target->pos = source->pos;
    target->def_pos = source->def_pos;
    target->act = source->act;
    target->flag = source->flag;
    target->language = source->language;
    target->max_hit = source->max_hit;
    target->max_stun = source->max_stun;
    target->sex = source->sex;
    target->guild = source->guild;
    target->race = source->race;
    target->age = source->age;
    target->apparent_race = source->apparent_race;
    target->level = source->level;
    target->tribe = source->tribe;
    target->luck = source->luck;

    for (clan = source->clan; clan; clan = clan->next) {
        CLAN_DATA *pClan;

        pClan = new_clan_data();
        pClan->clan = clan->clan;
        pClan->rank = clan->rank;
        pClan->flags = clan->flags;
        pClan->next = target->clan;
        target->clan = pClan->next;
    }

    free_specials(target->programs);
    target->programs = copy_specials(source->programs);

    memcpy(&(target->skills[0]), &(source->skills[0]), sizeof(struct char_skill_data) * MAX_SKILLS);

    sprintf(buf, "Done copying from default of npc #%d to default of #%d.\n\r", vnum_source,
            vnum_target);

    send_to_char(buf, ch);
}

int
add_new_npc_in_creation(int virt_nr)
{
    int real_nr = alloc_npc(virt_nr, TRUE, TRUE);
    update_shopkeepers(real_nr);

    return (TRUE);
}

void
set_medesc(CHAR_DATA * ch, CHAR_DATA * vict, char *keywds)
{
    struct extra_descr_data *tmp, *newdesc;

    if (!*keywds) {
        send_to_char("You must give a list of keywords.\n\r", ch);
        return;
    }

    for (tmp = vict->ex_description; tmp; tmp = tmp->next)
        if (!stricmp(tmp->keyword, keywds))
            break;

    if (!tmp) {
        global_game_stats.edescs++;
        CREATE(newdesc, struct extra_descr_data, 1);
        newdesc->description = (char *) 0;
        newdesc->next = vict->ex_description;
        vict->ex_description = newdesc;
        newdesc->keyword = strdup(keywds);
    } else {
        newdesc = tmp;
    }

    act("$n has entered CREATING mode (updating character edesc).", TRUE, ch, 0, 0, TO_ROOM);

    if (newdesc->description && strlen(newdesc->description) > 0) {
        send_to_char("Continue editing description, use '.c' to clear.\n\r", ch);
        /* uncomment if this is causing problems - tenebrius  2006/04/01 */
        /*  #define DISABLE_LOG_EDITS HALASTER_MESSED_UP */
#ifdef DISABLE_LOG_EDITS
        string_append(ch, &newdesc->description, MAX_DESC);
#else
        {
            char d_str[MAX_STRING_LENGTH];
            sprintf(d_str, "Edesc '%s' on mobile", keywds);
            string_append_verbose(ch, &newdesc->description, MAX_DESC, d_str);
        }
#endif

    } else {
        send_to_char("Enter a new description.\n\r", ch);

        /* uncomment if this is causing problems - tenebrius  2006/04/01 */
        /*  #define DISABLE_LOG_EDITS HALASTER_MESSED_UP */
#ifdef DISABLE_LOG_EDITS
        string_edit(ch, &newdesc->description, MAX_DESC);
#else
        {
            char d_str[MAX_STRING_LENGTH];
            sprintf(d_str, "Edesc '%s' on mobile", keywds);
            string_edit_verbose(ch, &newdesc->description, MAX_DESC, d_str);
        }
#endif
    }
};

void
set_mbackground(CHAR_DATA * ch, CHAR_DATA * mob)
{
    /* if the npc/pc has a background at all, continue editing */
    if (MSTR(mob, background)) {
        /* if the current npc doesn't have a background, use the default */
        if (!mob->background && IS_NPC(mob))
            mob->background = strdup(npc_default[mob->nr].background);
    }

    if (mob->background && strlen(mob->background) > 0) {
        send_to_char("Continue editing background, use '.c' to clear.\n\r", ch);

        string_append(ch, &mob->background, MAX_NOTE_LENGTH);
    } else {
        send_to_char("Enter a new background.\n\r", ch);
        string_edit(ch, &mob->background, MAX_NOTE_LENGTH);
    }
}


/* create a mobile */
void
cmd_mcreate(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256];
    CHAR_DATA *npc;
    int virt_nr;

    /*
     * int flag = FALSE, virt_nr;
     * 
     * int  real_nr = 0, i, j, sk;
     * CHAR_DATA *tmpch;
     */

    argument = one_argument(argument, buf, sizeof(buf));

    if (0 == strlen(buf)) {
      send_to_char("Usage: mcreate <number>\n\r", ch);
      return;
    }

    if (!is_number(buf)) {
      cprintf(ch, "Usage: mcreate <number>\n\r'%s' is not a number.\n\r", buf);
      return;
    }

    virt_nr = atoi(buf);

    if (virt_nr < 0) {
        send_to_char("You can't do that.\n\r", ch);
        return;
    }

    if (!has_privs_in_zone(ch, virt_nr / 1000, MODE_CREATE)) {
        send_to_char("You do not have creation privs in that zone.\n\r", ch);
        return;
    }

    if (real_mobile(virt_nr) == -1) {
        if( IS_SWITCHED(ch) ) {
            cprintf(ch, "No such NPC #%d, can't create while switched.\n\r", virt_nr);
            return;
        }

        if (!has_privs_in_zone(ch, virt_nr / 1000, MODE_STR_EDIT)) {
            send_to_char("You do not have creation privs in that " "zone.\n\r", ch);
            return;
        }

        if (!add_new_npc_in_creation(virt_nr)) {
            send_to_char("Something failed in creating that mobile.\n\r", ch);
            return;
        }


    }
    if (!(npc = read_mobile(virt_nr, VIRTUAL))) {
        send_to_char("That NPC does not exist.\n\r", ch);
        return;
    }

    if( !IS_SWITCHED(ch) ) {
        act("You make a magickal gesture, and $N forms amidst a dust cloud.", TRUE, ch, 0, npc, TO_CHAR);
        act("$n makes a magickal gesture, and $N forms amidst a dust cloud.", TRUE, ch, 0, npc, TO_ROOM);
    }
    char_to_room(npc, ch->in_room);


}


/* give the mobile a name */
void
cmd_mname(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256];
    CHAR_DATA *mob;

    argument = one_argument(argument, buf, sizeof(buf));
    mob = get_char_room_vis(ch, buf);
    if (IS_NPC(ch))
        return;
    if (!mob) {
        send_to_char("There is no object of that name.\n\r", ch);
        return;
    }
    if (!IS_NPC(mob)) {
        send_to_char("That person is not an NPC.\n\r", ch);
        return;
    }
    if (!has_privs_in_zone(ch, npc_index[mob->nr].vnum / 1000, MODE_STR_EDIT)) {
        send_to_char("You do not have creation privs in that zone.\n\r", ch);
        return;
    }
    free(mob->name);
    mob->name = strdup(argument);
    send_to_char("Done.\n\r", ch);
}


/* describe the mobile */
void
set_mdesc(CHAR_DATA * ch, CHAR_DATA * mob)
{
    act("$n has entered CREATING mode (updating character description).", TRUE, ch, 0, 0, TO_ROOM);
    
    /* Load it up with the default description so it can be edited */
    if ( !mob->description  && npc_default[mob->nr].description )
      mob->description = strdup(npc_default[mob->nr].description);
    

    if (mob->description && strlen(mob->description) > 0) {
        send_to_char("Continue editing description, use '.c' to clear.\n\r", ch);
        string_append(ch, &(mob->description), MAX_DESC);
    } else {
        send_to_char("\n\rEnter a new description.\n\r", ch);
        string_edit(ch, &(mob->description), MAX_DESC);
    }
}

/* redescribe the mobile */
void
append_desc(CHAR_DATA * ch, char *text_to_append)
{
    char *old;
    int destroy_old;

    if (ch->description) {
        old = ch->description;
        destroy_old = TRUE;
    } else {
        old = MSTR(ch, description);
        destroy_old = FALSE;
    };

    CREATE(ch->description, char, strlen(text_to_append) + strlen(old) + 2);

    strcpy(ch->description, old);
    strcat(ch->description, text_to_append);

    if (destroy_old) {
        free(old);
    }
}

/* temporary describe the mobile */
void
set_mtdesc(CHAR_DATA * ch, CHAR_DATA * mob)
{
    act("$n has entered CREATING mode (updating character tdescription).", TRUE, ch, 0, 0, TO_ROOM);
    
    /* Load it up with the default description so it can be edited */
    if ( !mob->temp_description )
      mob->temp_description = strdup("");
    

    if (mob->temp_description && strlen(mob->temp_description) > 0) {
        send_to_char("Continue editing temporary description, use '.c' to clear.\n\r", ch);
        string_append(ch, &(mob->temp_description), MAX_DESC);
    } else {
        send_to_char("\n\rEnter a new temporary description.\n\r", ch);
        string_edit(ch, &(mob->temp_description), MAX_DESC);
    }
}

/* set flags for a mobile */
void
set_mcflag(CHAR_DATA * ch, CHAR_DATA * mob, char *buf2, bool set)
{
    char tmp[256];
    int loc;
    long flag;
    struct affected_type *af, *next_af;

    if (!*buf2) {
        send_to_char("You may set the following character flags:\n\r", ch);
        send_flags_to_char(char_flag, ch);
        return;
    }

    loc = get_flag_num(char_flag, buf2);
    if (loc == -1) {
        send_to_char("That flag doesn't exist.\n\r", ch);
        return;
    }
    if (GET_LEVEL(ch) < char_flag[loc].priv) {
        send_to_char("That flag doesn't exist.\n\r", ch);
        return;
    }

    flag = char_flag[loc].bit;

    if (IS_SET(mob->specials.act, flag)) {
        if (set) {
            sprintf(tmp, "$N already had the '%s' flag.\n\r", char_flag[loc].name);
        } else {
            sprintf(tmp, "You have removed the '%s' flag from $N.\n\r", char_flag[loc].name);

            REMOVE_BIT(mob->specials.act, flag);

            for (af = mob->affected; af; af = next_af) {
                next_af = af->next;

                if ((af->location == CHAR_APPLY_CFLAGS) && IS_SET(af->modifier, flag))
                    affect_remove(mob, af);

            }
        }
    } else if (set) {
        sprintf(tmp, "You have set the '%s' extra flag on $N.\n\r", char_flag[loc].name);
        MUD_SET_BIT(mob->specials.act, flag);
        /* add them to the event list if mount */
        if (flag == CFL_MOUNT)
            new_event(EVNT_REST, 20, mob, 0, 0, 0, 0);
    } else {
        sprintf(tmp, "$N didnt have the '%s' extra flag.\n\r", char_flag[loc].name);
    }
    act(tmp, FALSE, ch, 0, mob, TO_CHAR);
}


void
set_mqflag(CHAR_DATA * ch, CHAR_DATA * mob, char *buf2, bool set)
{
    char tmp[256];
    int loc;
    long flag;

    if (!*buf2) {
        send_to_char("You may set the following quit flags:\n\r", ch);
        send_flags_to_char(quit_flag, ch);
        return;
    }

    loc = get_flag_num(quit_flag, buf2);
    if (loc == -1) {
        send_to_char("That flag doesn't exist.\n\r", ch);
        return;
    }
    if (GET_LEVEL(ch) < quit_flag[loc].priv) {
        send_to_char("That flag doesn't exist.\n\r", ch);
        return;
    }

    if (IS_NPC(mob)) {
       send_to_char("Not on NPCs!\n\r", ch);
       return;
    }

    flag = quit_flag[loc].bit;

    if (IS_SET(mob->specials.act, flag)) {
        if (set) {
            sprintf(tmp, "$N already had the '%s' flag.\n\r", quit_flag[loc].name);
        } else {
            sprintf(tmp, "You have removed the '%s' flag from $N.\n\r", quit_flag[loc].name);

            REMOVE_BIT(mob->quit_flags, flag);
        }
    } else if (set) {
        sprintf(tmp, "You have set the '%s' quit flag on $N.\n\r", quit_flag[loc].name);
        MUD_SET_BIT(mob->quit_flags, flag);
    } else {
        sprintf(tmp, "$N didnt have the '%s' quit flag.\n\r", quit_flag[loc].name);
    }
    act(tmp, FALSE, ch, 0, mob, TO_CHAR);
}

/* set a mobile's affects */
void
set_maflag(CHAR_DATA * ch, CHAR_DATA * mob, char *buf2, bool set)
{
    char tmp[256];
    int loc;
    long flag;

    if (!*buf2) {
        send_to_char("Usage: cset <char> affect <flag>\n\rflags are.\n\r", ch);
        send_flags_to_char(affect_flag, ch);
        return;
    }

    loc = get_flag_num(affect_flag, buf2);
    if (loc == -1) {
        send_to_char("That flag doesn't exist.\n\r", ch);
        return;
    }
    if (affect_flag[loc].priv > GET_LEVEL(ch)) {
        send_to_char("You are not high enough level to set that flag.\n\r", ch);
        return;
    };
    flag = affect_flag[loc].bit;

    if (IS_SET(mob->specials.affected_by, (flag))) {
        if (set == TRUE) {
            sprintf(tmp, "$N already had the '%s' affect flag.\n\r", affect_flag[loc].name);
        } else {
            sprintf(tmp, "You remove the '%s' affect flag on $N.\n\r", affect_flag[loc].name);
            REMOVE_BIT(mob->specials.affected_by, ((flag)));
        }
    } else {
        if (set == TRUE) {
            sprintf(tmp, "You set the '%s' affect flag on $N.\n\r", affect_flag[loc].name);
            MUD_SET_BIT(mob->specials.affected_by, ((flag)));
        } else {
            sprintf(tmp, "$N didnt have the '%s' affect flag.\n\r", affect_flag[loc].name);
        }
    }
    act(tmp, FALSE, ch, 0, mob, TO_CHAR);

}

void
cmd_cunset(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{

    char buf1[256], buf2[256], buf3[256], buf4[256];
    char tmp[256], tmp2[256];
    char ldesc_s[MAX_INPUT_LENGTH];
    /* ldesc_s is my cheasy kludge for sdesc and ldesc -Tenebrius */
    int amount, loc, i, j, spaces;
    CHAR_DATA *mob;
    CHAR_DATA *tmpvict = NULL;
    struct memory_data *remove;

    const char * const field[] = {
        "cflag",
        "aflag",
        "affect",
        "program",
        "tribe",
        "edesc",
        "hates",
        "fears",
        "likes",
        "qflag",
        "\n"
    };

    char *desc[] = {
        "(character flags)",
        "(affect flags)",
        "(affects)",
        "(programs)",
        "(tribe)",
        "(removes an extra description)",
        "(removes someone from NPC's hates list)",
        "(removes someone from NPC's fears list)",
        "(removes someone from NPC's likes list)",
        "(quit flags)",
        "\n"
    };

    argument = one_argument(argument, buf1, sizeof(buf1));
    argument = one_argument(argument, buf2, sizeof(buf2));
    strcpy(ldesc_s, argument);
    argument = one_argument(argument, buf3, sizeof(buf3));
    strcpy(buf4, argument);

    if (!(mob = get_char_room_vis(ch, buf1))) {
        send_to_char("That person isn't here.\n\r", ch);
        return;
    }

    if (!*buf2) {
        for (i = 0; *field[i] != '\n'; i++) {
            spaces = 15 - strlen(field[i]);
            strcpy(tmp2, "");
            /*sprintf(tmp2, ""); */
            for (j = 0; j < spaces; j++)
                strcat(tmp2, " ");
            sprintf(tmp, "%s%s%s\n\r", field[i], tmp2, desc[i]);
            send_to_char(tmp, ch);
        }
        return;
    }

    if (!IS_NPC(mob)) {
        if (((GET_LEVEL(ch) < DIRECTOR) || (GET_LEVEL(ch) < GET_LEVEL(mob))) && (ch != mob)) {
            send_to_char("That person is not an NPC.\n\r", ch);
            return;
        }
    }

    loc = (old_search_block(buf2, 0, strlen(buf2), field, 0));

    if (loc == -1) {
        send_to_char("That field does not exist.\n\r", ch);
        return;
    }
    amount = atoi(buf3);

    switch (loc) {
    case 1:                    /* character flag */
        set_mcflag(ch, mob, buf3, FALSE);
        /* send_to_char ("Remove character flag is not working.\n\r", ch); */
        break;
    case 2:                    /* affect flag */
        set_maflag(ch, mob, buf3, FALSE);
        /* send_to_char ("Remove affect flag is not working.\n\r", ch); */
        break;
    case 3:                    /* affect */
        if (*buf3 && !strncasecmp(buf3, "all", strlen(buf3))) {
            send_to_char("All affects removed.\n\r", ch);
            remove_all_affects_from_char(mob);
        } else
            send_to_char("Remove affect is not working.  Use 'all' to remove all affects.\n\r", ch);
        break;
    case 4:                    /* program */
        {
        int old_prognum = mob->specials.programs->count;
        mob->specials.programs =
            unset_prog(ch, mob->specials.programs, ldesc_s);
        if (mob->specials.programs->count < old_prognum)
            send_to_char("Program removed.\n\r", ch);
        break;
        }
    case 5:                    /* tribe */
        if (!is_number(buf3)) {
            amount = lookup_clan_by_name(ldesc_s);

            if (amount == -1) {
                send_to_char("No such tribe.\n\r", ch);
                return;
            }
        }

        if (IS_TRIBE(mob, amount)) {
            remove_clan(mob, amount);

            /* if using multi clans only clear tribe variable if it is their
             * default tribe */
            if (mob->player.tribe == amount) {
                /* for single clans, cunset tribe should clear the tribe always */
                mob->player.tribe = 0;
                /* pull the clan leader flag, regardless */
                REMOVE_BIT(mob->specials.act, CFL_CLANLEADER);
            }
        } else {
            send_to_char("They aren't of that tribe.\n\r", ch);
            return;
        }
        break;
    case 6:                    /* edesc */
        unset_medesc(ch, mob, ldesc_s);
        break;

    case 7:                    /* hates */
        if (!strcmp(buf3, "all")) {
            while (mob->specials.hates) {
                remove = mob->specials.hates;
                mob->specials.hates = mob->specials.hates->next;
                free((char *) remove);
            }
            cprintf(ch, "Removed all hates from %s.\n\r", GET_NAME(mob));
            return;
        }
        if ((tmpvict = get_char_world(ch, buf3)) != NULL) {
            if (!IS_NPC(mob)) {
                send_to_char("This can only be unset on NPC's.\n\r", ch);
                return;
            }
            if (does_hate(mob, tmpvict)) {
                remove_from_hates(mob,tmpvict);
                cprintf(ch, "%s has been removed from %s's hates list.\n\r", GET_NAME(tmpvict),
                         GET_NAME(mob));
            } else {
                send_to_char("They are not on the NPC's hates list.\n\r", ch);
                return;
            }
        } else
            send_to_char("Unable to remove from hates list, cannot find target.\n\r", ch);
        return;
        break;

    case 8:                    /* fears */
        if (!strcmp(buf3, "all")) {
            while (mob->specials.fears) {
                remove = mob->specials.fears;
                mob->specials.fears = mob->specials.fears->next;
                free((char *) remove);
            }
            cprintf(ch, "Removed all fears from %s.\n\r", GET_NAME(mob));
            return;
        }
        if ((tmpvict = get_char_world(ch, buf3)) != NULL) {
            if (!IS_NPC(mob)) {
                send_to_char("This can only be unset on NPC's.\n\r", ch);
                return;
            }
            if (does_fear(mob, tmpvict)) {
                remove_from_fears(mob,tmpvict);
                cprintf(ch, "%s has been removed from %s's fears list.\n\r", GET_NAME(tmpvict),
                         GET_NAME(mob));
            } else {
                send_to_char("They are not on the NPC's fears list.\n\r", ch);
                return;
            }
        } else
            send_to_char("Unable to remove from fears list, cannot find target.\n\r", ch);
        return;
        break;

    case 9:                    /* likes */
        if (!strcmp(buf3, "all")) {
            while (mob->specials.likes) {
                remove = mob->specials.likes;
                mob->specials.likes = mob->specials.likes->next;
                free((char *) remove);
            }
            cprintf(ch, "Removed all likes from %s.\n\r", GET_NAME(mob));
            return;
        }
        if ((tmpvict = get_char_world(ch, buf3)) != NULL) {
            if (!IS_NPC(mob)) {
                send_to_char("This can only be unset on NPC's.\n\r", ch);
                return;
            }
            if (does_like(mob, tmpvict)) {
                remove_from_likes(mob,tmpvict);
                cprintf(ch, "%s has been removed from %s's likes list.\n\r", GET_NAME(tmpvict),
                         GET_NAME(mob));
            } else {
                send_to_char("They are not on the NPC's likes list.\n\r", ch);
                return;
            }
        } else
            send_to_char("Unable to remove from likes list, cannot find target.\n\r", ch);
        return;
        break;

    case 10:                    /* quit flag */
        set_mqflag(ch, mob, buf3, FALSE);
        break;
    }

}

/* the big old CSET */
void
cmd_cset(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf1[256], buf2[256], buf3[256], buf4[256];
    char tmp[256], tmp2[256], buf[256];
    char mobnum[10];
    char ldesc_s[MAX_INPUT_LENGTH];
    /* ldesc_s is my cheasy kludge for sdesc and ldesc -Tenebrius */
    int amount, loc, i, j, sk, perc, spaces, ct;
    CHAR_DATA *mob, *orig = SWITCHED_PLAYER(ch);
    CHAR_DATA *tmpvict = NULL;
    bool had_pulse = FALSE;

    /* needed for accounts */
    PLAYER_INFO *pPInfo = NULL;
    DESCRIPTOR_DATA d;
    bool fOld = FALSE;

    const char * const field[] = {
        "level",
        "sex",
        "damage",
        "hps",
        "armor",
        "language",
        "guild",
        "tribe",
        "offense",
        "defense",
        "race",
        "obsidian",
        "coin_mod",
        "strength",
        "agility",
        "wisdom",
        "endurance",
        "name",
        "sdesc",
        "ldesc",
        "extra",
        "skill",
        "power",
        "city",
        "desc",
        "cflag",
        "aflag",
        "height",
        "weight",
        "mana",
        "moves",
        "hunger",
        "thirst",
        "drunk",
        "life",
        "bank",
        "age",
        "program",
        "karma",
        "uid",
        "edesc",
        "background",
        "stun",
        "locdesc",
        "longlocdesc",
        "skillflag",
        "subguild",
        "hates",
        "fears",
        "likes",
        "tdesc",
        "qflag",
        "startcity",
        "\n"
    };

    char *desc[] = {
        "(immortal level of character)",
        "(female, male, or neuter)",
        "(damage modifier)",
        "(maximum health points)",
        "(natural armor points)",
        "(language currently spoken)",
        "(guild the character belongs to)",
        "(tribe or clan)",
        "(offense points)",
        "(defense points)",
        "(race of the character)",
        "(obsidian carried)",
        "(coin_mod random modifier to money)",
        "(strength of character)",
        "(agility of character)",
        "(wisdom of character)",
        "(endurance of character)",
        "(keyword list or name)",
        "(short description)",
        "(long description)",
        "(extended keyword list)",
        "(skills)",
        "(power level of spell)",
        "(home city of character)",
        "(character description)",
        "(character flags)",
        "(affect flags)",
        "(height of character)",
        "(weight of character)",
        "(current mana)",
        "(current movement points)",
        "(hunger : 1 - 48)",
        "(thirst : 1 - 48)",
        "(drunk : 1 - 24)",
        "(life : -100 - 100)",
        "(obsidian in the bank)",
        "(age of the character)",
        "(program)",
        "(karma points for reincarnation)",
        "(user id for creation privs)",
        "(extra descriptions on a mobile)",
        "(background of character)",
        "(maximum stun points)",
        "(a location's extra description)",
        "(a location's extra long description)",
        "(a skill's flag )",
        "(subguild the character belongs to)",
        "(add someone to their hates list)",
        "(add someone to their fears list)",
        "(add someone to their likes list)",
        "(temporary description see: help tdesc)",
        "(quit flag)",
        "(city the character started playing in)",
        "\n"
    };


    argument = one_argument(argument, buf1, sizeof(buf1));
    argument = one_argument(argument, buf2, sizeof(buf2));
    strcpy(ldesc_s, argument);
    argument = one_argument(argument, buf3, sizeof(buf3));
    strcpy(buf4, argument);

    if (!(mob = get_char_room_vis(ch, buf1))) {
        send_to_char("That person isn't here.\n\r", ch);
        return;
    }

    if (IS_NPC(mob))
        sprintf(mobnum, "#%d", npc_index[mob->nr].vnum);
    else
        sprintf(mobnum, "PC");

    if (!*buf2) {
        for (i = 0; *field[i] != '\n'; i++) {
            spaces = 15 - strlen(field[i]);
            strcpy(tmp2, "");
            /*sprintf(tmp2, ""); */
            for (j = 0; j < spaces; j++)
                strcat(tmp2, " ");
            sprintf(tmp, "%s%s%s\n\r", field[i], tmp2, desc[i]);
            send_to_char(tmp, ch);
        }
        return;
    }

    if (!IS_NPC(mob))
        if (((GET_LEVEL(orig) < DIRECTOR) || (GET_LEVEL(orig) < GET_LEVEL(mob))) && (ch != mob)) {
            send_to_char("That person is not an NPC.\n\r", ch);
            return;
        }

    loc = (old_search_block(buf2, 0, strlen(buf2), field, 0));

    if (loc == -1) {
        send_to_char("That field does not exist.\n\r", ch);
        return;
    }

    amount = atoi(buf3);

    switch (loc) {
    case 1:                    /* LEVEL */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> level <amount>\n\r", ch);
            send_to_char("Valid values for level are 0 to 5.\n\r", ch);
            return;
        }

        if ((amount < 0) || (amount > OVERLORD) || (!IS_NPC(mob) && (amount > GET_LEVEL(orig)))) {
            send_to_char("That level is invalid.\n\r", ch);
            send_to_char("Valid values for level are 0 to 5.\n\r", ch);
            return;
        }

        if (!IS_NPC(mob) && !mob->desc && amount != 0) {
            send_to_char("You can't raise their level while they're gone.\n\r", ch);
            return;
        }

        if ((!IS_NPC(mob)) && (GET_LEVEL(mob) == MORTAL) && (amount != 0)
            && (!is_immortal_name(lowercase(mob->name)))) {
            add_immortal_name(lowercase(mob->name), mob->desc->player_info->name);
            save_immortal_names();
        } else if (!IS_NPC(mob) && (amount == 0) && (GET_LEVEL(mob) != MORTAL)) {
            remove_immortal_name(lowercase(mob->name));
            save_immortal_names();
        }

        cprintf(ch, "%s's (%s) level has been changed from %d to %d.\n\r", MSTR(mob, short_descr),
                mobnum, GET_LEVEL(mob), amount);

        GET_LEVEL(mob) = amount;
        break;
    case 2:                    /* SEX */
        if ((*buf3 == 'n') || (*buf3 == 'N'))
            amount = 0;
        else if ((*buf3 == 'm') || (*buf3 == 'M'))
            amount = 1;
        else if ((*buf3 == 'f') || (*buf3 == 'F'))
            amount = 2;
        else {
            send_to_char("Syntax: cset <person> sex <sex>\n\r", ch);
            send_to_char("Valid sexes are male, female, neuter.\n\r", ch);
            return;
        }
        cprintf(ch, "%s's (%s) sex has been changed from %s to %s.\n\r", MSTR(mob, short_descr),
                mobnum, (GET_SEX(mob) == 0) ? "neuter" : (GET_SEX(mob) == 1) ? "male" : "female",
                (amount == 0) ? "neuter" : (amount == 1) ? "male" : "female");
        GET_SEX(mob) = amount;
        break;
    case 3:                    /* DAMAGE */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> damage <amount>\n\r", ch);
            return;
        }

        if (amount > MAX_DAMAGE) {
            send_to_char("That damage is too high.\n\r", ch);
            sprintf(tmp, "Valid damage amounts are %d to %d.\n\r", MIN_DAMAGE, MAX_DAMAGE);
            send_to_char(tmp, ch);
            return;
        } else if (amount < MIN_DAMAGE) {
            send_to_char("That damage is too low.\n\r", ch);
            sprintf(tmp, "Valid damage amounts are %d to %d.\n\r", MIN_DAMAGE, MAX_DAMAGE);

            return;
        }

        sprintf(tmp, "%s's (%s) damage has been changed from %d  to %d.\n\r",
                MSTR(mob, short_descr), mobnum, mob->abilities.damroll, amount);
        send_to_char(tmp, ch);

        mob->abilities.damroll = amount;
        mob->tmpabilities.damroll = amount;
        break;
    case 4:                    /* HPS */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> hps <amount>\n\r", ch);
            return;
        }
        if ((amount > MAX_HITS) || (amount < 0)) {
            send_to_char("That amount is invalid.\n\r", ch);
            sprintf(tmp, "Valid values for hps are 0 to %d.\n\r", MAX_HITS);
            send_to_char(tmp, ch);
            return;
        }

        sprintf(tmp,
                "%s's (%s) hit points have changed from %d  to %d.\n\r"
                "(They will still get their Endurance bonus, however).\n\r", MSTR(mob, short_descr),
                mobnum, mob->points.max_hit, amount);
        send_to_char(tmp, ch);

        mob->points.max_hit = amount;
        break;
    case 5:                    /* ARMOR */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> armor <amount>\n\r", ch);
            cprintf(ch, "Valid values for armor are %d to %d.\n\r", MIN_ARMOR, MAX_ARMOR);
            return;
        }

        if ((amount < MIN_ARMOR) || (amount > MAX_ARMOR)) {
            send_to_char("That armor value is invalid.\n\r", ch);
            cprintf(ch, "Valid values for armor are %d to %d.\n\r", MIN_ARMOR, MAX_ARMOR);
            return;
        }

        cprintf(ch, "%s's (%s) armor has been changed from %d to %d.\n\r", MSTR(mob, short_descr),
                mobnum, mob->abilities.armor, amount);

        mob->abilities.armor = amount;
        mob->tmpabilities.armor = amount;

        break;
    case 6:                    /* LANGUAGE */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> language <language>\n\r", ch);
            send_to_char("You must supply a language name.\n\r", ch);
            send_to_char("Valid languages are:\n\r", ch);
            for (i = 0; i < (MAX_TONGUES); i++) {
                send_to_char(skill_name[language_table[i].spoken], ch);
                send_to_char("\n\r", ch);
            }
            return;
        }
        for (i = 0; i < (MAX_TONGUES); i++) {
            if (!strncasecmp(buf3, skill_name[language_table[i].spoken], strlen(buf3)))
                break;
        }
        if (i < (MAX_TONGUES)) {
            j = mob->specials.language;

            cprintf(ch, "%s's (%s) default language as been changed from %s to %s.\n\r",
                    MSTR(mob, short_descr), mobnum, skill_name[language_table[j].spoken],
                    skill_name[language_table[i].spoken]);

            mob->specials.language = i;

        } else {
            cprintf(ch, "%s is not a valid language.\n", buf3);
            send_to_char("Valid languages are:\n\r", ch);
            for (i = 0; i < (MAX_TONGUES); i++) {
                send_to_char(skill_name[language_table[i].spoken], ch);
                send_to_char("\n\r", ch);
            }
        }
        break;
    case 7:                    /* GUILD */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> guild <guild>\n\r", ch);
            send_to_char("Valid guilds are:\n\r", ch);
            for (i = 1; i < MAX_GUILDS; i++)
                cprintf(ch, "%s\n\r", guild[i].name);
            return;
        }
        if (!strnicmp(buf3, "none", strlen(buf3))) {
            mob->player.guild = 0;
            break;
        }
        for (i = 1; i < MAX_GUILDS; i++)
            if (!strnicmp(buf3, guild[i].name, strlen(buf3)))
                break;

        if (i == MAX_GUILDS) {
            send_to_char("That guild does not exist.\n\r", ch);
            send_to_char("Syntax: cset <person> guild <guild>\n\r", ch);
            send_to_char("Valid guilds are:\n\r", ch);
            for (i = 1; i < MAX_GUILDS; i++)
                cprintf(ch, "%s\n\r", guild[i].name);
            return;
        }
        j = mob->player.guild;
        cprintf(ch, "%s's (%s) guild has been changed from %s to %s.\n", MSTR(mob, short_descr),
                mobnum, guild[j].name, guild[i].name);
        mob->player.guild = i;

        break;
    case 8:                    /* TRIBE */
        if (buf3[0] != '\0') {
            int clan = amount;
            int rank = 0;
            int flag = 0;
            bool remove = FALSE;

            /* are they giving a rank or a tribe #? */
            if (is_number(buf3) && buf4[0] != '\0') {
                strcpy(ldesc_s, buf4);
                rank = atoi(buf3);

                /* need to know the clan for rank */
                if (!is_number(buf4)) {
                    clan = lookup_clan_by_name(ldesc_s);

                    if (clan == -1) {
                        send_to_char("No such tribe.\n\r", ch);
                        return;
                    }
                } else
                    clan = atoi(buf4);


                if (rank < MIN_CLAN_RANK || rank > clan_ranks[clan].num_ranks) {
                    sprintf(buf, "Rank is a number from %d to %d.\n\r", MIN_CLAN_RANK,
                            clan_ranks[clan].num_ranks);
                    send_to_char(buf, ch);
                    return;
                }

                /* grab the next arg for possible tribe # */
                one_argument(argument, buf3, sizeof(buf3));
            } else if ((flag = get_flag_bit(clan_job_flags, buf3)) != -1) {
                strcpy(ldesc_s, buf4);

                /* grab the next arg for possible tribe # */
                one_argument(argument, buf3, sizeof(buf3));
            } else if (!strcmp(buf3, "remove")
                       || !strcmp(buf3, "rem")
                       || !strcmp(buf3, "delete")
                       || !strcmp(buf3, "del")) {
                remove = TRUE;

                /* save the rest as possible tribe name */
                strcpy(ldesc_s, buf4);

                /* grab the next arg for possible tribe # */
                one_argument(argument, buf3, sizeof(buf3));
            }

            if (!is_number(buf3)) {
                clan = lookup_clan_by_name(ldesc_s);

                if (clan == -1) {
                    send_to_char("No such tribe.\n\r", ch);
                    return;
                }
            } else
                clan = atoi(buf3);

            if (flag == -1)
                flag = 0;

            if (remove) {
                if (IS_TRIBE(mob, clan)) {
                    remove_clan(mob, clan);
                    break;
                } else {
                    send_to_char("They aren't of that tribe, nothing changed.\n\r", ch);
                    return;
                }
            }

            if (!IS_TRIBE(mob, clan)) {
                /* first clan? set the tribe so that old system will still work */
                if (!mob->clan)
                    mob->player.tribe = clan;

                add_clan(mob, clan);
            }

            if (flag) {
                if (is_set_clan_flag(mob->clan, clan, flag))
                    remove_clan_flag(mob->clan, clan, flag);
                else
                    set_clan_flag(mob->clan, clan, flag);

                /* and for legacy's sake... */
                if (clan == mob->player.tribe && flag == CJOB_LEADER) {
                    if (IS_SET(mob->specials.act, CFL_CLANLEADER))
                        REMOVE_BIT(mob->specials.act, CFL_CLANLEADER);
                    else
                        MUD_SET_BIT(mob->specials.act, CFL_CLANLEADER);
                }
            } else if (rank) {
                set_clan_rank(mob->clan, clan, rank);
            }

            break;
        } else {
            send_to_char("Syntax:  cset <person> tribe <tribe>\n\r"
                         "         cset <person> tribe <rank #> <tribe>\n\r"
                         "         cset <person> tribe <job name> <tribe>\n\r"
                         "         cset <person> tribe remove <tribe>\n\r", ch);
            return;
        }
        break;
    case 9:                    /* OFFENSE */
        if (!(*buf3)) {
            send_to_char("Syntax: cset person off <amount>\n\r", ch);
            cprintf(ch, "Valid amounts for offense are %d to %d.\n\r", MIN_OFF, MAX_OFF);
            return;
        }
        if ((amount > MAX_OFF) || (amount < MIN_OFF)) {
            send_to_char("That offense modifier is invalid.\n\r", ch);
            cprintf(ch, "Valid amounts for offense are %d to %d.\n\r", MIN_OFF, MAX_OFF);
            return;
        }
        cprintf(ch, "%s's (%s) offense has been changed from %d to %d.\n\r", MSTR(mob, short_descr),
                mobnum, mob->abilities.off, amount);

        mob->abilities.off = amount;
        mob->tmpabilities.off = amount;

        break;
    case 10:                   /* DEFENSE */
        if (!(*buf3)) {
            send_to_char("Syntax: cset person def <amount>\n\r", ch);
            cprintf(ch, "Valid amounts for defense are %d to %d.\n\r", MIN_OFF, MAX_OFF);
            return;
        }
        if ((amount > MAX_OFF) || (amount < MIN_DEF)) {
            send_to_char("That defense modifier is invalid.\n\r", ch);
            cprintf(ch, "Valid amounts for defense are %d to %d.\n\r", MIN_OFF, MAX_OFF);
            return;
        }
        cprintf(ch, "%s's (%s) defense has been changed from %d to %d.\n\r", MSTR(mob, short_descr),
                mobnum, mob->abilities.def, amount);

        mob->abilities.def = amount;
        mob->tmpabilities.def = amount;

        break;
    case 11:                   /* RACE */
        if (!(*ldesc_s)) {
            send_to_char("Syntax: cset <person> race <race>\n\r", ch);
            parse_command(ch, "races");

            return;
        }
        for (i = 1; i < MAX_RACES; i++)
            if (!str_prefix(ldesc_s, race[i].name))
                break;

        if (i == MAX_RACES) {
            send_to_char("That race does not exist.\n\r", ch);
            parse_command(ch, "races");
            return;
        }
        cprintf(ch, "%s's (%s) race has been changed from %s  to %s.\n\r", MSTR(mob, short_descr),
                mobnum, race[mob->player.race].name, race[i].name);

        mob->player.race = i;
        mob->abilities.armor = race[i].natural_armor;
        mob->tmpabilities.armor = race[i].natural_armor;
        mob->player.height = char_init_height(i);
        mob->player.weight = char_init_weight(i);
        break;
    case 12:                   /* OBSIDIAN */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> obs <amount>\n\r", ch);
            return;
        }
        if (amount < MIN_OBSIDIAN) {
            send_to_char("You must specify a POSITIVE amount.\n\r", ch);
            return;
        }
        cprintf(ch, "%s's (%s) obsidian has been changed from %d to %d.\n\r",
                MSTR(mob, short_descr), mobnum, mob->points.obsidian, amount);

        mob->points.obsidian = amount;
        break;
    case 13:                   /* COIN MOD */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> coin_mod <amount>\n\r", ch);
            return;
        }
        if (amount < MIN_OBSIDIAN) {
            send_to_char("You must specify a POSITIVE amount.\n\r", ch);
            return;
        }
        cprintf(ch, "%s's (%s) coin_mod has been changed from %d to %d.\n\r",
                MSTR(mob, short_descr), mobnum, mob->specials.gp_mod, amount);

        mob->specials.gp_mod = amount;
        break;
    case 14:                   /* STRENGTH */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> str <amount>\n\r", ch);
            cprintf(ch, "Valid values for amount are %d to %d.\n\r", MIN_ABILITY, MAX_ABILITY);
            return;
        }
        if ((amount < MIN_ABILITY) || (amount > MAX_STRENGTH)) {
            send_to_char("That amount is invalid.\n\r", ch);
            cprintf(ch, "Valid values for amount are %d to %d.\n\r", MIN_ABILITY, MAX_ABILITY);
            return;
        }

        cprintf(ch, "%s's (%s) strength has been changed from %d to %d.\n\r",
                MSTR(mob, short_descr), mobnum, mob->abilities.str, amount);

        mob->abilities.str = amount;
        mob->tmpabilities = mob->abilities;

        break;
    case 15:                   /* AGILITY */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> agility <amount>\n\r", ch);
            cprintf(ch, "Valid values for amount are %d to %d.\n\r", MIN_ABILITY, MAX_ABILITY);
            return;
        }
        if ((amount < MIN_ABILITY) || (amount > MAX_ABILITY)) {
            send_to_char("That amount is invalid.\n\r", ch);
            cprintf(ch, "Valid values for amount are %d to %d.\n\r", MIN_ABILITY, MAX_ABILITY);
            return;
        }
        cprintf(ch, "%s's (%s) agility has been changed from %d to %d.\n\r", MSTR(mob, short_descr),
                mobnum, mob->abilities.agl, amount);

        mob->abilities.agl = amount;
        mob->tmpabilities = mob->abilities;

        break;
    case 16:                   /* WISDOM */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> wisdom <amount>\n\r", ch);
            cprintf(ch, "Valid values for amount are %d to %d.\n\r", MIN_ABILITY, MAX_ABILITY);
            return;
        }
        if ((amount < MIN_ABILITY) || (amount > MAX_ABILITY)) {
            send_to_char("That amount is invalid.\n\r", ch);
            cprintf(ch, "Valid values for amount are %d to %d.\n\r", MIN_ABILITY, MAX_ABILITY);
            return;
        }
        cprintf(ch, "%s's (%s) wisdom has been changed from %d to %d.\n\r", MSTR(mob, short_descr),
                mobnum, mob->abilities.wis, amount);

        mob->abilities.wis = amount;
        mob->tmpabilities = mob->abilities;
        break;
    case 17:                   /* ENDURANCE */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> endurance <amount>\n\r", ch);
            cprintf(ch, "Valid values for amount are %d to %d.\n\r", MIN_ABILITY, MAX_ABILITY);
            return;
        }
        if ((amount < MIN_ABILITY) || (amount > MAX_ABILITY)) {
            send_to_char("That amount is invalid.\n\r", ch);
            cprintf(ch, "Valid values for amount are %d to %d.\n\r", MIN_ABILITY, MAX_ABILITY);
            return;
        }
        cprintf(ch, "%s's (%s) endurance has been changed from %d to %d.\n\r",
                MSTR(mob, short_descr), mobnum, mob->abilities.end, amount);

        mob->abilities.end = amount;
        mob->tmpabilities = mob->abilities;

        break;
    case 18:                   /* NAME */
        if (IS_NPC(mob)) {
            if (!*ldesc_s) {
                send_to_char("Syntax: cset <person> name <namestring>\n\r", ch);
                return;
            } else {
                cprintf(ch, "%s's (%s) name changed from: %s\n\rto: %s\n\r", MSTR(mob, short_descr),
                        mobnum, MSTR(mob, name), ldesc_s);
                free((char *) mob->name);
                mob->name = strdup(ldesc_s);
            };
        } else {                /* not an NPC */
            if (GET_LEVEL(orig) < HIGHLORD) {
                send_to_char("You can only set the name of NPC's!\n\r", ch);
                return;
            }

            if (!*buf3) {
                send_to_char("Syntax: cset <person> name <namestring>\n\r", ch);
                return;
            } else if (!mob->desc) {
                send_to_char("Only do this while the person has their link.\n\r", ch);
                return;
            } else if (strcmp(mob->desc->player_info->name, mob->account)) {
                send_to_char("Only do this while the person has THEIR link.\n\r", ch);
                return;
            } else {
                /* CAP(buf3); */
                cprintf(ch, "%s's (%s) name changed from: %s\n\rto: %s\n\r", MSTR(mob, short_descr),
                        mobnum, MSTR(mob, name), buf3);

#define MORG_FIX_CSET_NAME
#ifdef MORG_FIX_CSET_NAME
                if (lookup_char_info(mob->desc->player_info, mob->name)) {
                    remove_char_info(mob->desc->player_info, lowercase(mob->name));
                }
                save_player_info(mob->desc->player_info);
                add_new_char_info(mob->desc->player_info, lowercase(buf3), APPLICATION_NONE);

                sprintf(buf, "%s/%s/%s", ACCOUNT_DIR, mob->desc->player_info->name,
                        lowercase(mob->name));
                unlink(buf);
#endif

                free((char *) mob->name);

                *buf3 = toupper(*buf3);
                mob->name = strdup(buf3);
            }
        }
        break;
    case 19:                   /* SDESC */
        if (!(*ldesc_s)) {
            send_to_char("Syntax: cset <person> sdesc <sdescstring>\n\r", ch);
            cprintf(ch, "Maximum sdesc length is %d characters.\n\r", MAX_SDESC);
            return;
        }
        if (strlen(ldesc_s) > MAX_SDESC) {      /*  Kel  */
            send_to_char("That string is too long.\n\r", ch);
            cprintf(ch, "Maximum sdesc length is %d characters.\n\r", MAX_SDESC);
            return;
        }

        cprintf(ch, "%s's (PC) sdesc changed from: %s\n\rto: %s\n\r", MSTR(mob, short_descr),
                MSTR(mob, short_descr), ldesc_s);

        if (mob->short_descr)
            free((char *) mob->short_descr);
        mob->short_descr = strdup(ldesc_s);

        break;
    case 20:                   /* LDESC */
        if (!(*ldesc_s)) {
            send_to_char("Syntax: cset <person> ldesc <ldescstring>\n\r", ch);
            cprintf(ch, "Maximum ldesc length is %d characters.\n\r", MAX_LDESC);
            return;
        }
        if (strlen(ldesc_s) > MAX_LDESC) {
            send_to_char("That string is toooo long.\n\r", ch);
            cprintf(ch, "Maximum ldesc length is %d characters.\n\r", MAX_LDESC);
            return;
        }

        cprintf(ch, "%s's (%s) ldesc changed from: %s\n\rto: %s\n\r", MSTR(mob, short_descr),
                mobnum, MSTR(mob, long_descr), ldesc_s);

        if (mob->long_descr)
            free((char *) mob->long_descr);
        strcat(ldesc_s, "\n\r");
        mob->long_descr = strdup(ldesc_s);
        break;
    case 21:                   /* EXTRA KEYWORDS */
        if (!(*ldesc_s)) {
            send_to_char("Syntax: cset <person> extra <extrastring>\n\r", ch);
            send_to_char("(NPC extra keywords are set with the 'name' field.)" "\n\r", ch);
            return;
        }

        if (IS_NPC(mob)) {
            send_to_char("NPC extra keywords are set with the 'name' field." "\n\r", ch);
            return;
        }

        if (!IS_NPC(mob)) {
            cprintf(ch, "%s's (%s) keywords changed from: %s\n\rto: %s\n\r", MSTR(mob, short_descr),
                    mobnum, mob->player.extkwds, ldesc_s);
        }

        if (mob->player.extkwds)
            free((char *) mob->player.extkwds);
        mob->player.extkwds = strdup(ldesc_s);
        break;

    case 22:                   /* SKILLS */
        if (!*buf4) {
            send_to_char("Syntax: cset <person> skill <value> <skillname>:\n\r", ch);
            send_to_char("Following skills are available:\n\r", ch);
            send_list_to_char_f(ch, &skill_name[1], 0);
            return;
        };
        for (sk = 1; sk < MAX_SKILLS; sk++)
            if (!strnicmp(skill_name[sk], buf4, strlen(buf4)))
                break;
        if (sk == MAX_SKILLS) {
            send_to_char("No such skill of that name.\n\r", ch);
            return;
        }

        if (((perc = amount) > MAX_SKILL_P) || (perc < MIN_SKILL_P)) {
            send_to_char("That skill percentage is invalid.\n\r", ch);
            return;
        }

        if (perc > 0) {
            if (has_skill(mob, sk))
                /* if (mob->skills[sk]) */
                mob->skills[sk]->learned = perc;
            else
                init_skill(mob, sk, perc);
        } else {
            delete_skill(mob, sk);
        }


        cprintf(ch, "%s (%s) has been set with %s at %d%s", MSTR(mob, short_descr), mobnum,
                skill_name[sk], perc, "%.\n\r");

        break;
    case 23:                   /* POWER LEVEL */
        if (!*buf4) {
            send_to_char("Syntax: cset <person> power <powerlevel> <skill>.\n\r", ch);
            send_to_char("Valid powerlevels are wek, yuqa, kral, een pav, sul and mon.\n\r", ch);
            send_to_char("The following skills are available:\n\r", ch);
            send_list_to_char_f(ch, &skill_name[1], 0);
            return;
        };
        for (sk = 1; sk < MAX_SKILLS; sk++)
            if (!strnicmp(skill_name[sk], buf4, strlen(buf4)))
                break;

        if (sk == MAX_SKILLS) {
            send_to_char("No such skill of that name.\n\r", ch);
            return;
        }
        perc = (old_search_block(buf3, 0, strlen(buf3), Power, 0));

        // old_search_block returns 1 too high
        perc--;

        if ((perc > 6) || (perc < 0)) {
            send_to_char("That power level is invalid.\n\r", ch);
            send_to_char("Valid powerlevels are wek, yuqa, kral, een pav, sul and mon.\n\r", ch);

            return;
        }

        if (has_skill(mob, sk))
            mob->skills[sk]->rel_lev = perc;
        else {
            init_skill(mob, sk, skill[sk].start_p);
            mob->skills[sk]->rel_lev = perc;
        }

        cprintf(ch, "%s (%s) has been set with %s at power level %s.\n\r", MSTR(mob, short_descr),
                mobnum, skill_name[sk], buf3);

        break;
    case 24:                   /* CITY */
        if (!*buf3) {
            send_to_char("Syntax: cset <person> city <city>\n\r", ch);
            send_to_char("The following cities are currently defined:\n\r", ch);
            send_to_char("-------------------------------------------\n\r", ch);
            for (ct = 0; ct < MAX_CITIES; ct++) {
                sprintf(buf, "%s -- start room: %d, tribe: %d\n\r", city[ct].name, city[ct].room,
                        city[ct].tribe);
                send_to_char(buf, ch);
            }

            return;
        }

        for (ct = 0; ct < MAX_CITIES; ct++)
            if (!strnicmp(city[ct].name, buf3, strlen(buf3)))
                break;
        if (ct == MAX_CITIES) {
            send_to_char("That city does not exist.\n\r", ch);
            send_to_char("The following cities are currently defined:\n\r", ch);
            send_to_char("-------------------------------------------\n\r", ch);
            for (ct = 0; ct < MAX_CITIES; ct++) {
                sprintf(buf, "%s -- start room: %d, tribe: %d\n\r", city[ct].name, city[ct].room,
                        city[ct].tribe);
                send_to_char(buf, ch);
            }
            return;
        }

        cprintf(ch, "%s's (%s) city has been changed from %s to %s.\n\r", MSTR(mob, short_descr),
                mobnum, city[mob->player.hometown].name, city[ct].name);

        mob->player.hometown = ct;

        break;
    case 25:                   /* DESCRIPTION */
        set_mdesc(ch, mob);
        return;
    case 26:                   /* CFLAG */
        set_mcflag(ch, mob, buf3, TRUE);
        return;
    case 27:                   /* AFLAG */
        set_maflag(ch, mob, buf3, TRUE);
        return;
    case 28:
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> height <amount>\n\r", ch);
            cprintf(ch, "Amount must be at least %d.\n\r", MIN_HEIGHT);
            return;
        }
        if (amount <= MIN_HEIGHT) {
            cprintf(ch, "Amount must be at least %d.\n\r", MIN_HEIGHT);
            return;
        }
        cprintf(ch, "%s's (%s) height has been changed from %d  to %d.\n\r", MSTR(mob, short_descr),
                mobnum, mob->player.height, amount);

        mob->player.height = amount;
        break;
    case 29:                   /* WEIGHT */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> weight <amount>\n\r", ch);
            cprintf(ch, "Amount must be at least %d.\n\r", MIN_HEIGHT);
            return;
        }
        if (amount <= MIN_WEIGHT) {
            cprintf(ch, "Amount must be at least %d.\n\r", MIN_HEIGHT);
            return;
        }
        cprintf(ch, "%s's (%s) weight has been from %d to %d.\n\r", MSTR(mob, short_descr), mobnum,
                mob->player.weight, amount);

        mob->player.weight = amount;
        break;
    case 30:                   /* MANA */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> mana <amount>\n\r", ch);
            cprintf(ch, "Amount must be at least %d.\n\r", MIN_MANA);
            return;
        }
        if (amount < MIN_MANA) {
            cprintf(ch, "Amount must be at least %d.\n\r", MIN_MANA);
            return;
        }
        cprintf(ch,
                "%s's (%s) mana has been changed from %d to %d.\n\r"
                "(They will still get their wisdom bonus).", MSTR(mob, short_descr), mobnum,
                GET_MANA(mob), amount);

        set_mana(mob, amount);
        break;
    case 31:                   /* MOVES */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> moves <amount>\n\r", ch);
            cprintf(ch, "Amount must be at least %d.\n\r", MIN_MOVE);
            return;
        }
        if (amount < MIN_MOVE) {
            cprintf(ch, "Amount must be at least %d.\n\r", MIN_MOVE);
            return;
        }
        cprintf(ch, "%s's (%s) current moves have been changed from %d to %d." "\n\r",
                MSTR(mob, short_descr), mobnum, GET_MOVE(mob), amount);

        set_move(mob, amount);
        break;
    case 32:                   /* HUNGER */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> hunger <amount>\n\r", ch);
            cprintf(ch, "Valid values for amount (on this person) are from %d" " to %d.\n\r",
                    MIN_CONDITION, get_max_condition(mob, FULL));
            return;
        }
        if ((amount < MIN_CONDITION) || (amount > get_max_condition(mob, FULL))) {
            cprintf(ch, "Valid values for amount (on this person) are from %d " "to %d.\n\r",
                    MIN_CONDITION, get_max_condition(mob, FULL));
            return;
        }
        GET_COND(mob, FULL) = amount;
        cprintf(ch, "%s's hunger has been set to %d.\n\r", MSTR(mob, short_descr), amount);
        break;
    case 33:                   /* THIRST */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> thirst <amount>\n\r", ch);
            cprintf(ch, "Valid values for amount (on this person) are from %d" " to %d.\n\r",
                    MIN_CONDITION, get_max_condition(mob, THIRST));
            return;
        }

        if ((amount < MIN_CONDITION) || (amount > get_max_condition(mob, THIRST))) {
            cprintf(ch, "Valid values for amount (on this person) are from %d" " to %d.\n\r",
                    MIN_CONDITION, get_max_condition(mob, THIRST));
            return;
        }
        GET_COND(mob, THIRST) = amount;
        cprintf(ch, "%s's thirst has been set to %d.\n\r", MSTR(mob, short_descr), amount);
        break;
    case 34:                   /* DRUNK */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> drunk <amount>\n\r", ch);
            cprintf(ch, "Valid values for amount (on this person) are from %d" " to %d.\n\r",
                    MIN_CONDITION, get_max_condition(mob, DRUNK));
            return;
        }

        if ((amount < MIN_CONDITION) || (amount > get_max_condition(mob, DRUNK))) {
            cprintf(ch, "Valid values for amount (on this person) are from %d" " to %d.\n\r",
                    MIN_CONDITION, get_max_condition(mob, DRUNK));
            return;
        }
        GET_COND(mob, DRUNK) = amount;
        cprintf(ch, "%s's drunk has been set to %d.\n\r", MSTR(mob, short_descr), amount);
        break;
    case 35:                   /* life */
        if (GET_LEVEL(orig) < HIGHLORD) {
            send_to_char("You are not high enough level to set that.", ch);
            return;
        }
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> life <amount>\n\r", ch);
            cprintf(ch, "Valid values for amount are from %d to %d.\n\r", MIN_LIFE, MAX_LIFE);
            return;
        }

        if ((amount < MIN_LIFE) || (amount > MAX_LIFE)) {
            cprintf(ch, "Invalid life amount.\n\r" "Valid values for amount are from %d to %d.\n\r",
                    MIN_LIFE, MAX_LIFE);
            return;
        }

        mob->specials.eco = amount;
        cprintf(ch, "%s's life has been set to %d.\n\r", MSTR(mob, short_descr), amount);
        break;
    case 36:                   /* BANK */
        if (GET_LEVEL(orig) < DIRECTOR) {
            send_to_char("You are not high enough level to set that.", ch);
            return;
        }

        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> bank <amount>\n\r", ch);
            cprintf(ch, "Valid values for bank must be at least %d.\n\r", MIN_OBSIDIAN);
            return;
        }

        if (amount < MIN_OBSIDIAN) {
            cprintf(ch, "Invalid bank amount.\n\r" "Valid values for bank must be at least %d.\n\r",
                    MIN_OBSIDIAN);
            return;
        }

        mob->points.in_bank = amount;
        cprintf(ch, "%s's bank has been set to %d.\n\r", MSTR(mob, short_descr), amount);

        break;
    case 37:                   /* AGE */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> age <amount>\n\r", ch);
            send_to_char("Valid values for amount must be at least 1.\n\r", ch);
            return;
        }

        if (amount <= 0) {
            send_to_char("Valid values for amount must be at least 1.\n\r", ch);
            return;
        }

        if (!set_age(mob, amount)) {
            cprintf(ch, "%d is an unacceptable age.\n\r", amount);
            return;
        }

        cprintf(ch, "%s's age has been set to %d.\n\r", MSTR(mob, short_descr), amount);

        break;
    case 38:                   /* program */
        had_pulse = has_special_on_cmd(mob->specials.programs, NULL, CMD_PULSE);
        mob->specials.programs =
            set_prog(ch, mob->specials.programs, ldesc_s);
        if (!had_pulse && has_special_on_cmd(mob->specials.programs, NULL, CMD_PULSE))
            new_event(EVNT_NPC_PULSE, PULSE_MOBILE, mob, 0, 0, 0, 0);
        cprintf(ch, "Program set on %s.\n\r", MSTR(mob, short_descr));

        break;
    case 39:                   /* karma */
        if (IS_NPC(mob)) {
            send_to_char("Not on NPCs!\n\r", ch);
            return;
        }

        if (mob->desc)
            pPInfo = mob->desc->player_info;

        if (!pPInfo) {
            if ((pPInfo = find_player_info(mob->player.info[0])) == NULL) {
                if ((fOld = load_player_info(&d, mob->player.info[0])) == FALSE) {
                    /* didn't find them */
                    sprintf(buf, "No account found for %s.\n\r", mob->name);
                    send_to_char(buf, ch);
                    return;
                }
                pPInfo = d.player_info;
                d.player_info = NULL;
            }
        }

        pPInfo->karma = amount;
        sprintf(buf, "%s%s set %s (%s)'s karma to %d%s%s.\n\r",
                pPInfo->karmaLog ? pPInfo->karmaLog : "", MSTR(ch, name), MSTR(mob, name),
                pPInfo->name, pPInfo->karma, buf4[0] == '\0' ? "" : ", ",
                buf4[0] == '\0' ? "" : buf4);

        if (pPInfo->karmaLog || (pPInfo->karmaLog && !strcmp(pPInfo->karmaLog, "(null)")))
            free(pPInfo->karmaLog);

        pPInfo->karmaLog = strdup(buf);
        save_player_info(pPInfo);

        if (fOld)
            free_player_info(pPInfo);

        send_to_char("OK.\n\r", ch);
        break;
    case 40:                   /* UID */
        if (GET_LEVEL(orig) < OVERLORD) {
            send_to_char("Only Overlords may set UID.\n\r", ch);
            return;
        };

        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> uid <amount>\n\r", ch);
            return;
        }

        cprintf(ch, "%s's UID has been changed from %d to %d.\n\r", MSTR(mob, short_descr),
                mob->specials.uid, amount);

        mob->specials.uid = amount;

        break;
    case 41:                   /* edesc */
        set_medesc(ch, mob, ldesc_s);
        return;
    case 42:                   /* background */
        set_mbackground(ch, mob);
        return;
    case 43:                   /* stun */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> stun <amount>\n\r", ch);
            cprintf(ch, "Valid values for stun are from 0 to %d.\n\r", MAX_STUN);
            return;
        }

        if (amount > MAX_STUN || amount < 0) {
            cprintf(ch, "Invalid stun amount.\n\r" "Valid values for stun are from 0 to %d.\n\r",
                    MAX_STUN);
            return;
        }

        mob->points.max_stun = amount;

        cprintf(ch,
                "%s's stun has  been set to %d.\n\r" "(They will still get their endurance bonus)",
                MSTR(mob, short_descr), amount);
        break;
    case 44:
        if (!*ldesc_s) {
            sprintf(buf, "You can set the following edescs\n\r");
            send_to_char(buf, ch);

            for (i = 0; i < MAX_WEAR_EDESC; i++) {
                sprintf(buf, "   %s\n\r", wear_edesc[i].name);
                send_to_char(buf, ch);
            };
            return;
        };

        for (i = 0; i < MAX_WEAR_EDESC && strcmp(ldesc_s, wear_edesc[i].name); i++) {
            /* do nothing, find match */
        };
        if (i >= MAX_WEAR_EDESC) {
            send_to_char("That was not a valid location.\n\r", ch);
            return;
        };

        set_medesc(ch, mob, wear_edesc[i].edesc_name);

        /*  send_to_char("not working right now, bug tenebrius\n\r", ch); */
        return;

    case 45:
        if (!*ldesc_s) {
            sprintf(buf, "You can set the following long edescs\n\r");
            send_to_char(buf, ch);

            for (i = 0; i < MAX_WEAR_EDESC; i++) {
                sprintf(buf, "   %s\n\r", wear_edesc_long[i].name);
                send_to_char(buf, ch);
            };
            return;
        };

        for (i = 0; i < MAX_WEAR_EDESC && strcmp(ldesc_s, wear_edesc_long[i].name); i++) {
            /* do nothing, find match */
        };
        if (i >= MAX_WEAR_EDESC) {
            send_to_char("That was not a valid location.\n\r", ch);
            return;
        };

        set_medesc(ch, mob, wear_edesc_long[i].edesc_name);

        /*  send_to_char("not working right now, bug tenebrius\n\r", ch); */
        return;

    case 46:                   /* skillflag */
        set_mskillflag(ch, mob, ldesc_s);
        break;

    case 47:                   /* Sub-guild */
        if (!(*buf3)) {
            send_to_char("Syntax: cset <person> subguild <subguild>\n\r", ch);
            send_to_char("Valid subguilds are:\n\r", ch);
            for (i = 1; i < MAX_SUB_GUILDS; i++)
                cprintf(ch, "%s\n\r", sub_guild[i].name);
            return;
        }
        if (!strnicmp(buf3, "none", strlen(buf3))) {
            mob->player.sub_guild = 0;
            break;
        }
        for (i = 1; i < MAX_SUB_GUILDS; i++)
            if (!strnicmp(buf3, sub_guild[i].name, strlen(buf3)))
                break;

        if (i == MAX_SUB_GUILDS) {
            send_to_char("That subguild does not exist.\n\r", ch);
            send_to_char("Syntax: cset <person> subguild <subguild>\n\r", ch);
            send_to_char("Valid subguilds are:\n\r", ch);
            for (i = 1; i < MAX_SUB_GUILDS; i++)
                cprintf(ch, "%s\n\r", sub_guild[i].name);
            return;
        }
        j = mob->player.sub_guild;
        cprintf(ch, "%s's (%s) subguild has been changed from %s to %s.\n", MSTR(mob, short_descr),
                mobnum, sub_guild[j].name, sub_guild[i].name);
        mob->player.sub_guild = i;

        break;

    case 48:                   /* Hates */
        if ((tmpvict = get_char_world(ch, buf3)) != NULL) {
            if (!IS_NPC(mob)) {
                send_to_char("This can only be set on NPC's.\n\r", ch);
                return;
            }
            if (!does_hate(mob, tmpvict)) {
                add_to_hates(mob,tmpvict);
                cprintf(ch, "%s has been added to %s's hates list.\n\r", GET_NAME(tmpvict),
                         GET_NAME(mob));
            } else {
                send_to_char("They already hate that person.\n\r", ch);
                return;
            }
        } else
            send_to_char("Unable to add to hates list, cannot find target.\n\r", ch);
        return;
        
        break;

    case 49:                   /* Fears */
        if ((tmpvict = get_char_world(ch, buf3)) != NULL) {
            if (!IS_NPC(mob)) {
                send_to_char("This can only be set on NPC's.\n\r", ch);
                return;
            }
            if (!does_fear(mob, tmpvict)) {
                add_to_fears(mob,tmpvict);
                cprintf(ch, "%s has been added to %s's fears list.\n\r", GET_NAME(tmpvict),
                         GET_NAME(mob));
            } else {
                send_to_char("They already fear that person.\n\r", ch);
                return;
            }
        } else
            send_to_char("Unable to add to fears list, cannot find target.\n\r", ch);
        return;

        break;

    case 50:                   /* Likes */
        if ((tmpvict = get_char_world(ch, buf3)) != NULL) {
            if (!IS_NPC(mob)) {
                send_to_char("This can only be set on NPC's.\n\r", ch);
                return;
            }
            if (!does_like(mob, tmpvict)) {
                add_to_likes(mob,tmpvict);
                cprintf(ch, "%s has been added to %s's likes list.\n\r", GET_NAME(tmpvict),
                         GET_NAME(mob));
            } else {
                send_to_char("They alread like that person.\n\r", ch);
                return;
            }
        } else
            send_to_char("Unable to add to likes list, cannot find target.\n\r", ch);
        return;

        break;
    case 51:
       set_mtdesc(ch, mob);
       return;
    case 52:                   /* QFLAG */
        set_mqflag(ch, mob, buf3, TRUE);
        return;

    case 53:                   /* START CITY */
        if (!*buf3) {
            send_to_char("Syntax: cset <person> startcity <city>\n\r", ch);
            send_to_char("The following cities are currently defined:\n\r", ch);
            send_to_char("-------------------------------------------\n\r", ch);
            for (ct = 0; ct < MAX_CITIES; ct++) {
                sprintf(buf, "%s -- start room: %d, tribe: %d\n\r", city[ct].name, city[ct].room,
                        city[ct].tribe);
                send_to_char(buf, ch);
            }

            return;
        }

        for (ct = 0; ct < MAX_CITIES; ct++)
            if (!strnicmp(city[ct].name, buf3, strlen(buf3)))
                break;
        if (ct == MAX_CITIES) {
            send_to_char("That city does not exist.\n\r", ch);
            send_to_char("The following cities are currently defined:\n\r", ch);
            send_to_char("-------------------------------------------\n\r", ch);
            for (ct = 0; ct < MAX_CITIES; ct++) {
                sprintf(buf, "%s -- start room: %d, tribe: %d\n\r", city[ct].name, city[ct].room,
                        city[ct].tribe);
                send_to_char(buf, ch);
            }
            return;
        }

        cprintf(ch, "%s's (%s) startcity has been changed from %s to %s.\n\r", MSTR(mob, short_descr),
                mobnum, city[mob->player.start_city].name, city[ct].name);

        mob->player.start_city = ct;

        break;


    default:
        send_to_char("Invalid field.\n\r", ch);
        return;
    }

    send_to_char("Done.\n\r", ch);
}


/* duplicate a mobile, including equipment and stuff */
void
cmd_mdup(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *mob, *dup;
    OBJ_DATA *obj;
    char buf[256];
    int i;

    argument = one_argument(argument, buf, sizeof(buf));
    if (!(mob = get_char_room_vis(ch, buf))) {
        send_to_char("You don't see that mobile here.\n\r", ch);
        return;
    }
    if (!IS_NPC(mob)) {
        send_to_char("You can only duplicate NPCs.\n\r", ch);
        return;
    }
    dup = read_mobile(mob->nr, REAL);
    char_to_room(dup, mob->in_room);

    for (obj = mob->carrying; obj; obj = obj->next_content)
        obj_to_char(read_object(obj->nr, REAL), dup);

    for (i = 0; i < MAX_WEAR; i++)
        if (mob->equipment[i] && (mob->equipment[i]->nr != 0))
            equip_char(dup, read_object(mob->equipment[i]->nr, REAL), i);

    if( !IS_SWITCHED(ch)) {
        act("$n creates a duplicate of $N.", TRUE, ch, 0, mob, TO_ROOM);
        act("You create a duplicate of $N.", TRUE, ch, 0, mob, TO_CHAR);
    }
}


/* update a mobile to the online database */
void
cmd_mupdate(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *mob;
    char buf[256];

    SWITCH_BLOCK(ch);

    if (!ch->desc)
        return;

    argument = one_argument(argument, buf, sizeof(buf));
    if (!(mob = get_char_room_vis(ch, buf))) {
        send_to_char("You don't see that mobile here.\n\r", ch);
        return;
    }
    if (!IS_NPC(mob)) {
        send_to_char("That person is not an npc.\n\r", ch);
        return;
    }
    if (!has_privs_in_zone(ch, npc_index[mob->nr].vnum / 1000, MODE_STR_EDIT)) {
        send_to_char("You do not have creation privs in that zone.\n\r", ch);
        return;
    }

    npc_update(mob);
    send_to_char("Done.\n\r", ch);
};


/* save the mobile database for a zone to disk */
void
cmd_msave(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char buf[256], err[256];
    int zone;

    SWITCH_BLOCK(ch);

    argument = one_argument(argument, buf, sizeof(buf));

    if (!is_number(buf)) {
        sprintf(err, "Msave requires a number, '%s' is not a number.\n\r", buf);
        send_to_char(err, ch);
        return;
    }

    zone = atoi(buf);

    if (zone < 0) {
        send_to_char("Msave requires a positive number.\n\r", ch);
        return;
    }

    if (!*buf)
        zone = ch->in_room->zone;
    else if (!stricmp(buf, "all") && (GET_LEVEL(ch) >= HIGHLORD)) {
        send_to_char("Npc save in progress...\n\r", ch);
        for (zone = 0; zone < top_of_zone_table; zone++)
            save_mobiles(zone);
        send_to_char("Npc save completed.\n\r", ch);
    }

    if (!has_privs_in_zone(ch, zone, MODE_STR_EDIT)) {
        send_to_char("You don't have creation privs in that zone.\n\r", ch);
        return;
    }

    send_to_char("Npc save in progress...\n\r", ch);
    save_mobiles(zone);
    send_to_char("Npc save completed.\n\r", ch);
}


/* delete a mobile */
void
cmd_cdelete(CHAR_DATA * ch, const char *arg, Command cmd, int count)
{
    char arg1[256];
    int nr, virt, i, j;
    CHAR_DATA *npc, *next_npc;

    SWITCH_BLOCK(ch);

    arg = one_argument(arg, arg1, sizeof(arg1));
    if (*arg1 == '\0') {
        send_to_char("Usage: cdelete <nr>\n\r", ch);
        return;
    }

    nr = real_mobile(virt = atoi(arg1));
    if (!has_privs_in_zone(ch, virt / 1000, MODE_STR_EDIT)) {
        send_to_char("You do not have creation privs in that zone.\n\r", ch);
        return;
    }

    if (nr == -1) {
        send_to_char("No such npc.\n\r", ch);
        return;
    }


    for (i = 0; i <= top_of_zone_table; i++)
        for (j = 0; zone_table[i].cmd && zone_table[i].cmd[j].command != 'S'; j++)
            switch (zone_table[i].cmd[j].command) {
            case 'M':
                if (zone_table[i].cmd[j].arg1 == nr) {
                    send_to_char("Cannot delete NPC in zone table.\n\r", ch);
                    return;
                }
                break;
            }


    /* fix/destroy existing npcs */
    for (npc = character_list; npc; npc = next_npc) {
        next_npc = npc->next;
        if (IS_NPC(npc)) {
            if (npc->nr == nr)
                extract_char(npc);
            else if (npc->nr > nr)
                npc->nr--;
        }
    }

    for (i = nr + 1; i < top_of_npc_t; i++) {
        npc_index[i - 1] = npc_index[i];
        npc_default[i - 1] = npc_default[i];
    }

    top_of_npc_t -= 1;

    for (i = 0; i <= top_of_zone_table; i++)
        for (j = 0; zone_table[i].cmd && zone_table[i].cmd[j].command != 'S'; j++)
            switch (zone_table[i].cmd[j].command) {
            case 'M':
                if (zone_table[i].cmd[j].arg1 > nr)
                    zone_table[i].cmd[j].arg1--;
                if (zone_table[i].cmd[j].arg1 == nr) {
                    gamelog("Error:Deleted a mobile still in the command tables");
                };
                break;
            default:
                break;
            };

    update_shopkeepers_d(nr);

    send_to_char("Character deleted.\n\r", ch);
}

bool
is_all_alpha(const char *str) {
   int len = strlen(str);
   int i = 0;
   for (; i < len; i++ ) {
      if( !isalpha(*(str + i)) ) 
         return FALSE;
   }
   return TRUE;
}

/* show statistics of a mobile or player */

void
cmd_cstat(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    struct time_info_data playing_time;
    /* struct time_info_data player_age; */
    char tmp[256], type[256], arg1[256], arg2[256];
    char buf[MAX_STRING_LENGTH], output[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    CHAR_DATA *vict;
    struct affected_type *af;
    struct guard_type *tmp_guard;
    struct plyr_list *tmp_rider;
    struct extra_descr_data *e_tmp;
    int ct;
    CLAN_DATA *clan;
    bool show_affects = TRUE;
    bool brief_mode = FALSE;
    CHAR_DATA *staff = SWITCHED_PLAYER(ch);

    argument = one_argument(argument, arg1, sizeof(arg1));

    if (!*arg1) {
        send_to_char("You must supply an argument", ch);
        return;
    }

    if (!stricmp(arg1, "-s")) {
        show_affects = FALSE;
        argument = one_argument(argument, arg1, sizeof(arg1));
    } else if (!stricmp(arg1, "-b")) {
        brief_mode = TRUE;
        argument = one_argument(argument, arg1, sizeof(arg1));
    }

    argument = one_argument(argument, arg2, sizeof(arg2));

    if (!(vict = get_char_vis(ch, arg1))) {
        send_to_char("No character by that name exists.\n\r", ch);
//        return;
    }
    // Must mean to cstat an offline PC
    if (strlen(arg2) > 0) {
        if (GET_LEVEL(staff) < HIGHLORD) {
            send_to_char("You must be a highlord to do that!\n\r", ch);
            return;
        }

        if( !is_all_alpha(arg1) ) {
           cprintf( ch, "Name '%s' must be all letters.\n\r", arg1);
           return;
        }

        if( !is_all_alpha(arg2) ) {
           cprintf( ch, "Account name '%s' must be all letters.\n\r", arg2);
           return;
        }

        global_game_stats.npcs++;
        CREATE(vict, CHAR_DATA, 1);     // = read_mobile(1109, VIRTUAL);
        clear_char(vict);
        if (!vict) {
            errorlog("out of memory in cstat");
            return;
        }
        vict->nr = 1109;
        arg2[0] = toupper(arg2[0]);

        vict->player.info[0] = arg2;
        if (load_char(arg1, vict) < 0) {
            free(vict);
            return;
        }

        vict->desc = NULL;
        MUD_SET_BIT(vict->specials.act, CFL_ISNPC);
        MUD_SET_BIT(vict->specials.act, CFL_SENTINEL);

        add_to_character_list(vict);
        char_to_room(vict, ch->in_room);
        if (load_char_mnts_and_objs(vict) < 0) {
            extract_char(vict);
            return;
        }

        if (vict->player.info[0] == arg2)       // Didn't get set from the load, which means we failed
            vict->player.info[0] = NULL;        //  then make sure we don't free() it later

        update_pos(vict);
    }

    if (!vict) {
        send_to_char("Couldn't find a target.\n\r", ch);
        return;
    }

    strcpy(output, "");

    if (IS_NPC(vict)) {

        sprintf(buf,
                "Character number %d, of zone %d (%s), Limit: %d.\n\rSaved in room:  %d\n\r\n\r",
                npc_index[vict->nr].vnum, npc_index[vict->nr].vnum / 1000,
                zone_table[npc_index[vict->nr].vnum / 1000].name, npc_index[vict->nr].limit,
                vict->saved_room);
    } else {                    /* Level check put in for Ness - Kelvik */
        if (GET_LEVEL(staff) < QUESTMASTER) {
            send_to_char("You cannot do that.\n\r", ch);
            return;
        }
        sprintf(buf, "Character name: '%s'   Account: '%s'\n\r\n\r", MSTR(vict, name),
                vict->account);
    }

    string_safe_cat(output, buf, MAX_STRING_LENGTH);
    sprintf(buf, " Thirst:%d(Max:%d), Hunger:%d(Max:%d)," " Drunken:%d(Max:%d)\n\r",
            (int) (GET_COND(vict, THIRST)), get_max_condition(vict, THIRST),
            (int) (GET_COND(vict, FULL)), get_max_condition(vict, FULL),
            (int) (GET_COND(vict, DRUNK)), get_max_condition(vict, DRUNK));
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    memory_print_for(vict, buf, MAX_STRING_LENGTH, ch);
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    display_specials(buf, ARRAY_LENGTH(buf), vict->specials.programs, 1);
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    sprintf(buf, "Character keyword list: '%s'\n\r",
            IS_NPC(vict) ? MSTR(vict, name) : vict->player.extkwds);
    string_safe_cat(output, buf, MAX_STRING_LENGTH);


    sprintf(buf, "Character short description: '%s'\n\r", MSTR(vict, short_descr));
    string_safe_cat(output, buf, MAX_STRING_LENGTH);


    sprintf(buf, "Character long description: %s\n\r",
            vict->long_descr ? (strlen(vict->long_descr) >
                                90 ? "Too long for buffer" : vict->long_descr)
            : "Code Generated Long Description");
    string_safe_cat(output, buf, MAX_STRING_LENGTH);


    if (IS_IMMORTAL(vict))
        sprintf(buf, "%s is an Immortal %s, of the rank %s.\n\r", MSTR(vict, short_descr),
                ((GET_SEX(vict) ==
                  SEX_MALE) ? "man" : ((GET_SEX(vict) == SEX_FEMALE) ? "woman" : "creature")),
                level_name[(int) vict->player.level]);
    else
        sprintf(buf, "%s is a mere mortal %s.\n\r", MSTR(vict, short_descr),
                ((GET_SEX(vict) ==
                  SEX_MALE) ? "man" : ((GET_SEX(vict) == SEX_FEMALE) ? "woman" : "creature")));
    string_safe_cat(output, buf, MAX_STRING_LENGTH);


    sprintf(buf, "Race: %s   Guild: %s   SubGuild: %s   ", race[(int) vict->player.race].name,
            guild[(int) vict->player.guild].name, sub_guild[(int) vict->player.sub_guild].name);
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    sprintf(buf, "Age: %d years\n\r", age(vict).year);
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    for (clan = vict->clan; clan; clan = clan->next) {
        int c = clan->clan;
        char *rname = get_clan_rank_name(c, clan->rank);

        if (clan->clan > 0 && clan->clan < MAX_CLAN) {
            char *flags = show_flags(clan_job_flags, clan->flags);
            sprintf(buf, "Tribe: %d (%s) %s%s%sjobs: %s\n\r", clan->clan, 
             clan_table[clan->clan].name, rname ? "rank: " : "",
             rname ? rname : "", rname ? " " : "", flags);
            free(flags);
        } else
            sprintf(buf, "Tribe: %d (Unknown)\n\r", clan->clan);

        string_safe_cat(output, buf, MAX_STRING_LENGTH);
    }

    sprintf(buf, "Karma: %d   Hometown: %d (%s)   Start City: %d (%s)\n\r",
            vict->desc ? vict->desc->player_info->karma : 0, GET_HOME(vict),
            city[GET_HOME(vict)].name, GET_START(vict), city[GET_START(vict)].name);
    string_safe_cat(output, buf, MAX_STRING_LENGTH);


    sprintf(buf, "Weight: %d ten-stone  Height: %d inches  Size: %d  Armor: %d (%d) points\n\r",
            vict->player.weight, vict->player.height, get_char_size(vict), vict->abilities.armor,
            GET_ARMOR(vict));
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    sprintf(buf, "Health: %d/%d  Mana: %d/%d  Stamina: %d/%d  Stun: %d/%d\n\r", GET_HIT(vict),
            GET_MAX_HIT(vict), GET_MANA(vict), GET_MAX_MANA(vict), GET_MOVE(vict),
            GET_MAX_MOVE(vict), GET_STUN(vict), GET_MAX_STUN(vict));
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    sprintf(buf, "Str: %d   Agil: %d   Wis: %d   End: %d\n\r", GET_STR(vict), GET_AGL(vict),
            GET_WIS(vict), GET_END(vict));
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    sprintf(buf, "Coins Carried: %d   In bank: %d\n\r", GET_OBSIDIAN(vict), vict->points.in_bank);
    string_safe_cat(output, buf, MAX_STRING_LENGTH);

    if (!IS_NPC(vict)) {
        playing_time =
            real_time_passed((time(0) - vict->player.time.logon) + vict->player.time.played, 0);
        sprintf(buf, "Playing time: %d days and %d hours.\n\r", playing_time.day,
                playing_time.hours);
        string_safe_cat(output, buf, MAX_STRING_LENGTH);
    }

    if (vict->specials.act) {
        sprintf(buf, "Character Flags: \n\r");
        string_safe_cat(output, buf, MAX_STRING_LENGTH);

        sprint_flag(vict->specials.act, char_flag, tmp);
        sprintf(buf, "     %s\n\r\n\r", tmp);
        string_safe_cat(output, buf, MAX_STRING_LENGTH);
    } else {
        sprintf(buf, "Character Flags: None.\n\r");
        string_safe_cat(output, buf, MAX_STRING_LENGTH);
    }

    if (!IS_NPC(vict) && vict->quit_flags) {
        sprintf(buf, "Quit Flags: \n\r");
        string_safe_cat(output, buf, MAX_STRING_LENGTH);

        sprint_flag(vict->quit_flags, quit_flag, tmp);
        sprintf(buf, "     %s\n\r\n\r", tmp);
        string_safe_cat(output, buf, MAX_STRING_LENGTH);
    } else if( !IS_NPC(vict) ) {
        sprintf(buf, "Quit Flags: None.\n\r");
        string_safe_cat(output, buf, MAX_STRING_LENGTH);
    }

    if (vict->specials.affected_by) {
        sprintf(buf, "Affect Flags: \n\r");
        string_safe_cat(output, buf, MAX_STRING_LENGTH);

        sprint_flag(vict->specials.affected_by, affect_flag, tmp);
        sprintf(buf, "     %s\n\r\n\r", tmp);
        string_safe_cat(output, buf, MAX_STRING_LENGTH);
    } else {
        sprintf(buf, "Affect Flags: None.\n\r");
        string_safe_cat(output, buf, MAX_STRING_LENGTH);
    }

    if (vict->specials.guarding) {
        sprintf(buf, "Guarding char %s:\n\r", GET_NAME(vict->specials.guarding));
        string_safe_cat(output, buf, MAX_STRING_LENGTH);
    }
    if (is_char_watching(vict)) {
        if (is_char_watching_any_dir(vict)) {
            sprintf(buf, "Watching exit %s.\n\r", dirs[get_char_watching_dir(vict)]);
            string_safe_cat(output, buf, MAX_STRING_LENGTH);
        }
        if (is_char_watching_any_char(vict)) {
            sprintf(buf, "Watching %s.\n\r", GET_NAME(get_char_watching_victim(vict)));
            string_safe_cat(output, buf, MAX_STRING_LENGTH);
        }
    }

    if (vict->specials.dir_guarding != -1) {
        sprintf(buf, "Guarding exit %d:\n\r", vict->specials.dir_guarding);
        string_safe_cat(output, buf, MAX_STRING_LENGTH);
    }
    if (vict->specials.obj_guarding) {
        sprintf(buf, "Guarding obj %s:\n\r", OSTR(vict->specials.obj_guarding, short_descr));

        string_safe_cat(output, buf, MAX_STRING_LENGTH);
    }

    for (tmp_rider = vict->riders; tmp_rider; tmp_rider = tmp_rider->next) {
        sprintf(buf, "Ridden by %s:\n\r", GET_NAME(tmp_rider->ch));
        string_safe_cat(output, buf, MAX_STRING_LENGTH);
    }

    for (tmp_guard = vict->guards; tmp_guard; tmp_guard = tmp_guard->next) {
        sprintf(buf, "Guarded by %s:\n\r", GET_NAME(tmp_guard->guard));
        string_safe_cat(output, buf, MAX_STRING_LENGTH);
    }
    if (vict->affected && show_affects) {
        sprintf(buf, "Character affected by the following:\n\r");
        string_safe_cat(output, buf, MAX_STRING_LENGTH);

        for (af = vict->affected; af; af = af->next) {
            if (af->type == TYPE_CRIMINAL)
                strcpy(type, "criminal");
            else if ((af->type > SPELL_NONE) && (af->type < (MAX_SKILLS + 1)))
                strcpy(type, skill_name[af->type]);
            else
                strcpy(type, "?");

            if (af->type == TYPE_CRIMINAL)
                sprintf(buf, "     %s%s[%s] for %d hours\n\r",
                        (af->power < 0
                         || af->power > MAX_CRIMES) ? "" : Crime[af->power], (af->power < 0
                                                                              || af->power >
                                                                              MAX_CRIMES) ? "" :
                        " ", type, af->duration);
            else
                sprintf(buf, "     %s%s[%s] for %d hours\n\r",
                        (af->power - 1 < 0
                         || af->power - 1 > LAST_POWER) ? "" : Power[af->power - 1],
                        (af->power - 1 < 0
                         || af->power - 1 > LAST_POWER) ? "" : " ", type, af->duration);

            string_safe_cat(output, buf, MAX_STRING_LENGTH);

            if (!brief_mode) {
                if (af->type != TYPE_CRIMINAL && af->magick_type != MAGICK_NONE) {
                    if ((af->magick_type > 0) && (af->magick_type < 5)) {
                        sprintf(buf, "          cast by %s%s at %s power\n\r",
                                indefinite_article(MagickType[af->magick_type]),
                                MagickType[af->magick_type], (af->power - 1 < 0
                                                              || af->power - 1 >
                                                              LAST_POWER) ? "" : Power[af->power -
                                                                                       1]);
                        string_safe_cat(output, buf, MAX_STRING_LENGTH);
                    };
                }

                sprint_attrib(af->location, obj_apply_types, tmp);
                sprintf(buf, "          %s by %ld\n\r", tmp, af->modifier);
                if (af->type == TYPE_CRIMINAL) {
                    buf2[0] = '\0';
                    for (ct = 0; ct < MAX_CITIES; ct++) {
                        if (city[ct].c_flag == af->modifier) {
                            if (strlen(buf2)) {
                                strcat(buf2, ", ");
                                strcat(buf2, city[ct].name);
                            } else
                                sprintf(buf2, "        wanted in: %s", city[ct].name);
                        }
                    }
                    if (strlen(buf2))
                        sprintf(buf, "%s\n\r", buf2);
                }
                string_safe_cat(output, buf, MAX_STRING_LENGTH);

                sprint_flag(af->bitvector, affect_flag, tmp);
                sprintf(buf, "          flags  : %s\n\r", tmp);
                string_safe_cat(output, buf, MAX_STRING_LENGTH);
                sprintf(buf, "          started: %s", ctime(&(af->initiation)));
                string_safe_cat(output, buf, MAX_STRING_LENGTH);
                sprintf(buf, "          expires: %s", ctime(&(af->expiration)));
                string_safe_cat(output, buf, MAX_STRING_LENGTH);
            }
        }
    }

    if (vict->ex_description) {
        string_safe_cat(output, "Extra Descriptions:\n\r", MAX_STRING_LENGTH);
        for (e_tmp = vict->ex_description; e_tmp; e_tmp = e_tmp->next) {
            string_safe_cat(output, (e_tmp->def_descr ? "  -  " : "     "), MAX_STRING_LENGTH);
            string_safe_cat(output, e_tmp->keyword, MAX_STRING_LENGTH);
            string_safe_cat(output, "\n\r", MAX_STRING_LENGTH);
        }
    }

    heap_print(buf, vict, event_ch_tar_ch_cmp);
    string_safe_cat(output, buf, MAX_STRING_LENGTH);
    send_to_char(output, ch);

    if( vict->queued_commands ) {
        if (!brief_mode) {
            cprintf( ch, "Queued Commands:\n\r" );
	    show_queued_commands( ch, vict );
	} else {
	    cprintf (ch, "There are %d commands queued.\n\r", num_queued_commands(vict));
	}
    }
}

