/*
 * File: SP_PARSER.C
 * Usage: Spell-casting and spell definitions.
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

/* Revision History:
 * 05/05/2000: Poison spell now castable on objects in room. -Sanvean
 * 05/15/2000: Detect poison similarly now castable on objects in the
 *             room.  -San
 * 05/16/2000: When you cast the potion reach, it adds the GLOW flag
 *             to the object if it doesn't have it already.  -San
 */

#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <math.h>

#include "constants.h"
#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "db.h"
#include "limits.h"
#include "parser.h"
#include "psionics.h"
#include "skills.h"
#include "handler.h"
#include "guilds.h"
#include "cities.h"
#include "utility.h"
#include "vt100c.h"
#include "modify.h"
#include "event.h"

/* Comment out if the ranged spell bombs */
#define RANGED_SPELLS

int update_disease(CHAR_DATA * ch, struct affected_type *af);
sh_int get_magick_type(CHAR_DATA * ch);

void check_gather_flag(CHAR_DATA * ch);
void remove_summoned(CHAR_DATA * mount);
extern bool ch_starts_falling_check(CHAR_DATA *ch);

#define SPELL_HEADER byte level, CHAR_DATA *ch, const char *arg, int si, \
                     CHAR_DATA *tar_ch, struct obj_data *tar_obj
/*   0 */
/*   1 */
void cast_armor(SPELL_HEADER);
/*   2 */
void cast_teleport(SPELL_HEADER);
/*   3 */
void cast_blind(SPELL_HEADER);
/*   4 */
void cast_create_food(SPELL_HEADER);
/*   5 */
void cast_create_water(SPELL_HEADER);
/*   6 */
void cast_cure_blindness(SPELL_HEADER);
/*   7 */
void cast_detect_invisibility(SPELL_HEADER);
/*   8 */
void cast_detect_magick(SPELL_HEADER);
/*   9 */
void cast_detect_poison(SPELL_HEADER);
/*  10 */
void cast_earthquake(SPELL_HEADER);
/*  11 */
void cast_fireball(SPELL_HEADER);
/*  12 */
void cast_heal(SPELL_HEADER);
/*  13 */
void cast_invisibility(SPELL_HEADER);
/*  14 */
void cast_lightning_bolt(SPELL_HEADER);
/*  15 */
void cast_poison(SPELL_HEADER);
/*  16 */
void cast_remove_curse(SPELL_HEADER);
/*  17 */
void cast_sanctuary(SPELL_HEADER);
/*  18 */
void cast_sleep(SPELL_HEADER);
/*  19 */
void cast_strength(SPELL_HEADER);
/*  20 */
void cast_summon(SPELL_HEADER);
/*  21 */
void cast_rain_of_fire(SPELL_HEADER);
/*  22 */
void cast_cure_poison(SPELL_HEADER);
/*  23 */
void cast_relocate(SPELL_HEADER);
/*  24 */
void cast_fear(SPELL_HEADER);
/*  25 */
void cast_refresh(SPELL_HEADER);
/*  26 */
void cast_levitate(SPELL_HEADER);
/*  27 */
void cast_light(SPELL_HEADER);
/*  28 */
void cast_animate_dead(SPELL_HEADER);
/*  29 */
void cast_dispel_magick(SPELL_HEADER);
/*  30 */
void cast_calm(SPELL_HEADER);
/*  31 */
void cast_infravision(SPELL_HEADER);
/*  32 */
void cast_flamestrike(SPELL_HEADER);
/*  33 */
void cast_sand_jambiya(SPELL_HEADER);
/*  34 */
void cast_stone_skin(SPELL_HEADER);
/*  35 */
void cast_weaken(SPELL_HEADER);
/*  36 */
void cast_gate(SPELL_HEADER);
/*  37 */
void cast_oasis(SPELL_HEADER);
/*  38 */
void cast_send_shadow(SPELL_HEADER);
/*  39 */
void cast_show_the_path(SPELL_HEADER);
/*  40 */
void cast_demonfire(SPELL_HEADER);
/*  41 */
void cast_sandstorm(SPELL_HEADER);
/*  42 */
void cast_hands_of_wind(SPELL_HEADER);
/*  43 */
void cast_ethereal(SPELL_HEADER);
/*  44 */
void cast_detect_ethereal(SPELL_HEADER);
/*  45 */
void cast_banishment(SPELL_HEADER);
/*  46 */
void cast_burrow(SPELL_HEADER);
/*  47 */
void cast_empower(SPELL_HEADER);
/*  48 */
void cast_feeblemind(SPELL_HEADER);
/*  49 */
void cast_fury(SPELL_HEADER);
/*  50 */
void cast_guardian(SPELL_HEADER);
/*  51 */
void cast_glyph(SPELL_HEADER);
/*  52 */
void cast_transference(SPELL_HEADER);
/*  53 */
void cast_invulnerability(SPELL_HEADER);
/*  54 */
void cast_determine_relationship(SPELL_HEADER);
/*  55 */
void cast_mark(SPELL_HEADER);
/*  56 */
void cast_mount(SPELL_HEADER);
/*  57 */
void cast_pseudo_death(SPELL_HEADER);
/*  58 */
void cast_psionic_suppression(SPELL_HEADER);
/*  59 */
void cast_restful_shade(SPELL_HEADER);
/*  60 */
void cast_fly(SPELL_HEADER);
/*  61 */
void cast_stalker(SPELL_HEADER);
/*  62 */
void cast_thunder(SPELL_HEADER);
/*  63 */
void cast_dragon_drain(SPELL_HEADER);
/*  64 */
void cast_wall_of_sand(SPELL_HEADER);
/*  65 */
void cast_rewind(SPELL_HEADER);
/*  66 */
void cast_pyrotechnics(SPELL_HEADER);
/*  67 */
void cast_insomnia(SPELL_HEADER);
/*  68 */
void cast_tongues(SPELL_HEADER);
/*  69 */
void cast_charm_person(SPELL_HEADER);
/*  70 */
void cast_alarm(SPELL_HEADER);
/*  71 */
void cast_feather_fall(SPELL_HEADER);
/*  72 */
void cast_shield_of_nilaz(SPELL_HEADER);
/*  73 */
void cast_deafness(SPELL_HEADER);
/*  74 */
void cast_silence(SPELL_HEADER);
/*  75 */
void cast_solace(SPELL_HEADER);
/*  76 */
void cast_slow(SPELL_HEADER);
/*  77 */
void psi_contact(SPELL_HEADER);
/*  78 */
void psi_barrier(SPELL_HEADER);
/*  79 */
void psi_locate(SPELL_HEADER);
/*  80 */
void psi_probe(SPELL_HEADER);
/*  81 */
void psi_trace(SPELL_HEADER);
/*  82 */
void psi_expel(SPELL_HEADER);
/*  83 */
void cast_dragon_bane(SPELL_HEADER);
/*  84 */
void cast_repel(SPELL_HEADER);
/*  85 */
void cast_health_drain(SPELL_HEADER);
/*  86 */
void cast_aura_drain(SPELL_HEADER);
/*  87 */
void cast_stamina_drain(SPELL_HEADER);
/*  88 */
void skill_peek(SPELL_HEADER);
/*  89 */
/* night vision */
/*  90 */
void cast_godspeed(SPELL_HEADER);
/*  91 */
/*  92 */
void psi_cathexis(SPELL_HEADER);
/*  93 */
void psi_empathy(SPELL_HEADER);
/*  94 */
void psi_comprehend_langs(SPELL_HEADER);
/*  95 */
void psi_mindwipe(SPELL_HEADER);
/*  96 */
void psi_shadow_walk(SPELL_HEADER);
/*  97 */
void psi_hear(SPELL_HEADER);
/*  98 */
void psi_control(SPELL_HEADER);
/*  99 */
void psi_compel(SPELL_HEADER);
/* 100 */
void psi_conceal(SPELL_HEADER);
/* 101 */
/* 102 */
/* 103 */
/* 104 */
/* 105 */
/* 106 */
/* 107 */
/* 108 */
/* 109 */
/* 110 */
/* 111 */
/* void skill_sneak (SPELL_HEADER); */
/* 112 */
void skill_hide(SPELL_HEADER);
/* 113 */
void skill_steal(SPELL_HEADER);
/* 114 */
void skill_backstab(SPELL_HEADER);
/* 115 */
void skill_pick(SPELL_HEADER);
/* 116 */
void skill_kick(SPELL_HEADER);
/* 117 */
void skill_bash(SPELL_HEADER);
/* 118 */
void skill_rescue(SPELL_HEADER);
/* 119 */
void skill_disarm(SPELL_HEADER);
/* 120 */
/* 121 */
void skill_listen(SPELL_HEADER);
/* 122 */
void skill_trap(SPELL_HEADER);
/* 123 */
void skill_hunt(SPELL_HEADER);
/* 124 */
/* 125 */
/* 126 */
void skill_bandage(SPELL_HEADER);
/* 127 */
/* 128 */
void skill_throw(SPELL_HEADER);
/* 129 */
void skill_subdue(SPELL_HEADER);
/* 130 */
void skill_scan(SPELL_HEADER);
/* 131 */
void skill_search(SPELL_HEADER);
/* 132 */
void skill_value(SPELL_HEADER);
/* 133 */
/* 134 */
/* 135 */
void skill_poisoning(SPELL_HEADER);
/* 136 */
void skill_guard(SPELL_HEADER);
/* 137 */
/* 138 */
/* 139 */
/* 140 */
/* 141 */
void spice_methelinoc(SPELL_HEADER);
/* 142 */
void spice_melem_tuek(SPELL_HEADER);
/* 143 */
void spice_krelez(SPELL_HEADER);
/* 144 */
void spice_kemen(SPELL_HEADER);
/* 145 */
void spice_aggross(SPELL_HEADER);
/* 146 */
void spice_zharal(SPELL_HEADER);
/* 147 */
void spice_thodeliv(SPELL_HEADER);
/* 148 */
void spice_krentakh(SPELL_HEADER);
/* 149 */
void spice_qel(SPELL_HEADER);
/* 150 */
void spice_moss(SPELL_HEADER);
/* 151 */
void rw_lang_common(SPELL_HEADER);
/* 152 */
void rw_lang_nomad(SPELL_HEADER);
/* 153 */
void rw_lang_elvish(SPELL_HEADER);
/* 154 */
void rw_lang_dwarvish(SPELL_HEADER);
/* 155 */
void rw_lang_halfling(SPELL_HEADER);
/* 156 */
void rw_lang_mantis(SPELL_HEADER);
/* 157 */
void rw_lang_galish(SPELL_HEADER);
/* 158 */
void rw_lang_merchant(SPELL_HEADER);
/* 159 */
void rw_lang_ancient(SPELL_HEADER);
/* 160 */
void psi_dome(SPELL_HEADER);
/* 161 */
void psi_clairaudience(SPELL_HEADER);
/* 162 */
void psi_masquerade(SPELL_HEADER);
/* 163 */
void psi_illusion(SPELL_HEADER);
/* 164 */
/* 165 */
/* 166 */
/* 167 */
/* 168 */
/* 169 */
void skill_brew(SPELL_HEADER);
/* 170 */
void cast_create_darkness(SPELL_HEADER);
/* 171 */
void cast_paralyze(SPELL_HEADER);
/* 172 */
void cast_chain_lightning(SPELL_HEADER);
/* 173 */
void cast_lightning_storm(SPELL_HEADER);
/* 174 */
void cast_energy_shield(SPELL_HEADER);
/* 175 */
/* 176 */
void cast_curse(SPELL_HEADER);
/* 177 */
void cast_fluorescent_footsteps(SPELL_HEADER);
/* 178 */
void cast_regenerate(SPELL_HEADER);
/* 179 */
void cast_firebreather(SPELL_HEADER);
/* 180 */
void cast_parch(SPELL_HEADER);
/* 181 */
void cast_travel_gate(SPELL_HEADER);
/* 182 */
/* 183 */
void cast_forbid_elements(SPELL_HEADER);
/* 184 */
void cast_turn_element(SPELL_HEADER);
/* 185 */
void cast_portable_hole(SPELL_HEADER);
/* 186 */
void cast_elemental_fog(SPELL_HEADER);
/* 187 */
void cast_planeshift(SPELL_HEADER);
/* 188 */
void cast_quickening(SPELL_HEADER);
/* 189 */
/* 190 */
/* 191 */
/* 192 */
/* 193 */
/* 194 */
/* 195 */
/* 196 */
/* 197 */
/* 198 */
/* 199 */
/* 200 */
void tol_alcohol(SPELL_HEADER);
/* 201 */
void tol_generic(SPELL_HEADER);
/* 202 */
void tol_grishen(SPELL_HEADER);
/* 203 */
void tol_skellebain(SPELL_HEADER);
/* 204 */
void tol_methelinoc(SPELL_HEADER);
/* 205 */
void tol_terradin(SPELL_HEADER);
/* 206 */
void tol_peraine(SPELL_HEADER);
/* 207 */
void tol_heramide(SPELL_HEADER);
/* 208 */
void tol_plague(SPELL_HEADER);
/* 209 */
void tol_pain(SPELL_HEADER);

/* 243 */
void skill_split_opponents(SPELL_HEADER);
/* 246 */
void skill_sap(SPELL_HEADER);
/* 247 */
void cast_possess_corpse(SPELL_HEADER);
/* 248 */
void cast_portal(SPELL_HEADER);
/* 249 */
void cast_sand_shelter(SPELL_HEADER);

/* 250-315 is the hit vs. race type skills */

/* 316 */
void skill_forage(SPELL_HEADER);
/* 317 */
void skill_nesskick(SPELL_HEADER);
/* 318 */
void skill_flee(SPELL_HEADER);
/* 319 */
void cast_golem(SPELL_HEADER);
/* 320 */
void cast_fire_jambiya(SPELL_HEADER);
/* 321 */
/* 322 */
/* 323 */
/* 324 */
/* 325 */
/* 326 */
/* 327 */
/* 328 */
/* 329 */
/* 330 */
/* 331 */
/* 332 */
/* 333 */
/* 334 */
/* 335 */
/* 336 */
/* 337 */
void cast_identify(SPELL_HEADER);
/* 338 */
void cast_wall_of_fire(SPELL_HEADER);
void cast_wall_of_wind(SPELL_HEADER);
void cast_blade_barrier(SPELL_HEADER);
void cast_wall_of_thorns(SPELL_HEADER);
void cast_wind_armor(SPELL_HEADER);
void cast_fire_armor(SPELL_HEADER);
void cast_shadow_armor(SPELL_HEADER);
void skill_repair(SPELL_HEADER);
void psi_sense_presence(SPELL_HEADER);
void psi_suggestion(SPELL_HEADER);
void psi_babble(SPELL_HEADER);
void psi_disorient(SPELL_HEADER);
void psi_clairvoyance(SPELL_HEADER);
void psi_imitate(SPELL_HEADER);
void psi_project_emotion(SPELL_HEADER);
void psi_vanish(SPELL_HEADER);
void psi_coercion(SPELL_HEADER);
/* 370 */
void cast_haunt(SPELL_HEADER);
void cast_create_wine(SPELL_HEADER);
void cast_wind_fist(SPELL_HEADER);
void cast_lightning_whip(SPELL_HEADER);
void cast_shield_of_mist(SPELL_HEADER);
void cast_shield_of_wind(SPELL_HEADER);
void cast_drown(SPELL_HEADER);
void cast_healing_mud(SPELL_HEADER);
void cast_cause_disease(SPELL_HEADER);
void cast_acid_spray(SPELL_HEADER);
void cast_puddle(SPELL_HEADER);
void cast_shadow_sword(SPELL_HEADER);
void cast_fireseed(SPELL_HEADER);
/* 376 */
void cast_psionic_drain(SPELL_HEADER);
void cast_intoxication(SPELL_HEADER);
void cast_sober(SPELL_HEADER);
void cast_wither(SPELL_HEADER);
void cast_delusion(SPELL_HEADER);
void cast_illuminant(SPELL_HEADER);
void cast_mirage(SPELL_HEADER);
void cast_phantasm(SPELL_HEADER);
void cast_shadowplay(SPELL_HEADER);
/* 386 */
void cast_sandstatue(SPELL_HEADER);
/* 413 */
void psi_magicksense(SPELL_HEADER);
/* 414 */
void psi_mindblast(SPELL_HEADER);
/* 415 */
void skill_charge(SPELL_HEADER);
/* 422 */
void cast_disembody(SPELL_HEADER);
/* 423 */
void skill_axe_making(SPELL_HEADER);
/*424 */
void skill_club_making(SPELL_HEADER);
/* 425 */
void skill_weaving(SPELL_HEADER);
void cast_breathe_water(SPELL_HEADER);
void cast_lightning_spear(SPELL_HEADER);
/* 431 */
void cast_cure_disease(SPELL_HEADER);
/* 446 */
void psi_mesmerize(SPELL_HEADER);
/* 447 */
void cast_shatter(SPELL_HEADER);
/* 448 */
void cast_messenger(SPELL_HEADER);
/* 449 */
void skill_gather(SPELL_HEADER);
/* 450 */
void psi_rejuvenate(SPELL_HEADER);
/* 451 - Blind Fighting */
/* 452 */
void cast_daylight(SPELL_HEADER);
/* 453 */
void cast_dispel_invisibility(SPELL_HEADER);
/* 454 */
void cast_dispel_ethereal(SPELL_HEADER);
/* 455 */
void cast_repair_item(SPELL_HEADER);
/* 456 */
void cast_create_rune(SPELL_HEADER);
/* 457 - Watch */
/* 458 */
void cast_immolate(SPELL_HEADER);
/* 467 */
void cast_rot_items(SPELL_HEADER);
/* 468 */
void cast_vampiric_blade(SPELL_HEADER);
/* 469 */
void cast_dead_speak(SPELL_HEADER);
/* 488 */
void cast_hero_sword(SPELL_HEADER);
/* 489 */
void cast_recite(SPELL_HEADER);
/* 500 */
void skill_trample(SPELL_HEADER);

/* Following function will return 1 if both ch and victim have the same
 * "ethereal status" (both are ethereal or both are not ethereal).  It
 * will return 0 if their "ethereaL status" is different (one is ethereal
 * and the other is not).*/
int
ethereal_ch_compare(CHAR_DATA * ch, CHAR_DATA * victim)
{

    if ((!ch) || (!victim))
        return 1;

    if ((IS_AFFECTED(ch, CHAR_AFF_ETHEREAL))
        && (IS_AFFECTED(victim, CHAR_AFF_ETHEREAL)))
        return 1;

    if ((!IS_AFFECTED(ch, CHAR_AFF_ETHEREAL))
        && (!IS_AFFECTED(victim, CHAR_AFF_ETHEREAL)))
        return 1;

    if ((IS_AFFECTED(ch, CHAR_AFF_ETHEREAL))
        && (!IS_AFFECTED(victim, CHAR_AFF_ETHEREAL))) {
        if (char_can_see_char(ch, victim))
            send_to_char("You cannot affect them while you are ethereal.\n\r", ch);
        return 0;
    }

    if ((!IS_AFFECTED(ch, CHAR_AFF_ETHEREAL))
        && (IS_AFFECTED(victim, CHAR_AFF_ETHEREAL))) {
        if (char_can_see_char(ch, victim))
            send_to_char("You cannot affect someone who is ethereal.\n\r", ch);
        return 0;
    }

    return 0;

}


int
imbue_item(CHAR_DATA * ch, struct obj_data *obj, int reach, int lvl, int spl)
{
    struct affected_type af;
    int n, j;
    int spot;
    char rw_lang[120];

    memset(&af, 0, sizeof(af));

    switch (obj->obj_flags.type) {
    case ITEM_POTION:
        if (reach != REACH_POTION) {
            act("Your magick has no effect on $p.", TRUE, ch, obj, 0, TO_CHAR);
            return FALSE;
        }
        if (obj->obj_flags.value[0])
            lvl = MIN(obj->obj_flags.value[0], lvl);
        if (obj->obj_flags.value[1] == 0)
            spot = 1;
        else if (obj->obj_flags.value[2] == 0)
            spot = 2;
        else if (obj->obj_flags.value[3] == 0)
            spot = 3;
        else {
            send_to_char("The potion can contain no more magick.\n\r", ch);
            return 0;
        }
        obj->obj_flags.value[spot] = spl;
        obj->obj_flags.value[0] = lvl;

/* When an object gets the potion reach set on it, check to see  */
/* if it has the eflag GLOW, if not, then put it on it.  -San    */

        if (!IS_SET(obj->obj_flags.extra_flags, OFL_GLOW)) {
            MUD_SET_BIT(obj->obj_flags.extra_flags, OFL_GLOW);
        }
        break;
    case ITEM_WAND:
        if (reach != REACH_WAND) {
            act("Your magick has no effect on $p.", TRUE, ch, obj, 0, TO_CHAR);
            return FALSE;
        }


	char buf[100];
	if (get_obj_extra_desc_value(obj, "[ARTIFACT]", buf , 100)) {
	  act ("$p resists your attempts to alter it.", TRUE, ch, obj, 0, TO_CHAR);
	  return FALSE;
	}

        char edesc_title[200];

	!IS_NPC(ch) ? 
	  sprintf(edesc_title, "[ENCHANTER_%s_%s]", ch->account, ch->name)
	  :
	  sprintf(edesc_title, "[ENCHANTER_NPC_%d]", npc_index[ch->nr].vnum);

	set_obj_extra_desc_value(obj, edesc_title, "1");


        obj->obj_flags.value[0] = lvl * 100 + 1100 ;    /* max-mana */
        obj->obj_flags.value[1] = lvl;  /* level    */
        obj->obj_flags.value[3] = spl;  /* spell    */
        break;
    case ITEM_STAFF:
        if (reach != REACH_STAFF) {
            act("Your magick has no effect on $p.", TRUE, ch, obj, 0, TO_CHAR);
            return FALSE;
        }
        obj->obj_flags.value[0] = lvl * 500;    /* max-mana */
        obj->obj_flags.value[1] = lvl;  /* level    */
        obj->obj_flags.value[3] = spl;  /* spell    */
        break;

        /* oops, hasn't been defined in structs.h yet --brian */
    case ITEM_SCROLL:
        if (reach != REACH_SCROLL) {
            act("Your magick has no effect on $p.", TRUE, ch, obj, 0, TO_CHAR);
            return FALSE;
        }
        if (obj->obj_flags.value[1] == 0)
            n = 1;
        else if (obj->obj_flags.value[2] == 0)
            n = 2;
        else if (obj->obj_flags.value[3] == 0)
            n = 3;
        else {
            send_to_char("No room for more spells.\n\r", ch);
            return FALSE;
        }

        if ((obj->obj_flags.value[0] != lvl) && (obj->obj_flags.value[0] > 0)) {
            if (obj->obj_flags.value[0] > lvl)
                send_to_char("You're casting at too low of a power level for " "this scroll.\n\r",
                             ch);
            else
                send_to_char("You're casting at too high a power level for " "this scroll.\n\r",
                             ch);
            return FALSE;
        } else
            obj->obj_flags.value[0] = lvl;

        sprintf(rw_lang, "RW %s", skill_name[GET_SPOKEN_LANGUAGE(ch)]);

        for (j = 1; j < MAX_SKILLS; j++)
            if (!strnicmp(skill_name[j], rw_lang, strlen(rw_lang)))
                break;
        if (j == MAX_SKILLS) {
            send_to_char("There's no written form of the language you're speaking.\n\r", ch);
            return FALSE;
        }

        if (!has_skill(ch, j)) {
            sprintf(rw_lang, "You don't know how to write in %s (your spoken language).",
                    skill_name[GET_SPOKEN_LANGUAGE(ch)]);
            send_to_char(rw_lang, ch);
            return FALSE;
        }

        if ((obj->obj_flags.value[5] != j) && (obj->obj_flags.value[5] > 0)) {
            send_to_char("You're not speaking the correct language to add "
                         "spells to this scroll.\n\r", ch);
            return FALSE;
        } else
            obj->obj_flags.value[5] = j;

        obj->obj_flags.value[n] = spl;

        break;

    default:
        act("Your magick has no effect on $p.", TRUE, ch, obj, 0, TO_CHAR);
        return 0;
    }

    if (ch->skills[spl]->rel_lev > 0)
        ch->skills[spl]->rel_lev--;

    act("You weave magick into $p.", FALSE, ch, obj, 0, TO_CHAR);
    act("$n weaves magick into $p.", TRUE, ch, obj, 0, TO_ROOM);

    if ((!does_save(ch, SAVING_SPELL, wis_app[GET_WIS(ch) / 2].wis_saves))
        && (!IS_IMMORTAL(ch))) {
        af.type = SPELL_SLEEP;
        af.duration = lvl + 1;
        af.power = lvl;
        af.magick_type = get_magick_type(ch);
        af.modifier = 0;
        af.location = CHAR_APPLY_NONE;
        af.bitvector = CHAR_AFF_SLEEP;
        affect_join(ch, &af, FALSE, FALSE);
        if (GET_POS(ch) > POSITION_SLEEPING) {
            act("Magick drains from your body, and you fall into a deep sleep.", FALSE, ch, 0, 0,
                TO_CHAR);
            act("$n collapses, falling into a deep sleep.", TRUE, ch, 0, 0, TO_ROOM);
            set_char_position(ch, POSITION_SLEEPING);
        }
    }
    return TRUE;
}

void
remove_summoned(CHAR_DATA * mount)
{
    char message[MAX_STRING_LENGTH];

    if (affected_by_spell(mount, SPELL_MOUNT)) {
        if (!IS_NPC(mount))
            act("$n slowly disintegrates into a pile of sand.", FALSE, mount, 0, 0, TO_ROOM);
        else {
            if ((npc_index[mount->nr].vnum >= 101) && (npc_index[mount->nr].vnum <= 107))       /* Krathi 101-107 */
                sprintf(message, "$n crumples into a pile of charred embers.");
            else if ((npc_index[mount->nr].vnum >= 108) && (npc_index[mount->nr].vnum <= 114))  /*vivadu 108-114 */
                sprintf(message, "$n's form fades until it is a fine mist and " "dissapates.");
            else if ((npc_index[mount->nr].vnum >= 115) && (npc_index[mount->nr].vnum <= 121))  /* whira 115-121 */
                sprintf(message,
                        "$n draws a deep breath and then exhales at "
                        "length, until its body fades from view.");
            else if ((npc_index[mount->nr].vnum >= 122) && (npc_index[mount->nr].vnum <= 128))  /* drov 122-128 */
                sprintf(message, "$n falls back into its own shadow, which " "shrinks from view.");
            else if ((npc_index[mount->nr].vnum >= 129) && (npc_index[mount->nr].vnum <= 135))  /*elkros 129-135 */
                sprintf(message, "$n explodes in a shower of sparks.");
            else if ((npc_index[mount->nr].vnum >= 136) && (npc_index[mount->nr].vnum <= 142))  /* nilaz 136-142 */
                sprintf(message, "$n fades from view as a silence falls upon " "the area.");
            else if ((npc_index[mount->nr].vnum >= 1302) && (npc_index[mount->nr].vnum <= 1308))        /*ruk 1302-1308 */
                sprintf(message, "$n crumples into a pile of sand.");
            else if ((npc_index[mount->nr].vnum >= 1309) && (npc_index[mount->nr].vnum <= 1315))        /*templar1309-1315 */
                sprintf(message, "$n abruptly shatters into a small pile of " "obsidian flakes.");
            else
                sprintf(message, "$n falls in upon itself, dissapearing.");
            act(message, FALSE, mount, 0, 0, TO_ROOM);
        }
    }

    if (affected_by_spell(mount, SPELL_SANDSTATUE)) {
        act("$n suddenly explodes into a mass of sand and dust!", TRUE, mount, 0, 0, TO_ROOM);
    }

    if (affected_by_spell(mount, SPELL_SEND_SHADOW)) {
        act("$n dissipates into wisps of darkness.", TRUE, mount, 0, 0, TO_ROOM);
        act("You are abruptly pulled back to your own body.", TRUE, mount, 0, 0, TO_CHAR);
    }

    if (affected_by_spell(mount, SPELL_GUARDIAN))
        act("$n melts into a pool of ooze.", FALSE, mount, 0, 0, TO_ROOM);

    if (affected_by_spell(mount, SPELL_ANIMATE_DEAD))
        act("$n slowly decomposes, and turns to dust.", FALSE, mount, 0, 0, TO_ROOM);

    if (affected_by_spell(mount, SPELL_GATE)) {
        switch (GET_RACE(mount)) {
        case RACE_DEMON:
            act("$n dissolves, as it returns to its plane of existence.", FALSE, mount, 0, 0,
                TO_ROOM);
            break;
        case RACE_ELEMENTAL:
            act("$n flickers dimly, and returns to its elemental plane.", FALSE, mount, 0, 0,
                TO_ROOM);
            break;
        default:
            act("$n dissolves.", FALSE, mount, 0, 0, TO_ROOM);
            break;
        }
    }
    /* End if gate */
    remove_all_affects_from_char(mount);
    extract_char(mount);
}

int
affect_update(CHAR_DATA *ch)
{
    OBJ_DATA *tmp_object;
    char buf[256];
    static struct affected_type *af, *next_af_dude;
    bool alive;
    long crimflag;
    long permflags = 0;
    int pos = 0;
    int chance;
    time_t delta;
    bool changed;
    int duration;
    
    if (!IS_NPC(ch))
        check_idling(ch);
    alive = TRUE;

    // PSIONIC AFFECTS
    /* Needs to be handled separately as they accumulate stun drain */
    if (IS_AFFECTED(ch, CHAR_AFF_PSI))
        if( !update_psionics(ch) )
            return FALSE;

    // Spice Withdrawl  (FIXME:  Should be moved to update_spice() -- X)
    if (in_spice_wd(ch)) {
        set_hit(ch, MIN(GET_HIT(ch), GET_MAX_HIT(ch)));
        set_stun(ch, MIN(GET_STUN(ch), GET_MAX_STUN(ch)));
        set_mana(ch, MIN(GET_MANA(ch), GET_MAX_MANA(ch)));
        set_move(ch, MIN(GET_MOVE(ch), GET_MAX_MOVE(ch)));

        if (!number(0, 5)) {
            chance = number(MIN_SPICE, MAX_SPICE);
            if (has_skill(ch, chance) && is_skill_taught(ch, chance)
             && !spice_in_system(ch, chance)) {
                cprintf(ch, "%s\n\r", spell_wear_off_msg[chance]);
            }
        }
    }

    /* Take damage in sector water_plane if not affected by Water Breathing
     * or have the edesc [BREATHE_WATER] (for natural water breathers)
     */
    if (ch->in_room && ch->in_room->sector_type == SECT_WATER_PLANE
     && !affected_by_spell(ch, SPELL_BREATHE_WATER)
     && !IS_IMMORTAL(ch)
     && !find_ex_description("[BREATHE_WATER]", ch->ex_description, TRUE)
     && !(GET_RACE(ch) == RACE_ELEMENTAL 
      && GET_GUILD(ch) == GUILD_WATER_ELEMENTALIST)) {
        if (!number(0, 10)) {
            set_stun(ch, MAX(0, GET_STUN(ch) - 20));
            cprintf(ch, "Water gets into your lungs as you slowly drown.\n\r");
            act("$n breathes in some water, slowly drowning.", TRUE, ch, 0, 0, TO_ROOM);
            if (GET_STUN(ch) <= 0) {
                cprintf(ch, "Your lungs completely fill with water, and the world goes dark.\n\r");
                act("$n's body goes limp as $e drowns to death.", TRUE, ch, 0, 0, TO_ROOM);
                /* Ch is dead, so let's leave a death message */
                if( !IS_NPC(ch) ) {
                    free(ch->player.info[1]);
                    sprintf(buf, "%s has drowned to death in room %d.  Glug glug.", GET_NAME(ch), ch->in_room->number);
                    ch->player.info[1] = strdup(buf);
                    gamelog(buf);
                }
                die(ch);
                return FALSE;
            }
        }
    }

    // mul rage check: If they've run out of stamina remove the rage affect.
    if (GET_RACE(ch) == RACE_MUL && affected_by_spell(ch, AFF_MUL_RAGE) && GET_MOVE(ch) <= 0)  {
        if (spell_wear_off_msg[AFF_MUL_RAGE] && *spell_wear_off_msg[AFF_MUL_RAGE])
           cprintf(ch, "%s\n\r", spell_wear_off_msg[AFF_MUL_RAGE]);
        affect_from_char(ch, AFF_MUL_RAGE);
    }


    // go over affects
    for (af = ch->affected; (af && (alive == TRUE)); af = next_af_dude) {
        /* might have to check if they are still in the playerlist at this 
         * point, cause they might die from having 
         * mount/gate/animate_dead/guardian removed off of them */
        /* this is one of the reasons I put them  dying from these affects
         * into the event_heap thingy, because it does not have this problem
         * with char pointers becomeing lost in these loops.
         */
//            sprintf(buf, "aff_update: %s, %d", GET_NAME(ch), af->type);
//            shhlog(buf);
        next_af_dude = af->next;

        /* heramide inhibits knock out from recovering */
        if (af->type == SPELL_SLEEP && af->magick_type == MAGICK_NONE
         && affected_by_spell(ch, POISON_HERAMIDE))
            continue;

        delta = af->expiration - time(NULL);
        if (delta > 0) {
            duration = (delta / RT_ZAL_HOUR) + 1;
            changed = (af->duration != duration);
            af->duration = duration;

            // Update Spice at duration markers
            if (changed && af->type >= MIN_SPICE && af->type <= MAX_SPICE)
                update_spice(ch, af);

            // Magick updates all the time, but tell them if it passed
            if (af->magick_type != MAGICK_NONE) {
                alive = update_magick(ch, af, changed);
            }
            if( !alive ) return FALSE;

            // might be nice to have different way of figuring out poison
            if (af->type == SPELL_POISON
             || (af->type >= POISON_GENERIC && af->type <= POISON_SKELLEBAIN_3)) {
                alive = update_poison(ch, af, changed);
            }
            if( !alive ) return FALSE;

            // Update any diseases they might have
            if (af->type >= DISEASE_BOILS && af->type <= DISEASE_YELLOW_PLAGUE) {
                alive = update_disease(ch, af);
            }
            if( !alive ) return FALSE;

            continue;
        }
        // otherwise it expired
        af->duration = 0;

        /* Expired, clean up and remove.  Some helper functions call
         * affect_remove(), so when they do we have to continue the
         * loop and not let it attempt to access the now-removed affect
         */

        // Clean up spice affects
        if ((af->type >= MIN_SPICE) && (af->type <= MAX_SPICE)) {
            update_spice(ch, af);
            continue;
        }

        // Clean up diseases
        if ((af->type >= DISEASE_BOILS) && (af->type <= DISEASE_YELLOW_PLAGUE)) {
            update_disease(ch, af);
            continue;
        }

        if( af->type == POISON_GENERIC ) {
            affect_remove(ch, af);
            // still some in the system?
            if( suffering_from_poison(ch, POISON_GENERIC) )  
                parse_command(ch, "feel a bit better");
            else  // all done, give them the end message
                cprintf(ch, "%s\n\r", spell_wear_off_msg[POISON_GENERIC]);
            continue;
        }

        // Generic affect clean up
        if ((af->type > 0) && (af->type < MAX_SKILLS))
            if (!af->next || af->next->type != af->type 
             || af->next->duration > 0)
                if (spell_wear_off_msg[af->type] 
                 && *spell_wear_off_msg[af->type]
                 && (af->type != SPELL_SLEEP
                  || (af->type == SPELL_SLEEP 
                   && af->magick_type != MAGICK_NONE))) {
                    cprintf(ch, "%s\n\r", spell_wear_off_msg[af->type]);
                }

        // Clean up contact pointer, other psi affects
        if (af->type == PSI_CONTACT) {
            remove_contact(ch);
            continue;       // Continue here, because we're already done
        }

        // Clean up people shifting out of the ethereal plane
        if (af->type == SPELL_ETHEREAL) {
            for (tmp_object = ch->carrying; tmp_object; tmp_object = tmp_object->next_content) {
                act("You bring $p out of the shadow plane with you.", TRUE, ch, tmp_object, 0,
                    TO_CHAR);
                REMOVE_BIT(tmp_object->obj_flags.extra_flags, OFL_ETHEREAL);
            }

            for (pos = 0; pos < MAX_WEAR; pos++) {
                if (ch->equipment && ch->equipment[pos]) {
                    tmp_object = ch->equipment[pos];
                    if (IS_SET(tmp_object->obj_flags.extra_flags, OFL_ETHEREAL)) {
                        act("You bring $p into the shadow plane with you.", TRUE, ch,
                         tmp_object, 0, TO_CHAR);
                        REMOVE_BIT(tmp_object->obj_flags.extra_flags, OFL_ETHEREAL);
                    }
                }
            }
        }

        // Clean up summoned NPCs
        if ((af->type == SPELL_MOUNT) || (af->type == SPELL_SANDSTATUE)
         || (af->type == SPELL_GATE) || (af->type == SPELL_ANIMATE_DEAD)
         || (af->type == SPELL_GUARDIAN) || (af->type == SPELL_SEND_SHADOW)) {
            remove_summoned(ch);
            return FALSE;
        }
#ifndef TIERNAN_PLAGUE
        // Clean up the plague
        if (af->type == POISON_PLAGUE) {    /* The plague wears off */
            /* The black plague causes permanent brain damage
             * if allowed to run for a full term */
            if (GET_WIS(ch) > 0)
                (ch)->abilities.wis -= 1;
        }
#endif

        /* knocked out -Morg */
        if (af->type == SPELL_SLEEP && af->magick_type == MAGICK_NONE
         && GET_STUN(ch) <= 0) {
            affect_remove(ch, af);
            set_stun(ch, 1);
            /* update their position, they may have hp probs as well */
            update_pos(ch);

            /* if sleeping or better, wake them up */
            if (GET_POS(ch) >= POSITION_SLEEPING) {
                cprintf(ch,"Your head clears and your eyes flutter open.\n\r");
                act("$n's eyes flutter open.", FALSE, ch, NULL, NULL, TO_ROOM);
                set_char_position(ch, POSITION_RESTING);
                snprintf(buf, sizeof(buf), "%s wakes up.", MSTR(ch, short_descr));
                send_to_empaths(ch, buf);
            }

            continue;       /* already removed the affect, move on */
        }

        // Clean up crimflags
        if (af->type == TYPE_CRIMINAL) {
            crimflag = af->modifier;
            affect_remove(ch, af);
            /* Check to see if the affect just removed what
             * should be a permanent crimflag. Tiernan 6/8/2004 */
            if (get_char_extra_desc_value(ch, "[CRIM_FLAGS]", buf, 100)) {
                permflags = atol(buf);
                /* It was permanent, reset it. */
                if (IS_SET(permflags, crimflag)) {
                    MUD_SET_BIT(ch->specials.act, crimflag);
                    continue;
                }
            }
            // Basically, we should continue in either case,
            // it is a mistake to allow "affect_remove()" to run
            // again regardless of what happens here, since we ALWAYS
            // remove it at the top of this case
            continue;
        }

        // Fury is subsiding, tell the empaths
        if (af->type == SPELL_FURY) {
            snprintf(buf, sizeof(buf), "%s is no longer enraged.", 
             MSTR(ch, short_descr));
            send_to_empaths(ch, buf);
        }

        // Calm is subsiding, tell the empaths
        if (af->type == SPELL_CALM) {
            snprintf(buf, sizeof(buf), "%s feels less calm.", 
             MSTR(ch, short_descr));
            send_to_empaths(ch, buf);
        }

        // Paralyze is subsiding, tell the empaths
        if (af->type == SPELL_PARALYZE) {
            snprintf(buf, sizeof(buf), "%s is no longer paralyzed.", 
             MSTR(ch, short_descr));
            send_to_empaths(ch, buf);
        }
        // Fear is subsiding, tell the empaths
        if (af->type == SPELL_FEAR) {
            snprintf(buf, sizeof(buf), "%s is no longer terrified.", 
             MSTR(ch, short_descr));
            send_to_empaths(ch, buf);
        }
        affect_remove(ch, af);
    }

    init_saves(ch);

    // Added 10/22/2000 by nessalin because I couldn't see where
    // it should be.  This is to prevent spells from leaving you with
    // more stun points than you should have.

    set_stun(ch, MIN(GET_MAX_STUN(ch), GET_STUN(ch)));
    return TRUE;
}

/* Check if making CH follow VICTIM will create an illegal */
/* Follow "Loop/circle"                                    */
bool
circle_follow(CHAR_DATA * ch, CHAR_DATA * victim)
{
    CHAR_DATA *temp_char;

    for (temp_char = victim; temp_char; temp_char = temp_char->master) {
        if (temp_char == ch)
            return (TRUE);
    }

    return (FALSE);
}

bool
circle_follow_ch_obj(CHAR_DATA * ch, OBJ_DATA * obj_leader)
{
    struct follow_type *victim, *next_vic;

    for (victim = ch->followers; victim; victim = next_vic) {
        next_vic = victim->next;
        if (victim->follower && victim->follower->master && (victim->follower->master == ch)) {
            if (victim->follower->obj_master == obj_leader)
                return TRUE;
            if (victim->follower->specials.harnessed_to == obj_leader)
                return TRUE;
            if (victim->follower->followers)
                circle_follow_ch_obj(victim->follower, obj_leader);
        }
    }

    return (FALSE);


}

bool
circle_follow_obj_obj(OBJ_DATA * obj, OBJ_DATA * obj_leader)
{

    return (FALSE);
}

bool
circle_follow_obj_ch(OBJ_DATA * obj, CHAR_DATA * leader)
{
/*
  struct follow_type *victim, *next_vic;

  for (victim = obj->followers;
       victim;
       victim = next_vic)
    {
      next_vic = victim->next;
      if (victim->follower == leader)
        return (TRUE);
    }
  return (FALSE);
*/
    char buf[MAX_STRING_LENGTH];
    struct harness_type *victim;

    for (victim = obj->beasts; victim; victim = victim->next) {
        sprintf(buf, "checking %s, who is pulling %s.", MSTR(victim->beast, short_descr),
                OSTR(obj, short_descr));
/*      gamelog(buf); */
        if (victim->beast == leader)
            return (TRUE);
/*      gamelog("Checking if beast's master is original char."); */
        if (victim->beast->master == leader)
            return (TRUE);
        sprintf(buf, "Beast is not leader, enter new_circle_follow_ch_ch: %s %s",
                MSTR(leader, short_descr), MSTR(victim->beast, short_descr));
/*      gamelog(buf); */
        if (new_circle_follow_ch_ch(leader, victim->beast))
            return (TRUE);
    }
    return (FALSE);
}

bool
new_circle_follow_ch_ch(CHAR_DATA * ch, CHAR_DATA * leader)
{
/*  char buf[MAX_STRING_LENGTH];*/
    CHAR_DATA *temp_char;

    for (temp_char = leader; temp_char; temp_char = temp_char->master) {
/*      gamelogf( "Checking %s.", MSTR(temp_char, short_descr)); */
        if (temp_char == ch)
            return (TRUE);
/*      gamelogf("Not ch, checking for obj_master"); */
        if (temp_char->obj_master) {
/*          gamelogf( "obj_master found: %s.  Entering circle_follow_obj_ch",
                    OSTR(temp_char->obj_master, short_descr)); 
*/
            if (circle_follow_obj_ch(temp_char->obj_master, ch))
                return TRUE;
        }
    }

    return FALSE;
}


struct obj_data *
get_component_nr(CHAR_DATA * ch,        /* caster */
                 int num)
{                               /* components virtual number */
    struct obj_data *obj = (struct obj_data *) NULL;

    for (obj = ch->carrying; obj; obj = obj->next_content) {
        if (obj_index[obj->nr].vnum == num)
            return obj;
    }

    return ((struct obj_data *) 0);
}


struct obj_data *
get_component(CHAR_DATA * ch,   /* The caster     */
              int type,         /* sphere         */
              int power)
{                               /* level of spell *//* Begin get_component */
    struct obj_data *obj;

    /* need to subtract 1 from the arguments to account for the
     * way the list of spheres and moods goes */

    for (obj = ch->carrying; obj; obj = obj->next_content) {
        if ((obj->obj_flags.type == ITEM_COMPONENT) && (obj->obj_flags.value[0] == type - 1)
            && (obj->obj_flags.value[1] >= power - 1))
            return obj;
    }

    return ((struct obj_data *) 0);
}

int
get_componentB(CHAR_DATA * ch,  /* the caster         */
               int type,        /* sphere             */
               int power,       /* level of spell     */
               char *chsuccmesg,        /* succ message to ch */
               char *rmsuccmesg,        /* succ message to rm */
               char *chfailmesg)
{                               /* fail message to ch *//* Being get_componentB */
    struct obj_data *obj;

    /* Immortals don't need components */
    if (IS_IMMORTAL(ch))
        return TRUE;

    /* Neither do Templars */
    if (GET_GUILD(ch) == GUILD_TEMPLAR)
        return TRUE;

    if (GET_GUILD(ch) == GUILD_LIRATHU_TEMPLAR)
        return TRUE;

    if (GET_GUILD(ch) == GUILD_JIHAE_TEMPLAR)
        return TRUE;

    /* Neither do full Elementals or Fiends */
    if (GET_RACE(ch) == RACE_ELEMENTAL)
        return TRUE;

    if (GET_RACE(ch) == RACE_FIEND)
        return TRUE;

    /* need to subtract 1 from the arguments to account for the
     * way the list of spheres and moods goes */

    for (obj = ch->carrying; obj; obj = obj->next_content) {    /* loop searching for component */
        if ((obj->obj_flags.type == ITEM_COMPONENT) && (obj->obj_flags.value[0] == type - 1) && (obj->obj_flags.value[1] >= power - 1)) {       /* Component Found */

            /* send messages */
            act(chsuccmesg, FALSE, ch, obj, 0, TO_CHAR);
            act(rmsuccmesg, FALSE, ch, obj, 0, TO_ROOM);

            /* remove the object */
            extract_obj(obj);

            /* exit the program */
            return TRUE;
        }                       /* End component found */
    }                           /* end loop searching for component */

    /* Component not found */

    /* send the message */
    act(chfailmesg, FALSE, ch, obj, 0, TO_CHAR);

    /* exit the program */
    return 0;

}                               /* End get_componentB */


const char *
skip_spaces(const char *string)
{
    for (; *string && (*string) == ' '; string++);

    return (string);
}


void
cmd_skill(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{                               /* Cmd_Skill */
    struct obj_data *tar_obj;
    CHAR_DATA *tar_char;
    OBJ_DATA *wagobj = NULL;

    char name[256], oldarg[MAX_STRING_LENGTH];
    char msg[MAX_STRING_LENGTH];

    int nr;
    int bflag = FALSE;
    bool target_ok;
    bool throw_subdue = FALSE;

    for (nr = 1; nr < MAX_SKILLS; nr++) {
        if (!stricmp(command_name[cmd], skill_name[nr]))
            break;
    }

    if (nr == MAX_SKILLS) {
        send_to_char("What sort of skill is that?\n\r", ch);
        return;
    }

    if (skill[nr].min_pos > GET_POS(ch)) {
        switch (GET_POS(ch)) {
        case POSITION_FIGHTING:
            send_to_char("You cannot do this in the middle of a fight!\n\r", ch);
            break;
        case POSITION_SITTING:
        case POSITION_RESTING:
            send_to_char("You'll need to stand up first.\n\r", ch);
            break;
        case POSITION_SLEEPING:
            send_to_char("You find it awfully hard to do this while asleep.\n\r", ch);
            break;
        default:
            send_to_char("You're in no condition for doing that!\n\r", ch);
            break;
        }
        return;
    }

    /* Psionics check.  If they asked for a psionic ability but don't have
     * the skill, we want to keep the ability a mystery so we send "What?"
     * instead of revealing that the command works. -Tiernan */
    if (skill[nr].sk_class == CLASS_PSIONICS && !has_skill(ch, nr)) {
        send_to_char("What?\n\r", ch);
        return;
    }

    strcpy(oldarg, argument);
    argument = one_argument(argument, name, sizeof(name));

    tar_char = 0;
    tar_obj = 0;
    target_ok = FALSE;

    // We remove the "blind" bit for searching, then restore later 
    if (is_psi_skill(nr))
        if (TRUE == (bflag = (0 != (IS_AFFECTED(ch, CHAR_AFF_BLIND))))) {
            REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_BLIND);
        }

    /* originated for skill_throw, more applications are available */
    if (!target_ok && IS_SET(skill[nr].targets, TAR_CHAR_DIR)) {
        char arg1[MAX_STRING_LENGTH];
        char arg2[MAX_STRING_LENGTH];
        char tmparg[MAX_STRING_LENGTH];
        int dir = -1;

        /* if TAR_USE_EQUIP is also set, assume first arg is what to do it with */
        if (IS_SET(skill[nr].targets, TAR_USE_EQUIP)) {
            int i;

            for (i = 0; i < MAX_WEAR && !target_ok; i++) {
                if (ch->equipment[i]
                    && isname(name, OSTR(ch->equipment[i], name))
                    && ((nr == SKILL_THROW ? i == EP : TRUE)
                        || (nr == SKILL_THROW ? i == ES : TRUE))) {
                    tar_obj = ch->equipment[i];
                    target_ok = TRUE;
                }
            }

            /* if we didn't find an object, and it's throw... */
            if (!target_ok && nr == SKILL_THROW) {
                /* are they trying to throw someone? */
                if (!ch->specials.subduing
                    || (ch->specials.subduing != get_char_room_vis(ch, name))) {
                    send_to_char("Who or what do you want to throw?\n\r", ch);
                    return;
                }
                /* Remember that they're trying to throw who they have subdued -Morg */
                throw_subdue = TRUE;
            }
        }

        strcpy(tmparg, argument);
        if (throw_subdue)       /* shoving a subdued person */
            strcpy(arg1, name);
        else                    /* throwing a weapon/obj */
            argument = one_argument(argument, arg1, sizeof(arg1));      /* <victim>|<direction> */
        argument = one_argument(argument, arg2, sizeof(arg2));  /* <direction>|<wagon>  */

        // Moved all these here instead of rechecking them 500 times individually below
        // -Nessalin 11/27/2004
        if (*arg2) {
            wagobj = get_obj_room_vis(ch, arg2);
            dir = get_direction(ch, arg2);
            if ((dir == -1) && (!stricmp("out", arg2)))
                dir = DIR_OUT;
        }

        tar_char = get_char_room_vis(ch, arg1);
        tar_obj = get_obj_room_vis(ch, arg1);

        if ((target_ok) && (dir == -1) && (!wagobj)) {  /* have an object to use, but no direction or wagon , check room for target */
            if (tar_char == NULL) {     /* nope, target's no good */
                argument = tmparg;
                target_ok = FALSE;
            } else
                target_ok = TRUE;
        } else if (!target_ok && (dir == -1) && (!wagobj)) {    /* throwing character into a wagon obj? */
            if (throw_subdue && (tar_obj == NULL && str_prefix(arg1, "outside")))
                argument = tmparg;
            /* no object, no direction, check room for target */
            else if (!throw_subdue && (tar_char == NULL))
                /* nope, target's no good */
                argument = tmparg;
            else
                target_ok = TRUE;
        } else if (target_ok) { /* we have an obj to use */

            ROOM_DATA *tar_room;
            char arg4[MAX_STRING_LENGTH];
            dir = get_direction(ch, arg1);

            if (dir > -1) {     /* Throwing the object in a valid direction */
                if ((tar_room = get_room_distance(ch, 1, dir)) == NULL) {
                    send_to_char("You can't see anyone in that direction.\n\r", ch);
                    return;
                }

                if ((tar_char = get_char_other_room_vis(ch, tar_room, arg1)) == NULL) {
                    if (dir < 5)
                        sprintf(arg4, "to the %s", dir_name[dir]);
                    else
                        sprintf(arg4, "%s", dir_name[dir]);

                    sprintf(msg, "No one like that %s.\n\r", arg4);
                    send_to_char(msg, ch);
                    return;
                }

                /* at this point we have an object to use, a target character and
                 * a direction to do it in */
                tar_obj = NULL;
                target_ok = TRUE;
            } else if (wagobj) {        /* Throwing the object in a valid object */
                if (wagobj->obj_flags.type != ITEM_WAGON) {
                    send_to_char("You can't throw things into that.\n\r", ch);
                    return;
                }
                if (wagobj->obj_flags.value[0] == NOWHERE) {
                    send_to_char("You can't throw things into that.\n\r", ch);
                    return;
                }
                tar_room = get_room_num(wagobj->obj_flags.value[0]);
                if ((tar_char = get_char_other_room_vis(ch, tar_room, arg1)) == NULL) {
                    sprintf(msg, "You don't see anyone like that in %s.",
                            OSTR(wagobj, short_descr));
                    send_to_char(msg, ch);
                    return;
                }
                tar_obj = wagobj;
                target_ok = TRUE;
            }
        } else
            target_ok = TRUE;   /* throw <person> <direction> */
    }

    if (!target_ok && IS_SET(skill[nr].targets, TAR_CHAR_ROOM)) {
        if ((tar_char = get_char_room_vis(ch, name)))
            target_ok = TRUE;
    }

    if (!target_ok && IS_SET(skill[nr].targets, TAR_CHAR_WORLD)) {
        if (nr == SKILL_HUNT) {
            tar_char = get_char(name);
            target_ok = TRUE;
        } else {
            if (nr == PSI_CONTACT) {
                if (IS_IMMORTAL(ch)) {
                    send_to_char("Use send to communicate," " contact is for mortals.\n\r", ch);
                    return;
                } else if (is_immortal_name(name)) {
                    send_to_char("Use the wish command.\n\r", ch);
                    return;
                }
                /* did they specify #.<descriptor> ? */
                else if (isdigit(name[0])) {
                    /* give them a warning if they specified extra arguments */

                    if (strlen(argument) > 0) {
                        send_to_char("Extra arguments to contact are ignored"
                                     " when specifying #.<descriptor>.\n\rSee 'help contact'"
                                     " for more information.\n\r", ch);
                    }

                    if ((tar_char = get_char_world_raw(ch, name, FALSE))) {
                        target_ok = TRUE;
                    }
                }
                /* otherwise, use all of the keywords they passed to us */
                else if ((tar_char = get_char_world_all_name_raw(ch, oldarg, FALSE))) {
                    target_ok = TRUE;
                }
            } else if ((tar_char = get_char_vis(ch, name)))
                target_ok = TRUE;
        }
    }
    if (!target_ok && IS_SET(skill[nr].targets, TAR_CHAR_CONTACT)) {
        if ((tar_char = ch->specials.contact))
            target_ok = TRUE;
    }
    if (!target_ok && IS_SET(skill[nr].targets, TAR_OBJ_ROOM)) {
        if ((tar_obj = get_obj_in_list_vis(ch, name, ch->in_room->contents)))
            target_ok = TRUE;
    }
    if (!target_ok && IS_SET(skill[nr].targets, TAR_OBJ_INV)) {
        if ((tar_obj = get_obj_in_list_vis(ch, name, ch->carrying)))
            target_ok = TRUE;
    }
    if (!target_ok && IS_SET(skill[nr].targets, TAR_OBJ_WORLD)) {       /* banish needs to be able to take two arguments */
        if (nr == SPELL_BANISHMENT)
            argument = one_argument(argument, name, sizeof(name));
        if ((tar_obj = get_obj_vis(ch, name)))
            target_ok = TRUE;
    }
    if (!target_ok && IS_SET(skill[nr].targets, TAR_FIGHT_SELF)) {
        if (ch->specials.fighting) {
            tar_char = ch;
            target_ok = TRUE;
        }
    }
    if (!target_ok && IS_SET(skill[nr].targets, TAR_FIGHT_VICT))
        if ((tar_char = ch->specials.fighting))
            target_ok = TRUE;
    if (!target_ok && IS_SET(skill[nr].targets, TAR_IGNORE))
        target_ok = TRUE;

    // Restoring blind flag 
    if (TRUE == bflag) {
        MUD_SET_BIT(ch->specials.affected_by, CHAR_AFF_BLIND);
    }

    if (!target_ok) {
        if (nr == PSI_CONTACT) {
            /* This may knock 'em out, but we want to tell them why they
             * just passed out. -Tiernan */
            drain_char(ch, PSI_CONTACT);

            if (blocked_psionics(ch, NULL)) {
                WAIT_STATE(ch, PULSE_VIOLENCE * skill[nr].delay);
                return;
            }
            if (affected_by_spell(ch, PSI_CONTACT)) {
                send_to_char("You are already in contact with someone else.\n\r", ch);
                WAIT_STATE(ch, PULSE_VIOLENCE * skill[nr].delay);
                return;
            }
            // Changing this message to be just like a failed contact
            // since players are using it to gleam OOC information regarding
            // whether or not a PC is online. While this may seem harmless,
            // it has interfered with several plotlines and caused undo grief
            // for the immortals running those plots. -- Tiernan 6/5/2003
            //send_to_char("You've never heard of that person.\n\r", ch);
            send_to_char("You are unable to reach their mind.\n\r", ch);
            WAIT_STATE(ch, PULSE_VIOLENCE * skill[nr].delay);
            return;
        }

        if (IS_SET(skill[nr].targets, TAR_CHAR_ROOM))
            send_to_char("You don't see that person here.\n\r", ch);
        else if (IS_SET(skill[nr].targets, TAR_CHAR_WORLD))
            send_to_char("You've never heard of that person.\n\r", ch);
        else if (IS_SET(skill[nr].targets, TAR_OBJ_ROOM))
            send_to_char("You don't see that here.\n\r", ch);
        else if (IS_SET(skill[nr].targets, TAR_OBJ_WORLD))
            send_to_char("You've never heard of one of those.\n\r", ch);
        else if (IS_SET(skill[nr].targets, TAR_CHAR_DIR))
            send_to_char("You don't see that person in that direction.\n\r", ch);
        else if (IS_SET(skill[nr].targets, TAR_CHAR_CONTACT))
            send_to_char("You haven't established contact with anyone.\n\r", ch);
        return;
    }

    if ((ch == tar_char) && IS_SET(skill[nr].targets, TAR_SELF_NONO)) {
        send_to_char("You can't do this to yourself!\n\r", ch);
        return;
    }

    if (((nr == SKILL_BACKSTAB) || (nr == SKILL_PICK_LOCK) || (nr == SKILL_STEAL)
         || (nr == SKILL_SEARCH) || (nr == SKILL_FORAGE) || (nr == SKILL_THROW)
         || (nr == SKILL_HUNT) || (nr == SKILL_SAP) || (nr == SKILL_HIDE)
         || (nr == SKILL_SPLIT_OPPONENTS) || (nr == PSI_REJUVENATE))
        && (!ch->specials.to_process)) {
        add_to_command_q(ch, oldarg, cmd, 0, 0);
    } else {
        if ((ch->long_descr) && !(
                                     /* list of skills which DON'T reset the LDESC */
                                     (nr == PSI_CONTACT) || (nr == PSI_EXPEL) || (nr == PSI_LOCATE)
                                     || (nr == PSI_BARRIER) || (nr == SKILL_LISTEN)
                                     || (nr == SKILL_VALUE) || (nr == PSI_EXPEL)
                                     || (nr == PSI_LOCATE) || (nr == PSI_PROBE) || (nr == PSI_TRACE)
                                     || (nr == PSI_CATHEXIS) || (nr == PSI_EMPATHY)
                                     || (nr == PSI_COMPREHEND_LANGS) || (nr == PSI_MINDWIPE)
                                     || (nr == PSI_SHADOW_WALK) || (nr == PSI_HEAR)
                                     || (nr == PSI_COMPEL) || (nr == PSI_CLAIRVOYANCE)
                                     || (nr == PSI_CONCEAL) || (nr == PSI_DOME)
                                     || (nr == PSI_CLAIRAUDIENCE) || (nr == PSI_MASQUERADE)
                                     || (nr == SKILL_PILOT) || (nr == PSI_ILLUSION)
                                     || (nr == PSI_MAGICKSENSE) || (nr == PSI_MINDBLAST))) {
            WIPE_LDESC(ch);
        }

        if ((nr != SKILL_BACKSTAB) && (nr != SKILL_PICK_LOCK) && (nr != SKILL_HUNT)
            && (nr != SKILL_STEAL)) {
            if (!IS_IMMORTAL(ch)) {
                WAIT_STATE(ch, PULSE_VIOLENCE * skill[nr].delay);
            }
        }

        ((*skill[nr].function_pointer)
         (ch->skills[nr] ? ((ch->skills[nr]->learned * 7) / 100) : 0, ch, oldarg, 0, tar_char,
          tar_obj));
    }
}                               /* Cmd_Skill */


int
get_power(char *word)
{
    int i;
    for (i = 0; i < 7; i++)
        if (!stricmp(Power[i], word))
            return (i + 1);
    return (-1);
}

int
get_reach(char *word)
{
    int i;
    for (i = 0; i < LAST_REACH; i++)
        if (!stricmp(Reach[i], word))
            return (i + 1);
    return (-1);
}

int
get_element(char *word)
{
    int i;
    for (i = 0; i < 8; i++)
        if (!stricmp(Element[i], word))
            return (i + 1);
    return (-1);
}

int
get_sphere(char *word)
{
    int i;
    for (i = 0; i < 12; i++)
        if (!stricmp(Sphere[i], word))
            return (i + 1);
    return (-1);
}

int
get_mood(char *word)
{
    int i;
    for (i = 0; i < 11; i++)
        if (!stricmp(Mood[i], word))
            return (i + 1);
    return (-1);
}

void
find_spell(char *arg, int *power_return, int *reach_return, int *spell)
{
/*
  at the point this is called, words should be the string, not surrounded
  by quotes of the spell they want to cast.
  It will break them up into 5 different words.
   it will check to make sure only 5 were found, and return error (-1)
    if not.
  Next it will run through the 5 parts to the spells, once for each part.
  It will error out if it reaches the end and all 5 parts are not found.
*/

    int power, reach, element, sphere, mood;
    int i;
    char copy[MAX_INPUT_LENGTH];
    char *word[5];
    const char delimiters[2] = " ";

    power = -1;
    reach = -1;
    element = -1;
    sphere = -1;
    mood = -1;


    *power_return = -1;
    *reach_return = -1;
    *spell = -1;


    /* copy the string, becaose strtok will mangle it */
    strcpy(copy, arg);

    /* get the pointer to the first one */
    word[0] = strtok(copy, delimiters);
    if (word[0] == NULL)
        return;

    /* loop through till next is found */
    for (i = 1; (i < 5); i++) {
        word[i] = strtok(NULL, delimiters);
        if (word[i] == NULL)
            return;
    }

    for (i = 0; (i < 5) && power == -1; i++)
        power = get_power(word[i]);
    for (i = 0; (i < 5) && reach == -1; i++)
        reach = get_reach(word[i]);
    for (i = 0; (i < 5) && element == -1; i++)
        element = get_element(word[i]);
    for (i = 0; (i < 5) && sphere == -1; i++)
        sphere = get_sphere(word[i]);
    for (i = 0; (i < 5) && mood == -1; i++)
        mood = get_mood(word[i]);

    if ((power == -1) || (reach == -1) || (element == -1) || (sphere == -1) || (mood == -1))
        return;


    for (i = 1; *skill_name[i] != '\n'; i++)
        if (skill[i].sk_class == CLASS_MAGICK && skill[i].element > 0 && skill[i].sphere > 0
            && skill[i].mood > 0)
            if ((skill[i].element == element) && (skill[i].sphere == sphere)
                && (skill[i].mood == mood)) {
                sprintf(arg, "%s %s %s %s %s", word[0], word[1], word[2], word[3], word[4]);

                *power_return = power;
                *reach_return = reach;
                *spell = i;
                return;
            }

    *power_return = -1;
    *reach_return = -1;
    *spell = -1;
}

int
get_reach_skill(int reach)
{
    switch (reach) {
    case REACH_LOCAL:
        return SKILL_REACH_LOCAL;
    case REACH_NONE:
        return SKILL_REACH_NONE;
    case REACH_POTION:
        return SKILL_REACH_POTION;
    case REACH_SCROLL:
        return SKILL_REACH_SCROLL;
    case REACH_STAFF:
        return SKILL_REACH_STAFF;
    case REACH_WAND:
        return SKILL_REACH_WAND;
    case REACH_ROOM:
        return SKILL_REACH_ROOM;
    case REACH_SILENT:
        return SKILL_REACH_SILENT;
    default:
        return -1;
    }
}


bool
can_cast_spell(CHAR_DATA * ch, int power, int spl, int reach)
{
    int percent, vdiff, i, reach_sk;

    if (IS_IMMORTAL(ch))
        return TRUE;
    if (!ch->skills[spl])
        return FALSE;
    if ((vdiff = (ch->skills[spl]->rel_lev + 1)) < 0)
        return FALSE;

    /* Templars get their power from their sorc-king.  They will not fail much */
    /* Elementalists get their power from elementals.  They won't either */
    /* Sorc's rely on their own power.  Sucks to be them */
    if ((GET_GUILD(ch) == GUILD_TEMPLAR) || 
        (GET_GUILD(ch) == GUILD_LIRATHU_TEMPLAR) ||
        (GET_GUILD(ch) == GUILD_JIHAE_TEMPLAR)) {
      percent = 95;
    } else {
      if (is_guild_elementalist(GET_GUILD(ch))) {
        percent = 90;
      } else {
        percent = ch->skills[spl]->learned;
      }
    }

    // lookup a skill for the reach, if any
    if ((reach_sk = get_reach_skill(reach)) != -1) {
      // if you don't have skill in that reach, no chance of casting
      if (!has_skill(ch, reach_sk)) {
        return FALSE;
      }
      
      // chance is min of reach and real skill
      percent = MIN(percent, ch->skills[reach_sk]->learned);
    }

    if (affected_by_spell(ch, PSI_MINDWIPE))
        percent = 0;

    if (affected_by_spell(ch, POISON_SKELLEBAIN))
        percent -= 65;

    if (affected_by_spell(ch, POISON_SKELLEBAIN_2))
        percent -= 75;

    if (affected_by_spell(ch, POISON_SKELLEBAIN_3))
        percent -= 85;

    /* If the character is intoxicated, their ability to case spells will
     * be affected by how drunk they are.  */
    switch (is_char_drunk(ch)) {
    case 0:
        break;
    case DRUNK_LIGHT:
        percent = (percent * .85);
        break;
    case DRUNK_MEDIUM:
        percent = (percent * .75);
        break;
    case DRUNK_HEAVY:
        percent = (percent * .5);
        break;
    case DRUNK_SMASHED:
        percent = (percent * .3);
        break;
    case DRUNK_DEAD:
        percent = (percent * .1);
        break;
    }

    if (vdiff >= power)
        return (percent > number(1, 100));

    if ((GET_GUILD(ch) == GUILD_TEMPLAR) || (GET_GUILD(ch) == GUILD_LIRATHU_TEMPLAR)
        || (GET_GUILD(ch) == GUILD_JIHAE_TEMPLAR)) {
        send_to_char("You are unable to summon that much power.\n\r", ch);
        return FALSE;
    }

    for (i = 0; i < (power - vdiff); i++)
        percent = (percent / 2);

    return (percent > number(1, 100));
}


int
mana_amount_level(int vdiff, byte power, int spl)
{
    int base, tbase, pdiff;

    vdiff++;
    base = 50;
    if (vdiff == power)
        return base;
    else if (power > vdiff) {
        pdiff = power - vdiff;
        tbase = base / (pdiff + 1);
        base = 100 - tbase;
        return MAX(base, skill[spl].min_mana);
    } else {
        pdiff = vdiff - power;
        base = base / (pdiff + 1);
        return MAX(base, skill[spl].min_mana);
    }
}

int
mana_amount(CHAR_DATA * ch, int power, int spl)
{
    int vdiff;

    if (IS_IMMORTAL(ch))
        return 0;

    if ((vdiff = ch->skills[spl]->rel_lev) < 0)
        return GET_MAX_MANA(ch);
    return (mana_amount_level(vdiff, power, spl));
}



void
check_defiler_flag(CHAR_DATA * ch)
{
    int ct, dir;
    CHAR_DATA *tch;
    struct room_data *rm;
    struct affected_type af;

    memset(&af, 0, sizeof(af));

    if (GET_GUILD(ch) != GUILD_DEFILER)
        return;

    if (!(ct = room_in_city(ch->in_room)) || IS_SET(ch->specials.act, city[ct].c_flag)
        || IS_TRIBE(ch, city[ct].tribe))
        return;

    if (!IS_SET(ch->in_room->room_flags, RFL_POPULATED)) {
        for (tch = ch->in_room->people; tch; tch = tch->next_in_room)
            if (IS_TRIBE(tch, city[ct].tribe))
                break;
        for (dir = 0; (dir < 6) && !tch; dir++)
            if (ch->in_room->direction[dir] && (rm = ch->in_room->direction[dir]->to_room))
                for (tch = rm->people; tch; tch = tch->next_in_room)
                    if (IS_TRIBE(tch, city[ct].tribe))
                        break;
        if (!tch)
            return;
    }

    af.type = TYPE_CRIMINAL;
    af.location = CHAR_APPLY_CFLAGS;
    af.duration = number(24, 48);
    af.modifier = city[ct].c_flag;
    af.bitvector = 0;
    af.power = CRIME_DEFILING;

    affect_to_char(ch, &af);
}

/* returns true if marit is allowed to cast the spell.  In her current
   form, she can only cast the first level fire spelss, though she
   still knows an can teach the rest */
int
is_legal_marit_spell(int spell)
{

    if ((spell == SPELL_LIGHT) || (spell == SPELL_FLAMESTRIKE) || (spell == SPELL_FURY)
        || (spell == SPELL_DETECT_MAGICK))
        return 1;
    else
        return 0;
}

#define ROOM_REACH_PENALTY 2 // Lose this many levels of power on room reach castings
void
cmd_cast(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    struct obj_data *tar_obj;
    CHAR_DATA *tar_char, *persons, *next;
    struct room_data *char_room;
    char argtmp[MAX_STRING_LENGTH];
    char name[MAX_STRING_LENGTH], oldarg[MAX_STRING_LENGTH];
    char buf[MAX_STRING_LENGTH];
    /* char nbuf[MAX_STRING_LENGTH]; */
    char symbols[MAX_STRING_LENGTH];
    int qend = 0, power, reach, spl, i, percent;
    bool target_ok;
    int tmpamt;
    int cast_ok = 0;
    int reach_sk = -1;
    struct affected_type af;
    long permflags = 0;

    memset(&af, 0, sizeof(af));

    char_room = (struct room_data *) 0;
    next = (CHAR_DATA *) 0;
    persons = (CHAR_DATA *) 0;

    strcpy(oldarg, argument);
    while (*argument == ' ')
        argument++;

    /* If there is no chars in argument */
    if (!(*argument)) {
        send_to_char("Cast which what where?\n\r", ch);
        return;
    }
    if (ch->lifting) {
        sprintf(buf, "[Dropping %s first.]\n\r", format_obj_to_char(ch->lifting, ch, 1));
        send_to_char(buf, ch);
        if (!drop_lifting(ch, ch->lifting))
            return;
    }

    /* now for the all new, better than ever spell parser */
    if (*argument == '\'') {
        if ((qend = close_parens(argument, '\''))) {
            strcpy(argtmp, argument);
            sub_string(argtmp, symbols, 1, qend - 1);
            delete_substr(argtmp, 0, qend + 1);
            for (i = 0; *(symbols + i) != '\0'; i++)
                *(symbols + i) = LOWER(*(symbols + i));
            find_spell(symbols, &power, &reach, &spl);
        }
    }

    argument = argtmp;

    /* and the error msgs... */
    if (!qend) {
        send_to_char("Symbols must always be enclosed in single quotes.\n\r", ch);
        return;
    }
    if (IS_AFFECTED(ch, CHAR_AFF_SILENCE)) {
        act("$n waves around $s hands and mumbles words beneath $s breath.", TRUE, ch, 0, 0,
            TO_ROOM);
        act("You wave around your hands, but you can't recite the incantation.", FALSE, ch, 0, 0,
            TO_CHAR);
        return;
    }
    if (affected_by_spell(ch, PSI_BABBLE)) {
        act("$n waves around $s hands and babbles incoherently.", TRUE, ch, 0, 0, TO_ROOM);
        act("You wave around your hands and babble incoherently.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }
    if (spl == -1) {
        act("$n utters a meaningless incantation.", FALSE, ch, 0, 0, TO_ROOM);
        act("You utter a meaningless incantation.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }
    reach_sk = get_reach_skill(reach);
    if (!has_skill(ch, spl) || !has_skill(ch, reach_sk)) {
        act("$n utters a meaningless incantation.", FALSE, ch, 0, 0, TO_ROOM);
        act("You utter a meaningless incantation.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }
    if (!strcasecmp(MSTR(ch, name), "marit") && !is_legal_marit_spell(spl)) {
        act("$n utters a meaningless incantation.", FALSE, ch, 0, 0, TO_ROOM);
        act("You are unable to draw the power for that spell.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (IS_AFFECTED(ch, CHAR_AFF_SUBDUED)) {
        send_to_char("You are held tight, unable to cast magick!.\n\r", ch);
        return;
        }

    if (ch->specials.subduing) {
        act("You're too busy holding $N to cast right now.", FALSE, ch, 0, ch->specials.subduing,
            TO_CHAR);
        return;
    }

    if (affected_by_spell(ch, SPELL_FEEBLEMIND)) {
        send_to_char("You can't seem to concentrate enough to cast your" " magick.\n\r", ch);
        return;
    }

    if (affected_by_spell(ch, SPELL_SHIELD_OF_NILAZ)) {

        if (is_guild_elementalist(GET_GUILD(ch)) && (GET_GUILD(ch) != GUILD_VOID_ELEMENTALIST)) {
            send_to_char("You call upon your element...there is no answer.\n\r", ch);
            return;
        }
    }

    if (hands_free(ch) < 2) {
        cast_ok = 1;

        if ((ch->equipment[ES]) && (!IS_SET(ch->equipment[ES]->obj_flags.extra_flags, OFL_CAST_OK)))
            cast_ok--;

        if ((ch->equipment[EP]) && (!IS_SET(ch->equipment[EP]->obj_flags.extra_flags, OFL_CAST_OK)))
            cast_ok--;

        if ((ch->equipment[ETWO])
            && (!IS_SET(ch->equipment[ETWO]->obj_flags.extra_flags, OFL_CAST_OK)))
            cast_ok--;

        if (cast_ok < 1) {
            send_to_char("You'll need two free hands to properly weave the spell." "\n\r", ch);
            return;
        }
    }

    if (skill[spl].min_pos > POSITION_SLEEPING)
        if (IS_SET(ch->specials.affected_by, CHAR_AFF_HIDE))
            REMOVE_BIT(ch->specials.affected_by, CHAR_AFF_HIDE);

    if (!(IS_SPELL(spl) && skill[spl].function_pointer)) {
        send_to_char("That is not a proper spell\n\r", ch);
        return;
    }


    if (!IS_IMMORTAL(ch)) {
        if (GET_POS(ch) < skill[spl].min_pos) {
            switch (GET_POS(ch)) {
            case POSITION_SLEEPING:
                send_to_char("You dream about great magical powers.\n\r", ch);
                break;
            case POSITION_RESTING:
                send_to_char("You can't concentrate enough while resting.\n\r", ch);
                break;
            case POSITION_SITTING:
                send_to_char("You can't do this sitting!\n\r", ch);
                break;
            case POSITION_FIGHTING:
                send_to_char("Impossible! You can't concentrate enough!.\n\r", ch);
                break;
            default:
                send_to_char("It seems like you're in a pretty bad shape!\n\r", ch);
                break;
            }
            return;
        }
        if (IS_CALMED(ch))
            if ((skill[spl].mood == MOOD_HARMFUL) || (skill[spl].mood == MOOD_AGGRESSIVE)) {
                send_to_char("You feel too calm right now to harm anyone.\n\r", ch);
                return;
            }
        if (IS_SET(ch->in_room->room_flags, RFL_NO_MAGICK)) {
            send_to_char("Something prevents the channeling of the necessary "
                         "magickal energies.\n\r", ch);
            return;
        }

        if ((IS_SET(ch->in_room->room_flags, RFL_NO_ELEM_MAGICK))
            && ((is_guild_elementalist(GET_GUILD(ch))
                 && (GET_GUILD(ch) != GUILD_VOID_ELEMENTALIST)))) {
            send_to_char("Some barrier prevents you from making contact " "with your element.\n\r",
                         ch);
            return;
        }

        if (IS_SET(ch->in_room->room_flags, RFL_IMMORTAL_ROOM)) {
            send_to_char("You call upon your magick...And nothing happens.\n\r", ch);
            return;
        }
        if ((reach == REACH_ROOM) && ((power - ROOM_REACH_PENALTY) <= 0)) {
            act("That reach will not work with such a low power level.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }

        char caster_msg_to_tuluk_templars[256];

        if (!IS_TRIBE(ch, 8) &&  // Templar casters ignored
            room_in_city(ch->in_room) == CITY_TULUK &&
            !IS_IMMORTAL(ch)) {
          
          act("A cold, sudden presence warns you that you have been observed.", FALSE, ch, 0, 0, TO_CHAR);
          
          bool room_domed = FALSE;
          
          for (CHAR_DATA *person = ch->in_room->people; person; person = person->next_in_room) {
            if (affected_by_spell(person, PSI_DOME)) {
              room_domed = TRUE;
              break;
            }
          }

          bool psi_blocked = FALSE;

          if (affected_by_spell(ch, PSI_BARRIER) ||
              affected_by_spell(ch, SPELL_PSI_SUPPRESSION) ||
              affected_by_spell(ch, SPELL_SHIELD_OF_NILAZ) ||
              IS_SET(ch->in_room->room_flags, RFL_NO_PSIONICS) ||
              room_domed) {
            psi_blocked = TRUE;
          }

          if (psi_blocked) {
            sprintf(caster_msg_to_tuluk_templars, "You sense magick use in %s.", ch->in_room->name);
            
            // Incriminate for one day
            af.type = TYPE_CRIMINAL;
            af.location = CHAR_APPLY_CFLAGS;
            af.duration = 8; // one dayf
            af.modifier = city[CITY_TULUK].c_flag;
            af.bitvector = 0;
            af.power = CRIME_TREASON;
            affect_to_char(ch, &af);
          } else {
            sprintf(caster_msg_to_tuluk_templars, "You sense magick use in by %s in %s.", 
                    MSTR(ch, short_descr), ch->in_room->name);
            
            // Permanent criminal
            if (get_char_extra_desc_value(ch, "[CRIM_FLAGS]", buf, 100))
              permflags = atol(buf);
            MUD_SET_BIT(ch->specials.act, city[CITY_TULUK].c_flag);
            MUD_SET_BIT(permflags, city[CITY_TULUK].c_flag);
            sprintf(buf, "%ld", permflags);
            set_char_extra_desc_value(ch, "[CRIM_FLAGS]", buf);
          }
          
          struct descriptor_data *d;
          
          for (d = descriptor_list; d; d = d->next) {
            if (!d->character) {
              continue;
            }

            CHAR_DATA *templar = d->character;

            if (!d->connected && 
                !d->str && 
                templar != ch &&
                room_in_city(templar->in_room) == CITY_TULUK &&
                (templar->player.guild == GUILD_JIHAE_TEMPLAR ||
                 templar->player.guild == GUILD_LIRATHU_TEMPLAR) && 
                !IS_IMMORTAL(templar) &&
                IS_TRIBE(templar, 8) &&
                can_use_psionics(templar) &&
                AWAKE(templar) &&
                !IS_CREATING(templar) &&
                !IS_SET(templar->specials.quiet_level, QUIET_CONT_MOB)) {
              send_to_char(caster_msg_to_tuluk_templars, templar);
              clairaudient_listener(ch, templar, FALSE, argument);
            }
          }
        }
        
        tmpamt = mana_amount(ch, power, spl);

        // Cost is double in Tuluk for elementalists
        if (!IS_TRIBE(ch, 8) &&  // Templar casters ignored
            room_in_city(ch->in_room) == CITY_TULUK &&
            !IS_IMMORTAL(ch)) {
          if (is_guild_elementalist(GET_GUILD(ch))) {
            tmpamt = tmpamt * 2;
          }
        }
        if ((GET_GUILD(ch) == GUILD_WATER_ELEMENTALIST)
            && ((ch->in_room->sector_type == SECT_FIRE_PLANE)
                || (ch->in_room->sector_type == SECT_LIGHTNING_PLANE)))
            tmpamt = tmpamt * 2;
        if ((GET_GUILD(ch) == GUILD_FIRE_ELEMENTALIST)
            && ((ch->in_room->sector_type == SECT_WATER_PLANE)
                || (ch->in_room->sector_type == SECT_SHADOW_PLANE)))
            tmpamt = tmpamt * 2;
        if ((GET_GUILD(ch) == GUILD_WIND_ELEMENTALIST)
            && (ch->in_room->sector_type == SECT_EARTH_PLANE))
            tmpamt = tmpamt * 2;
        if ((GET_GUILD(ch) == GUILD_STONE_ELEMENTALIST)
            && (ch->in_room->sector_type == SECT_AIR_PLANE))
            tmpamt = tmpamt * 2;
        if ((GET_GUILD(ch) == GUILD_LIGHTNING_ELEMENTALIST)
            && (ch->in_room->sector_type == SECT_WATER_PLANE))
            tmpamt = tmpamt * 2;
        if ((GET_GUILD(ch) == GUILD_SHADOW_ELEMENTALIST)
            && (ch->in_room->sector_type == SECT_FIRE_PLANE))
            tmpamt = tmpamt * 2;

        if ((GET_GUILD(ch) == GUILD_WIND_ELEMENTALIST)
            && (ch->in_room->sector_type == SECT_AIR_PLANE))
            tmpamt = tmpamt / 5;
        if ((GET_GUILD(ch) == GUILD_FIRE_ELEMENTALIST)
            && (ch->in_room->sector_type == SECT_FIRE_PLANE))
            tmpamt = tmpamt / 5;
        if ((GET_GUILD(ch) == GUILD_WATER_ELEMENTALIST)
            && (ch->in_room->sector_type == SECT_WATER_PLANE))
            tmpamt = tmpamt / 5;
        if ((GET_GUILD(ch) == GUILD_STONE_ELEMENTALIST)
            && (ch->in_room->sector_type == SECT_EARTH_PLANE))
            tmpamt = tmpamt / 5;
        if ((GET_GUILD(ch) == GUILD_LIGHTNING_ELEMENTALIST)
            && (ch->in_room->sector_type == SECT_LIGHTNING_PLANE))
            tmpamt = tmpamt / 5;
        if ((GET_GUILD(ch) == GUILD_SHADOW_ELEMENTALIST)
            && (ch->in_room->sector_type == SECT_SHADOW_PLANE))
            tmpamt = tmpamt / 5;
        if ((GET_GUILD(ch) == GUILD_VOID_ELEMENTALIST)
            && (ch->in_room->sector_type == SECT_NILAZ_PLANE))
            tmpamt = tmpamt / 5;

        if (reach == REACH_ROOM || reach == REACH_SILENT)
            tmpamt = tmpamt * 2;
        if (GET_MANA(ch) < tmpamt) {
            if (IS_IMMORTAL(ch))
                send_to_char("Checking is_templar_in_city.\n\r", ch);
            if (!is_templar_in_city(ch)) {
                send_to_char("You cannot channel enough energy to cast the " "spell.\n\r", ch);
                return;
            }
        }

    }

    /* **************** Locate targets **************** */

    target_ok = FALSE;
    tar_char = 0;
    tar_obj = 0;
    switch (reach) {
    case REACH_NONE:           /* doesn't matter about target */
        break;

    case REACH_ROOM:
    case REACH_SILENT:
    case REACH_LOCAL:
        if (!IS_SET(skill[spl].targets, TAR_IGNORE)) {
            argument = one_argument(argument, name, sizeof(name));

            if (*name) {
                if (IS_SET(skill[spl].targets, TAR_CHAR_ROOM)) {
                    if ((!strcmp(name, "me")) || (!strcmp(name, "self"))) {
                        tar_char = ch;
                        target_ok = TRUE;
                    } else if ((tar_char = get_char_room_vis(ch, name)))
                        target_ok = TRUE;
                    else if ((tar_char = get_char_room(name, ch->in_room))) {
                        if (tar_char == ch)
                            target_ok = TRUE;
                    }
                    if (spl == SPELL_PSEUDO_DEATH)
                        target_ok = FALSE;
                }

                if (!target_ok && IS_SET(skill[spl].targets, TAR_CHAR_WORLD)) {
                    if ((tar_char = get_char_vis(ch, name)))
                        target_ok = TRUE;
                }

                if (!target_ok && IS_SET(skill[spl].targets, TAR_OBJ_INV))
                    if ((tar_obj = get_obj_in_list_vis(ch, name, ch->carrying)))
                        target_ok = TRUE;

                if (!target_ok && IS_SET(skill[spl].targets, TAR_OBJ_ROOM)) {
                    if (spl == SPELL_PSEUDO_DEATH)
                        argument = one_argument(argument, name, sizeof(name));
                    if ((tar_obj = get_obj_in_list_vis(ch, name, ch->in_room->contents)))
                        target_ok = TRUE;
                }

                if (!target_ok && IS_SET(skill[spl].targets, TAR_OBJ_WORLD))
                    if ((tar_obj = get_obj_vis(ch, name)))
                        target_ok = TRUE;

                if (!target_ok && IS_SET(skill[spl].targets, TAR_OBJ_EQUIP)) {
                    for (i = 0; i < MAX_WEAR && !target_ok; i++)
                        if (ch->equipment[i] && stricmp(name, OSTR(ch->equipment[i], name)) == 0) {
                            tar_obj = ch->equipment[i];
                            target_ok = TRUE;
                        }
                }
                if (!target_ok && IS_SET(skill[spl].targets, TAR_SELF_ONLY))
                    if (stricmp(GET_NAME(ch), name) == 0) {
                        tar_char = ch;
                        target_ok = TRUE;
                    }
            } else {            /* No argument was typed */

                if (IS_SET(skill[spl].targets, TAR_FIGHT_SELF))
                    if (ch->specials.fighting) {
                        tar_char = ch;
                        target_ok = TRUE;
                    }
                if (!target_ok && IS_SET(skill[spl].targets, TAR_FIGHT_VICT))
                    if (ch->specials.fighting) {
                        /* WARNING, MAKE INTO POINTER */
                        tar_char = ch->specials.fighting;
                        target_ok = TRUE;
                    }
                if (!target_ok && IS_SET(skill[spl].targets, TAR_SELF_ONLY)) {
                    tar_char = ch;
                    target_ok = TRUE;
                }
                if (!target_ok && IS_SET(skill[spl].targets, TAR_CHAR_CONTACT))
                    if (ch->specials.contact) {
                        tar_char = ch->specials.contact;
                        target_ok = TRUE;
                    }
            }

        } else
            target_ok = TRUE;   /* No target, is a good target */

        if (reach == REACH_ROOM)
            target_ok = TRUE;

        if (!target_ok) {
            if (*name) {
                if (IS_SET(skill[spl].targets, TAR_CHAR_ROOM))
                    send_to_char("Nobody here by that name.\n\r", ch);
                else if (IS_SET(skill[spl].targets, TAR_CHAR_WORLD))
                    send_to_char("Nobody playing by that name.\n\r", ch);
                else if (IS_SET(skill[spl].targets, TAR_OBJ_INV))
                    send_to_char("You are not carrying anything like " "that.\n\r", ch);
                else if (IS_SET(skill[spl].targets, TAR_OBJ_ROOM))
                    send_to_char("Nothing here by that name.\n\r", ch);
                else if (IS_SET(skill[spl].targets, TAR_OBJ_WORLD))
                    send_to_char("Nothing at all by that name.\n\r", ch);
                else if (IS_SET(skill[spl].targets, TAR_OBJ_EQUIP))
                    send_to_char("You are not wearing anything like " "that.\n\r", ch);
                else if (IS_SET(skill[spl].targets, TAR_OBJ_WORLD))
                    send_to_char("Nothing at all by that" " name.\n\r", ch);

            } else {            /* Nothing was given as argument */
                if (skill[spl].targets < TAR_OBJ_INV)
                    send_to_char("Who should the spell be cast upon?\n\r", ch);
                else {
                    if (spl == SPELL_PSEUDO_DEATH) {
                        send_to_char("  Syntax: cast 'pseudo death' <target>" " <object>\n\r", ch);
                        return;
                    } else
                        send_to_char("What should the spell be cast upon?\n\r", ch);
                }
            }
            return;
        } else {                /* TARGET IS OK */

            if ((tar_char) && (IS_IMMORTAL(tar_char) && !IS_IMMORTAL(ch))) {
                send_to_char("That could be bad for your health.\n\r", ch);
                return;
            }

            if ((tar_char == ch) && IS_SET(skill[spl].targets, TAR_SELF_NONO)) {
                send_to_char("You cannot cast this spell upon yourself.\n\r", ch);
                return;
            } else if ((tar_char) && (tar_char != ch) && IS_SET(skill[spl].targets, TAR_SELF_ONLY)) {
                send_to_char("You can only cast this spell upon yourself.\n\r", ch);
                return;
            } else if (is_charmed(ch) && (ch->master == tar_char)) {
                send_to_char("You are afraid that it could harm yourmaster.\n\r", ch);
                return;
            }
            if (!IS_SET(skill[spl].targets, TAR_ETHEREAL_PLANE)
                && !IS_IMMORTAL(ch) && !ethereal_ch_compare(ch, tar_char))
                return;

        }                       /* Target is OK */
        if (ch_starts_falling_check(ch)) {
          return;
        }

        break;

    case REACH_POTION:
    case REACH_SCROLL:
    case REACH_WAND:
    case REACH_STAFF:
        argument = one_argument(argument, name, sizeof(name));
        if (!(tar_obj = get_obj_in_list_vis(ch, name, ch->carrying))) {
            sprintf(buf, "You do not see '%s' in your inventory.\n\r", name);
            send_to_char(buf, ch);
            return;
        }
        break;
    default:
        shhlog("Invalid reach case in cmd_cast().");
        return;
    }

    if (!ch->specials.to_process) {
        /* send to monitoring people */
        sprintf(buf, "%s begins casting '%s'%s.\n\r", MSTR(ch, name), skill_name[spl], oldarg);
        send_to_monitor(ch, buf, MONITOR_MAGICK);

        if (reach == REACH_SILENT)
            cmd = CMD_SILENT_CAST;
        add_to_command_q(ch, oldarg, cmd, spl, 0);
    } else {
        /* send to monitoring people */
        sprintf(buf, "%s casts '%s'%s.\n\r", MSTR(ch, name), skill_name[spl], oldarg);
        send_to_monitor(ch, buf, MONITOR_MAGICK);

        WIPE_LDESC(ch);

        /* Empty 'buf' to prevent possible monitor bug */
        strcpy(buf, "");

        char_room = ch->in_room;
        if (!(find_ex_description("[NO_CAST_MESSAGE]", ch->ex_description, TRUE))) {
            for (persons = char_room->people; persons; persons = next) {
                next = persons->next_in_room;
                if (persons == ch) {
                    sprintf(buf, "You %s the incantation, '%s'.",
                            reach == REACH_SILENT ? "whisper" : "utter", symbols);
                    act(buf, FALSE, ch, 0, persons, TO_CHAR);
                }
                else if (reach != REACH_SILENT
                 || (reach == REACH_SILENT 
                  && !IS_IMMORTAL(ch)
                  && IS_AFFECTED(persons, CHAR_AFF_LISTEN))) {
                    if ((has_skill(persons, spl)
                         && persons->skills[spl]->learned > 15
                         && number(0, 100) < persons->skills[spl]->learned)
                        || IS_IMMORTAL(persons)) {
                        sprintf(buf, "$n %ss the incantation, '%s'.",
                                reach == REACH_SILENT ? "whisper" : "utter", symbols);
                        act(buf, FALSE, ch, 0, persons, TO_VICT);
                    } else {
                        sprintf(buf, "$n %ss an incantation.",
                                reach == REACH_SILENT ? "whisper" : "utter");
                        act(buf, FALSE, ch, 0, persons, TO_VICT);
                    }
                }
            }
        }

        if (!ch) {
            errorlog("ch is null in cmd_cast(), aborting.");
            return;
        }

        if (!ch->in_room) {
            errorlog("ch is in null room in cmd_cast(), aborting.");
            return;
        }

        if ((skill[spl].function_pointer == 0) && spl > 0)
            send_to_char("Sorry, this magick is not working yet.\n\r", ch);
        else {
            if (!can_cast_spell(ch, power, spl, reach)) {
                send_to_char("You lost your concentration!\n\r", ch);
                if (!(is_templar_in_city(ch) && room_in_city(ch->in_room) == CITY_ALLANAK)) {
                    tmpamt = (mana_amount(ch, power, spl) >> 1);
                    if ((GET_GUILD(ch) == GUILD_WATER_ELEMENTALIST)
                        && ((ch->in_room->sector_type == SECT_FIRE_PLANE)
                            || (ch->in_room->sector_type == SECT_LIGHTNING_PLANE)))
                        tmpamt = tmpamt * 2;
                    if ((GET_GUILD(ch) == GUILD_FIRE_ELEMENTALIST)
                        && ((ch->in_room->sector_type == SECT_WATER_PLANE)
                            || (ch->in_room->sector_type == SECT_SHADOW_PLANE)))
                        tmpamt = tmpamt * 2;
                    if ((GET_GUILD(ch) == GUILD_WIND_ELEMENTALIST)
                        && (ch->in_room->sector_type == SECT_EARTH_PLANE))
                        tmpamt = tmpamt * 2;
                    if ((GET_GUILD(ch) == GUILD_STONE_ELEMENTALIST)
                        && (ch->in_room->sector_type == SECT_AIR_PLANE))
                        tmpamt = tmpamt * 2;
                    if ((GET_GUILD(ch) == GUILD_LIGHTNING_ELEMENTALIST)
                        && (ch->in_room->sector_type == SECT_WATER_PLANE))
                        tmpamt = tmpamt * 2;
                    if ((GET_GUILD(ch) == GUILD_SHADOW_ELEMENTALIST)
                        && (ch->in_room->sector_type == SECT_FIRE_PLANE))
                        tmpamt = tmpamt * 2;

                    if ((GET_GUILD(ch) == GUILD_WIND_ELEMENTALIST)
                        && (ch->in_room->sector_type == SECT_AIR_PLANE))
                        tmpamt = tmpamt / 5;
                    if ((GET_GUILD(ch) == GUILD_FIRE_ELEMENTALIST)
                        && (ch->in_room->sector_type == SECT_FIRE_PLANE))
                        tmpamt = tmpamt / 5;
                    if ((GET_GUILD(ch) == GUILD_WATER_ELEMENTALIST)
                        && (ch->in_room->sector_type == SECT_WATER_PLANE))
                        tmpamt = tmpamt / 5;
                    if ((GET_GUILD(ch) == GUILD_STONE_ELEMENTALIST)
                        && (ch->in_room->sector_type == SECT_EARTH_PLANE))
                        tmpamt = tmpamt / 5;
                    if ((GET_GUILD(ch) == GUILD_LIGHTNING_ELEMENTALIST)
                        && (ch->in_room->sector_type == SECT_LIGHTNING_PLANE))
                        tmpamt = tmpamt / 5;
                    if ((GET_GUILD(ch) == GUILD_SHADOW_ELEMENTALIST)
                        && (ch->in_room->sector_type == SECT_SHADOW_PLANE))
                        tmpamt = tmpamt / 5;
                    if ((GET_GUILD(ch) == GUILD_VOID_ELEMENTALIST)
                        && (ch->in_room->sector_type == SECT_NILAZ_PLANE))
                        tmpamt = tmpamt / 5;

                    if (reach == REACH_ROOM || reach == REACH_SILENT)
                        tmpamt = tmpamt * 2;
                   adjust_mana(ch, -tmpamt);

                }

                adjust_ch_infobar(ch, GINFOBAR_MANA);

                if ((GET_GUILD(ch) != GUILD_TEMPLAR)
                    && (power <= (ch->skills[spl]->rel_lev + 1))) {

                    /* 50% chance to gain using 'nil' reach */
                    percent = number(1, 100);
                    if ((reach == REACH_NONE) && (percent > 50))
                        return;

                    // if they have skill in the reach and it's less than the
                    // spell's level, gain that instead
                    if (reach_sk != -1 && has_skill(ch, reach_sk)
                        && ch->skills[reach_sk]->learned < ch->skills[spl]->learned) {
                        // increased gain rate for reach
                        gain_skill(ch, reach_sk, number(5, 7));
                    } else {
                        // 1-4 % increase, instead of 1-2 for elem, 1-3 for sorc
                        gain_skill(ch, spl, number(1, 4));
                    }
                    return;
                }
                return;
            }

            if (power > ch->skills[spl]->rel_lev) {
                percent = 10 * (power - ch->skills[spl]->rel_lev);
                if (GET_GUILD(ch) == GUILD_TEMPLAR)
                    percent *= 2;
                if (GET_GUILD(ch) == GUILD_LIRATHU_TEMPLAR)
                    percent *= 2;
                if (GET_GUILD(ch) == GUILD_JIHAE_TEMPLAR)
                    percent *= 2;
                if (percent >= number(1, 100))
                    ch->skills[spl]->rel_lev++;
                if ((GET_GUILD(ch) == GUILD_DEFILER) && (spl == SPELL_HEAL))
                    ch->skills[spl]->rel_lev = MIN(ch->skills[spl]->rel_lev, 2);
                else
                    ch->skills[spl]->rel_lev = MIN(ch->skills[spl]->rel_lev, 6);
            }

            send_to_char("Ok.\n\r", ch);

            if (reach != REACH_NONE) {
              if (room_in_city(ch->in_room) == CITY_ALLANAK) {
                if (!(IS_TRIBE(ch, 12) || (GET_GUILD(ch) == GUILD_TEMPLAR) || IS_IMMORTAL(ch))) {
                  if (!wearing_allanak_black_gem(ch)) {
                    send_to_char("Something inhibits your abilities, your magick is reduced.\n\r", ch);
                    power = MAX(0, power - 4);
                  }
                }
              }
            }
            
            snprintf(buf, sizeof(buf), "%s casts a magick spell.", MSTR(ch, short_descr));
            send_to_empaths(ch, buf);

            switch (reach) {
            case REACH_NONE:   /* no need to do anything */
                break;

            case REACH_SILENT:
            case REACH_LOCAL:
                if (tar_char && (tar_char != ch) && is_guild_elementalist(GET_GUILD(ch))
                    && (affected_by_spell(tar_char, SPELL_SHIELD_OF_NILAZ))) {
                    send_to_char("Nothing happens.\n\r", ch);
                    send_to_char("You feel a cold sensation, which passes" " quickly.\n\r",
                                 tar_char);
                    return;
                }

                if (tar_char && (tar_char != ch) && affected_by_spell(tar_char, SPELL_TURN_ELEMENT)
                    && get_magick_type(ch) == MAGICK_ELEMENTAL) {
                    act("A flash of grey surrounds $N, as $n's magicks are turned back to $m.",
                        FALSE, ch, 0, tar_char, TO_NOTVICT);
                    act("A flash of grey surrounds you, as $n's magicks are turned back to $m.",
                        FALSE, ch, 0, tar_char, TO_VICT);
                    act("A flash of grey surrounds $N, as your magicks are turned back towards you.",
                        FALSE, ch, 0, tar_char, TO_CHAR);
                    tar_char = ch;
                }

                ((*skill[spl].function_pointer)
                 (power, ch, argument, SPELL_TYPE_SPELL, tar_char, tar_obj));
                break;

            case REACH_POTION:
            case REACH_SCROLL:
            case REACH_WAND:
            case REACH_STAFF:
                imbue_item(ch, tar_obj, reach, power, spl);
                break;
            case REACH_ROOM:
                ((*skill[spl].function_pointer)
                 (MAX(1, power - ROOM_REACH_PENALTY), ch, argument, SPELL_TYPE_ROOM, tar_char, tar_obj));
                break;
            }

            if (!ch) {
                shhlog("ch is null in cmd_cast(), aborting.");
                return;
            }

            if (!ch->in_room) {
                shhlog("ch is in null room in cmd_cast(), aborting.");
                return;
            }

            if (!(is_templar_in_city(ch) && room_in_city(ch->in_room) == CITY_ALLANAK)) {
                tmpamt = mana_amount(ch, power, spl);
                if ((GET_GUILD(ch) == GUILD_WATER_ELEMENTALIST)
                    && ((ch->in_room->sector_type == SECT_FIRE_PLANE)
                        || (ch->in_room->sector_type == SECT_LIGHTNING_PLANE)))
                    tmpamt = tmpamt * 2;
                if ((GET_GUILD(ch) == GUILD_FIRE_ELEMENTALIST)
                    && ((ch->in_room->sector_type == SECT_WATER_PLANE)
                        || (ch->in_room->sector_type == SECT_SHADOW_PLANE)))
                    tmpamt = tmpamt * 2;
                if ((GET_GUILD(ch) == GUILD_WIND_ELEMENTALIST)
                    && (ch->in_room->sector_type == SECT_EARTH_PLANE))
                    tmpamt = tmpamt * 2;
                if ((GET_GUILD(ch) == GUILD_STONE_ELEMENTALIST)
                    && (ch->in_room->sector_type == SECT_AIR_PLANE))
                    tmpamt = tmpamt * 2;
                if ((GET_GUILD(ch) == GUILD_LIGHTNING_ELEMENTALIST)
                    && (ch->in_room->sector_type == SECT_WATER_PLANE))
                    tmpamt = tmpamt * 2;
                if ((GET_GUILD(ch) == GUILD_SHADOW_ELEMENTALIST)
                    && (ch->in_room->sector_type == SECT_FIRE_PLANE))
                    tmpamt = tmpamt * 2;

                if ((GET_GUILD(ch) == GUILD_WIND_ELEMENTALIST)
                    && (ch->in_room->sector_type == SECT_AIR_PLANE))
                    tmpamt = tmpamt / 5;
                if ((GET_GUILD(ch) == GUILD_FIRE_ELEMENTALIST)
                    && (ch->in_room->sector_type == SECT_FIRE_PLANE))
                    tmpamt = tmpamt / 5;
                if ((GET_GUILD(ch) == GUILD_WATER_ELEMENTALIST)
                    && (ch->in_room->sector_type == SECT_WATER_PLANE))
                    tmpamt = tmpamt / 5;
                if ((GET_GUILD(ch) == GUILD_STONE_ELEMENTALIST)
                    && (ch->in_room->sector_type == SECT_EARTH_PLANE))
                    tmpamt = tmpamt / 5;
                if ((GET_GUILD(ch) == GUILD_LIGHTNING_ELEMENTALIST)
                    && (ch->in_room->sector_type == SECT_LIGHTNING_PLANE))
                    tmpamt = tmpamt / 5;
                if ((GET_GUILD(ch) == GUILD_SHADOW_ELEMENTALIST)
                    && (ch->in_room->sector_type == SECT_SHADOW_PLANE))
                    tmpamt = tmpamt / 5;
                if ((GET_GUILD(ch) == GUILD_VOID_ELEMENTALIST)
                    && (ch->in_room->sector_type == SECT_NILAZ_PLANE))
                    tmpamt = tmpamt / 5;

                if (reach == REACH_ROOM || reach == REACH_SILENT)
                    tmpamt = tmpamt * 2;

                adjust_mana(ch, -tmpamt);

                /* Make sure it doesn't go below zero, which it will if
                 * * they're imbueing items */
                set_mana(ch, MAX(GET_MANA(ch), 0));
            }

            adjust_ch_infobar(ch, GINFOBAR_MANA);
        }
    }
}

struct skill_data skill[MAX_SKILLS] = {
    /*   0 */
    {0, 1, POSITION_STANDING, 0,
     0, 0, 0, 1, 1, 1, 0, 0},
    /*   1 */
    {cast_armor, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_EARTH, SPHERE_ABJURATION, MOOD_PROTECTIVE, 0, 0},
    /*   2 */
    {cast_teleport, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_OBJ_WORLD | TAR_CHAR_ROOM, 0, 15,
     ELEMENT_AIR, SPHERE_TELEPORTATION, MOOD_NEUTRAL, 0, 0},
    /*   3 */
    {cast_blind, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_SHADOW, SPHERE_ALTERATION, MOOD_HARMFUL, 0, 0},
    /*   4 */
    {cast_create_food, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_EARTH, SPHERE_CREATION, MOOD_BENEFICIAL, 0, 0},
    /*   5 */
    {cast_create_water, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_OBJ_INV | TAR_OBJ_ROOM, 0, 15,
     ELEMENT_WATER, SPHERE_CREATION, MOOD_BENEFICIAL, 0},
    /*   6 */
    {cast_cure_blindness, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_SHADOW, SPHERE_CLERICAL, MOOD_BENEFICIAL, 0},
    /*   7 */
    {cast_detect_invisibility, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_AIR, SPHERE_DIVINATION, MOOD_REVEALING, 0},
    /*   8 */
    {cast_detect_magick, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_FIRE, SPHERE_DIVINATION, MOOD_REVEALING, 0},
    /*   9 */
    {cast_detect_poison, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_WATER, SPHERE_DIVINATION, MOOD_REVEALING, 0},
    /*  10 */
    {cast_earthquake, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_EARTH, SPHERE_NATURE, MOOD_DESTRUCTIVE, 0},
    /*  11 */
    {cast_fireball, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15,
     ELEMENT_FIRE, SPHERE_INVOCATION, MOOD_AGGRESSIVE, 0},
    /*  12 */
    {cast_heal, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15,
     ELEMENT_WATER, SPHERE_CLERICAL, MOOD_BENEFICIAL, 0},
    /*  13 */
    {cast_invisibility, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, 0, 15,
     ELEMENT_AIR, SPHERE_ILLUSION, MOOD_PROTECTIVE, 0},
    /*  14 */
    {cast_lightning_bolt, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15,
     ELEMENT_ELECTRIC, SPHERE_INVOCATION, MOOD_AGGRESSIVE, 0},
    /*  15 */
    {cast_poison, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, 0, 15,
     ELEMENT_WATER, SPHERE_CLERICAL, MOOD_HARMFUL, 0},
    /*  16 */
    {cast_remove_curse, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, 0, 15,
     ELEMENT_SHADOW, SPHERE_INVOCATION, MOOD_BENEFICIAL, 0},
    /*  17 */
    {cast_sanctuary, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_WATER, SPHERE_CLERICAL, MOOD_PROTECTIVE, COMP_CLERICAL},
    /*  18 */
    {cast_sleep, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15,
     ELEMENT_EARTH, SPHERE_ENCHANTMENT, MOOD_DOMINANT, 0},
    /*  19 */
    {cast_strength, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15,
     ELEMENT_EARTH, SPHERE_ALTERATION, MOOD_BENEFICIAL, 0},
    /*  20 */
    {cast_summon, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_WORLD, 50, 15,
     ELEMENT_AIR, SPHERE_CONJURATION, MOOD_BENEFICIAL, COMP_CONJURATION},
    /*  21 */
    {cast_rain_of_fire, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_FIRE, SPHERE_INVOCATION, MOOD_DESTRUCTIVE, COMP_INVOCATION},
    /*  22 */
    {cast_cure_poison, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, 0, 15,
     ELEMENT_WATER, SPHERE_NATURE, MOOD_BENEFICIAL, 0},
    /*  23 */
    {cast_relocate, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_WORLD, 0, 15,
     ELEMENT_AIR, SPHERE_TELEPORTATION, MOOD_BENEFICIAL, COMP_TELEPORTATION},
    /*  24 */
    {cast_fear, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_SHADOW, SPHERE_ILLUSION, MOOD_HARMFUL, 0},
    /*  25 */
    {cast_refresh, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15,
     ELEMENT_ELECTRIC, SPHERE_CLERICAL, MOOD_BENEFICIAL, 0},
    /*  26 */
    {cast_levitate, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_OBJ_ROOM | TAR_OBJ_INV, 0, 15,
     ELEMENT_AIR, SPHERE_ALTERATION, MOOD_BENEFICIAL, 0},
    /*  27 */
    {cast_light, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_FIRE, SPHERE_CREATION, MOOD_NEUTRAL, 0},
    /*  28 */
    {cast_animate_dead, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_VOID, SPHERE_NECROMANCY, MOOD_NEUTRAL, 0},
/*  29 */
    {cast_dispel_magick, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_IGNORE | TAR_ETHEREAL_PLANE, 0, 15,
     ELEMENT_FIRE, SPHERE_ENCHANTMENT, MOOD_DESTRUCTIVE, 0},
/*  30 */
    {cast_calm, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_WATER, SPHERE_ENCHANTMENT, MOOD_DOMINANT, 0},
/*  31 */
    {cast_infravision, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_SELF_ONLY, 0, 15,
     ELEMENT_SHADOW, SPHERE_ALTERATION, MOOD_BENEFICIAL, 0},
/*  32 */
    {cast_flamestrike, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_FIRE, SPHERE_CLERICAL, MOOD_AGGRESSIVE, 0},
/*  33 */
    {cast_sand_jambiya, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_EARTH, SPHERE_CREATION, MOOD_NEUTRAL, 0},
/*  34 */
    {cast_stone_skin, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_STONE, SPHERE_ABJURATION, MOOD_PROTECTIVE, 0},
/*  35 */
    {cast_weaken, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_EARTH, SPHERE_NECROMANCY, MOOD_HARMFUL, 0},
    /*  36 */
    {cast_gate, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 50, 15,
     ELEMENT_VOID, SPHERE_CONJURATION, MOOD_DOMINANT, 0},
    /*  37 */
    {cast_oasis, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_WATER, SPHERE_INVOCATION, MOOD_BENEFICIAL, 0},
    /*  38 */
    {cast_send_shadow, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_SHADOW, SPHERE_ILLUSION, MOOD_DOMINANT, 0},
    /*  39 */
    {cast_show_the_path, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_WORLD, 0, 15,
     ELEMENT_EARTH, SPHERE_NATURE, MOOD_REVEALING, 0},
    /*  40 */
    {cast_demonfire, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 33, 15,
     ELEMENT_FIRE, SPHERE_NECROMANCY, MOOD_AGGRESSIVE, COMP_NECROMANCY},
    /*  41 */
    {cast_sandstorm, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_AIR, SPHERE_NATURE, MOOD_DESTRUCTIVE, 0},
    /*  42 */
    {cast_hands_of_wind, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_WORLD | TAR_SELF_NONO, 0, 15,
     ELEMENT_AIR, SPHERE_NATURE, MOOD_AGGRESSIVE, 0},
    /*  43 */
    {cast_ethereal, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_SELF_ONLY, 20, 15,
     ELEMENT_SHADOW, SPHERE_ALTERATION, MOOD_PASSIVE, COMP_ALTERATION},
    /*  44 */
    {cast_detect_ethereal, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_SELF_ONLY, 0, 15,
     ELEMENT_SHADOW, SPHERE_DIVINATION, MOOD_REVEALING, COMP_DIVINATION},
    /*  45 */
    {cast_banishment, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_OBJ_WORLD | TAR_CHAR_ROOM | TAR_SELF_NONO, 0, 15,
     ELEMENT_AIR, SPHERE_TELEPORTATION, MOOD_HARMFUL, 0},
    /*  46 */
    {cast_burrow, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_SELF_ONLY, 10, 15,
     ELEMENT_EARTH, SPHERE_INVOCATION, MOOD_PROTECTIVE, 0},
    /*  47 */
    {cast_empower, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 33, 15,
     ELEMENT_FIRE, SPHERE_ENCHANTMENT, MOOD_NEUTRAL, 0},
    /*  48 */
    {cast_feeblemind, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15,
     ELEMENT_EARTH, SPHERE_ALTERATION, MOOD_HARMFUL, 0},
    /*  49 */
    {cast_fury, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15,
     ELEMENT_EARTH, SPHERE_ENCHANTMENT, MOOD_HARMFUL, 0},
    /*  50 */
    {cast_guardian, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_IGNORE, 33, 15,
     ELEMENT_AIR, SPHERE_CONJURATION, MOOD_AGGRESSIVE, COMP_CONJURATION},
    /*  51 */
    {cast_glyph, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_OBJ_INV | TAR_OBJ_ROOM, 10, 15,
     ELEMENT_FIRE, SPHERE_ENCHANTMENT, MOOD_HARMFUL, 0},
    /*  52 */
    {cast_transference, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_WORLD, 33, 15,
     ELEMENT_AIR, SPHERE_TELEPORTATION, MOOD_PASSIVE,
     COMP_ALTERATION | COMP_TELEPORTATION},
    /*  53 */
    {cast_invulnerability, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_SELF_ONLY, 0, 15,
     ELEMENT_WATER, SPHERE_ABJURATION, MOOD_PROTECTIVE, 0},
    /*  54 */
    {cast_determine_relationship, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15,
     ELEMENT_WATER, SPHERE_NATURE, MOOD_REVEALING, 0},
    /*  55 */
    {cast_mark, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_OBJ_INV, 25, 15,
     ELEMENT_VOID, SPHERE_ENCHANTMENT, MOOD_NEUTRAL, 0},
    /*  56 */
    {cast_mount, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 20, 15,
     ELEMENT_EARTH, SPHERE_CREATION, MOOD_DOMINANT, 0},
    /*  57 */
    {cast_pseudo_death, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_OBJ_ROOM, 20, 15,
     ELEMENT_VOID, SPHERE_ILLUSION, MOOD_DESTRUCTIVE, 0},
    /*  58 */
    {cast_psionic_suppression, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15,
     ELEMENT_VOID, SPHERE_ENCHANTMENT, MOOD_DOMINANT, 0},
    /*  59 */
    {cast_restful_shade, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 20, 15,
     ELEMENT_SHADOW, SPHERE_NATURE, MOOD_BENEFICIAL, 0},
    /*  60 */
    {cast_fly, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_SELF_ONLY, 25, 15,
     ELEMENT_AIR, SPHERE_INVOCATION, MOOD_DOMINANT, 0},
    /*  61 */
    {cast_stalker, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 20, 15,
     ELEMENT_AIR, SPHERE_CONJURATION, MOOD_DESTRUCTIVE, COMP_CONJURATION},
    /*  62 */
    {cast_thunder, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_WATER, SPHERE_INVOCATION, MOOD_NEUTRAL, 0},
    /*  63 */
    {cast_dragon_drain, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 33, 15,
     ELEMENT_VOID, SPHERE_NECROMANCY, MOOD_HARMFUL, 0},
    /*  64 */
    {cast_wall_of_sand, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_IGNORE, 10, 15,
     ELEMENT_EARTH, SPHERE_ALTERATION, MOOD_PROTECTIVE, 0},
    /*  65 */
    {cast_rewind, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_EARTH, SPHERE_ALTERATION, MOOD_PASSIVE, 0},
    /*  66 */
    {cast_pyrotechnics, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_FIRE, SPHERE_ILLUSION, MOOD_PASSIVE, 0},
    /*  67 */
    {cast_insomnia, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15,
     ELEMENT_ELECTRIC, SPHERE_ENCHANTMENT, MOOD_DESTRUCTIVE, 0},
    /*  68 */
    {cast_tongues, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_SELF_ONLY, 0, 15,
     ELEMENT_FIRE, SPHERE_ALTERATION, MOOD_REVEALING, 0},
    /*  69 */
    {cast_charm_person, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 33, 15,
     ELEMENT_VOID, SPHERE_ALTERATION, MOOD_PASSIVE, 0},
    /*  70 */
    {cast_alarm, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 33, 15,
     ELEMENT_EARTH, SPHERE_ALTERATION, MOOD_REVEALING, 0},
    /*  71 */
    {cast_feather_fall, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15,
     ELEMENT_AIR, SPHERE_ALTERATION, MOOD_PROTECTIVE, 0},
    /*  72 */
    {cast_shield_of_nilaz, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_SELF_ONLY, 25, 15,
     ELEMENT_VOID, SPHERE_INVOCATION, MOOD_PROTECTIVE, 0},
    /*  73 */
    {cast_deafness, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_WATER, SPHERE_ILLUSION, MOOD_HARMFUL, 0},
    /*  74 */
    {cast_silence, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 33, 15,
     ELEMENT_WATER, SPHERE_ENCHANTMENT, MOOD_HARMFUL, 0},
    /*  75 */
    {cast_solace, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 33, 15,
     ELEMENT_VOID, SPHERE_CLERICAL, MOOD_BENEFICIAL, 0},
    /*  76 */
    {cast_slow, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 33, 15,
     ELEMENT_ELECTRIC, SPHERE_ALTERATION, MOOD_DOMINANT, 0},
    /*  77 */
    {psi_contact, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_CHAR_WORLD, 0, 5, 0, 0, 0, 0},
    /*  78 */
    {psi_barrier, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /*  79 */
    {psi_locate, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_CHAR_CONTACT, 0, 5, 0, 0, 0, 0},
    /*  80 */
    {psi_probe, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_CHAR_CONTACT, 0, 5, 0, 0, 0, 0},
    /*  81 */
    {psi_trace, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /*  82 */
    {psi_expel, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /*  83 */
    {cast_dragon_bane, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 33, 15,
     ELEMENT_VOID, SPHERE_INVOCATION, MOOD_AGGRESSIVE, 0},
    /*  84 */
    {cast_repel, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_AIR, SPHERE_TELEPORTATION, MOOD_DOMINANT, 0},
    /*  85 */
    {cast_health_drain, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_WATER, SPHERE_INVOCATION, MOOD_HARMFUL, 0},
    /*  86 */
    {cast_aura_drain, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_VOID, SPHERE_INVOCATION, MOOD_HARMFUL, 0},
    /*  87 */
    {cast_stamina_drain, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_ELECTRIC, SPHERE_INVOCATION, MOOD_HARMFUL, 0},
    /*  88 */
    {skill_peek, 1, POSITION_STANDING, CLASS_PERCEP,
     TAR_CHAR_ROOM | TAR_SELF_NONO, 0, 15, 0, 0, 0, 0},
    /*  89 */
    {0, 1, POSITION_STANDING, 0,
     0, 0, 0, 1, 1, 1, 0},
    /*  90 */
    {cast_godspeed, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_SELF_ONLY, 10, 15,
     ELEMENT_STONE, SPHERE_ALTERATION, MOOD_BENEFICIAL, 0},
    /*  91 */
    { /* willpower don't touch */ 0, 1, POSITION_STANDING, 0,
     0, 0, 0, 0, 0, 0, 0},
    /*  92 */
    {psi_cathexis, 1, POSITION_SLEEPING, CLASS_PSIONICS,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /*  93 */
    {psi_empathy, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_CHAR_CONTACT, 0, 5, 0, 0, 0, 0},
    /*  94 */
    {psi_comprehend_langs, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /*  95 */
    {psi_mindwipe, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_CHAR_CONTACT, 0, 5, 0, 0, 0, 0},
    /*  96 */
    {psi_shadow_walk, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /*  97 */
    {psi_hear, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /*  98 */
    {psi_control, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_CHAR_CONTACT, 0, 5, 0, 0, 0, 0},
    /*  99 */
    {psi_compel, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_CHAR_CONTACT, 0, 5, 0, 0, 0, 0},
    /* 100 */
    {psi_conceal, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 101 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 102 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 103 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 104 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 105 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 106 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 107 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 108 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 109 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 110 */
    {0, 1, POSITION_STANDING, CLASS_COMBAT,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 111 */
    {0, /* sneak */ 1, POSITION_STANDING, CLASS_STEALTH,
     TAR_IGNORE, 0, 20, 0, 0, 0, 0},
    /* 112 */
    {skill_hide, 1, POSITION_STANDING, CLASS_STEALTH,
     TAR_IGNORE, 0, 25, 0, 0, 0, 0},
    /* 113 */
    {skill_steal, 1, POSITION_STANDING, CLASS_MANIP,
     TAR_IGNORE, 0, 40, 0, 0, 0, 0},
    /* 114 */
    {skill_backstab, 1, POSITION_STANDING, CLASS_COMBAT,
     TAR_CHAR_ROOM | TAR_SELF_NONO, 0, 15, 0, 0, 0, 0},
    /* 115 */
    {skill_pick, 0, POSITION_SITTING, CLASS_MANIP,
     TAR_IGNORE, 0, 10, 0, 0, 0, 0},
    /* 116 */
    {skill_kick, 2, POSITION_FIGHTING, CLASS_COMBAT,
     TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, 0, 5, 0, 0, 0, 0},
    /* 117 */
    {skill_bash, 2, POSITION_FIGHTING, CLASS_COMBAT,
     TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, 0, 5, 0, 0, 0, 0},
    /* 118 */
    {skill_rescue, 1, POSITION_FIGHTING, CLASS_COMBAT,
     TAR_CHAR_ROOM | TAR_SELF_NONO, 0, 5, 0, 0, 0, 0},
    /* 119 */
    {skill_disarm, 1, POSITION_FIGHTING, CLASS_COMBAT,
     TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, 0, 5, 0, 0, 0, 0},
    /* 120 */
    {0, 1, POSITION_FIGHTING, CLASS_COMBAT,
     TAR_IGNORE | TAR_SELF_NONO, 0, 5, 0, 0, 0, 0},
    /* 121 */
    {skill_listen, 1, POSITION_RESTING, CLASS_PERCEP,
     TAR_IGNORE, 0, 60, 0, 0, 0, 0},
    /* 122 */
    {skill_trap, 1, POSITION_STANDING, CLASS_MANIP,
     TAR_IGNORE, 0, 40, 0, 0, 0, 0},
    /* 123 */
    {skill_hunt, 0, POSITION_STANDING, CLASS_PERCEP,
     TAR_CHAR_WORLD, 0, 30, 0, 0, 0, 0},
    /* 124 */
    {0, 1, POSITION_STANDING, CLASS_BARTER,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 125 */
    {0, 1, POSITION_STANDING, CLASS_STEALTH,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 126 */
    {skill_bandage, 1, POSITION_STANDING, CLASS_MANIP,
     TAR_CHAR_ROOM, 0, 5, 0, 0, 0, 0},
    /* 127 */
    {0, 1, POSITION_STANDING, CLASS_COMBAT,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 128 */
/* old skill_throw 
  { skill_throw,              1, POSITION_STANDING, CLASS_COMBAT,  
      TAR_IGNORE, 0, 20, 0, 0, 0, 0},
 */
    {skill_throw, 1, POSITION_STANDING, CLASS_COMBAT,
     TAR_CHAR_DIR | TAR_USE_EQUIP, 0, 20, 0, 0, 0, 0},
    /* 129 */
    {skill_subdue, 1, POSITION_STANDING, CLASS_COMBAT,
     TAR_CHAR_ROOM | TAR_SELF_NONO, 0, 20, 0, 0, 0, 0},
    /* 130 */
    {skill_scan, 1, POSITION_STANDING, CLASS_PERCEP,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 131 */
    {skill_search, 0, POSITION_STANDING, CLASS_PERCEP,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 132 */
    {skill_value, 1, POSITION_RESTING, CLASS_BARTER,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 133 */
    {0, 1, POSITION_STANDING, CLASS_MANIP,
     TAR_IGNORE, 0, 20, 0, 0, 0, 0},
    /* 134 skill_pilot */
    {0, 1, POSITION_STANDING, CLASS_MANIP,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 135 */
    {skill_poisoning, 1, POSITION_SITTING, CLASS_MANIP,
     TAR_OBJ_ROOM | TAR_OBJ_INV, 0, 15, 0, 0, 0, 0},
    /* 136 */
    {skill_guard, 1, POSITION_STANDING, CLASS_COMBAT,
     TAR_CHAR_ROOM, 0, 5, 0, 0, 0, 0},
    /* 137 */
    {0, 1, POSITION_STANDING, CLASS_WEAPON,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 138 */
    {0, 1, POSITION_STANDING, CLASS_WEAPON,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 139 */
    {0, 1, POSITION_STANDING, CLASS_WEAPON,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 140 */
    {0, 1, POSITION_STANDING, CLASS_WEAPON,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 141 */
    {spice_methelinoc, 1, POSITION_STANDING, CLASS_SPICE,
     TAR_SELF_ONLY, 0, 0, 0, 0, 0, 0},
    /* 142 */
    {spice_melem_tuek, 1, POSITION_STANDING, CLASS_SPICE,
     TAR_SELF_ONLY, 0, 0, 0, 0, 0, 0},
    /* 143 */
    {spice_krelez, 1, POSITION_STANDING, CLASS_SPICE,
     TAR_SELF_ONLY, 0, 0, 0, 0, 0, 0},
    /* 144 */
    {spice_kemen, 1, POSITION_STANDING, CLASS_SPICE,
     TAR_SELF_ONLY, 0, 0, 0, 0, 0, 0},
    /* 145 */
    {spice_aggross, 1, POSITION_STANDING, CLASS_SPICE,
     TAR_SELF_ONLY, 0, 0, 0, 0, 0, 0},
    /* 146 */
    {spice_zharal, 1, POSITION_STANDING, CLASS_SPICE,
     TAR_SELF_ONLY, 0, 0, 0, 0, 0, 0},
    /* 147 */
    {spice_thodeliv, 1, POSITION_STANDING, CLASS_SPICE,
     TAR_SELF_ONLY, 0, 0, 0, 0, 0, 0},
    /* 148 */
    {spice_krentakh, 1, POSITION_STANDING, CLASS_SPICE,
     TAR_SELF_ONLY, 0, 0, 0, 0, 0, 0},
    /* 149 */
    {spice_qel, 1, POSITION_STANDING, CLASS_SPICE,
     TAR_SELF_ONLY, 0, 0, 0, 0, 0, 0},
    /* 150 */
    {spice_moss, 1, POSITION_STANDING, CLASS_SPICE,
     TAR_SELF_ONLY, 0, 0, 0, 0, 0, 0},
    /* 151 */
    {0, 1, POSITION_STANDING, CLASS_LANG, TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 152 */
    {0, 1, POSITION_STANDING, CLASS_LANG, TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 153 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 154 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 155 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 156 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 157 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 158 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 159 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 160 */
    {psi_dome, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 161 */
    {psi_clairaudience, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_CHAR_CONTACT, 0, 5, 0, 0, 0, 0},
    /* 162 */
    {psi_masquerade, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_CHAR_CONTACT, 0, 5, 0, 0, 0, 0},
    /* 163 */
    {psi_illusion, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 164 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 165 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 166 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 167 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 168 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 169 */
    {skill_brew, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 170 */
    {cast_create_darkness, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_SHADOW, SPHERE_CREATION, MOOD_DESTRUCTIVE, 0},
    /* 171 */
    {cast_paralyze, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15,
     ELEMENT_ELECTRIC, SPHERE_ALTERATION, MOOD_HARMFUL, 0},
    /* 172 */
    {cast_chain_lightning, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_ELECTRIC, SPHERE_CONJURATION, MOOD_DESTRUCTIVE, 0},
    /* 173 */
    {cast_lightning_storm, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 20, 15,
     ELEMENT_ELECTRIC, SPHERE_NATURE, MOOD_HARMFUL, 0},
    /* 174 */
    {cast_energy_shield, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_SELF_ONLY, 0, 15,
     ELEMENT_ELECTRIC, SPHERE_CREATION, MOOD_PROTECTIVE, 0, 0},
    /* 175 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 176 */
    {cast_curse, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_SHADOW, SPHERE_ENCHANTMENT, MOOD_DOMINANT, 0},
    /* 177 */
    {cast_fluorescent_footsteps, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_ELECTRIC, SPHERE_ALTERATION, MOOD_REVEALING, 0},
    /* 178 */
    {cast_regenerate, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_ELECTRIC, SPHERE_INVOCATION, MOOD_BENEFICIAL, 0},
    /* 179 */
    {cast_firebreather, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_FIRE, SPHERE_NATURE, MOOD_NEUTRAL, 0},
    /* 180 */
    {cast_parch, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_FIRE, SPHERE_NATURE, MOOD_DESTRUCTIVE, 0},
    /* 181 */
    {cast_travel_gate, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_OBJ_INV, 33, 15,
     ELEMENT_VOID, SPHERE_TELEPORTATION, MOOD_DOMINANT, 0},
    /* 182 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 183 */
    {cast_forbid_elements, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 33, 15,
     ELEMENT_VOID, SPHERE_NATURE, MOOD_DESTRUCTIVE, 0},
    /* 184 */
    {cast_turn_element, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_SELF_ONLY, 0, 15,
     ELEMENT_VOID, SPHERE_ABJURATION, MOOD_PROTECTIVE, 0},
    /* 185 */
    {cast_portable_hole, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_VOID, SPHERE_CREATION, MOOD_NEUTRAL, 0},
    /* 186 */
    {cast_elemental_fog, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15,
     ELEMENT_VOID, SPHERE_ILLUSION, MOOD_NEUTRAL, 0},
    /* 187 */
    {cast_planeshift, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_SELF_ONLY, 33, 15,
     ELEMENT_VOID, SPHERE_TELEPORTATION, MOOD_HARMFUL, 0},
    /* 188 */
    {cast_quickening, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_SELF_ONLY, 33, 15,
     ELEMENT_ELECTRIC, SPHERE_CLERICAL, MOOD_AGGRESSIVE, 0},
    /* 189 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 190 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 191 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 192 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 193 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 194 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 195 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 196 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 197 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 198 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 199 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 200 */
    {tol_alcohol, 1, POSITION_STANDING, CLASS_TOLERANCE,
     TAR_SELF_ONLY, 0, 5, 0, 0, 0, 0},
    /* 201 */
    {tol_generic, 1, POSITION_STANDING, CLASS_TOLERANCE,
     TAR_SELF_ONLY, 0, 5, 0, 0, 0, 0},
    /* 202 */
    {tol_grishen, 1, POSITION_STANDING, CLASS_TOLERANCE,
     TAR_SELF_ONLY, 0, 5, 0, 0, 0, 0},
    /* 203 */
    {tol_skellebain, 1, POSITION_STANDING, CLASS_TOLERANCE,
     TAR_SELF_ONLY, 0, 5, 0, 0, 0, 0},
    /* 204 */
    {tol_methelinoc, 1, POSITION_STANDING, CLASS_TOLERANCE,
     TAR_SELF_ONLY, 0, 5, 0, 0, 0, 0},
    /* 205 */
    {tol_terradin, 1, POSITION_STANDING, CLASS_TOLERANCE,
     TAR_SELF_ONLY, 0, 5, 0, 0, 0, 0},
    /* 206 */
    {tol_peraine, 1, POSITION_STANDING, CLASS_TOLERANCE,
     TAR_SELF_ONLY, 0, 5, 0, 0, 0, 0},
    /* 207 */
    {tol_heramide, 1, POSITION_STANDING, CLASS_TOLERANCE,
     TAR_SELF_ONLY, 0, 5, 0, 0, 0, 0},
    /* 208 */
    {tol_plague, 1, POSITION_STANDING, CLASS_TOLERANCE,
     TAR_SELF_ONLY, 0, 5, 0, 0, 0, 0},
    /* 209 */
    {tol_pain, 1, POSITION_STANDING, CLASS_TOLERANCE,
     TAR_SELF_ONLY, 0, 5, 0, 0, 0, 0},
    /* 210 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 211 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 212 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 213 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 214 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 215 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 216 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 217 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 218 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 219 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 220 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 221 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 222 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 223 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 224 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 225 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 226 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 227 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 228 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 229 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 230 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 231 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 232 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 233 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 234 */
    {0, 1, POSITION_STANDING, CLASS_NONE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 235 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 236 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 237 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 238 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 239 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 240 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 241 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 242 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 243 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 244 */
    {0, 1, POSITION_STANDING, CLASS_LANG,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 245 */
    {0, 1, POSITION_STANDING, CLASS_COMBAT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 246 */
    {skill_sap, 1, POSITION_STANDING, CLASS_COMBAT,
     TAR_CHAR_ROOM | TAR_SELF_NONO, 0, 5, 0, 0, 0, 0},
    /* 247 */
    {cast_possess_corpse, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_OBJ_ROOM, 33, 15,
     ELEMENT_VOID, SPHERE_NECROMANCY, MOOD_PASSIVE, 0},
    /* 248 */
    {cast_portal, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_CONTACT | TAR_SELF_NONO | TAR_OBJ_INV, 33, 15,
     ELEMENT_VOID, SPHERE_TELEPORTATION, MOOD_PASSIVE, 0},
    /* 249 */
    {cast_sand_shelter, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_EARTH, SPHERE_CONJURATION, MOOD_PROTECTIVE, 0},
    /* 250 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 251 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 252 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 253 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 254 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 255 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 256 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 257 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 258 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 259 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 260 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 261 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 262 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 263 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 264 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 265 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 266 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 267 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 268 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 269 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 270 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 271 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 272 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 273 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 274 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 275 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 276 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 277 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 278 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 279 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 280 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 281 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 282 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 283 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 284 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 285 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 286 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 287 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 288 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 289 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 290 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 291 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 292 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 293 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 294 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 295 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 296 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 297 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 298 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 299 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 300 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 301 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 302 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 303 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 304 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 305 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 306 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 307 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 308 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 309 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 310 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 311 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 312 */
    {0, 1, POSITION_STANDING, CLASS_KNOWLEDGE,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 313 */
    {0, 1, POSITION_STANDING, CLASS_MANIP,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 314 */
    {0, 1, POSITION_STANDING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 315 */
    {0, 1, POSITION_STANDING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 316 */
    {skill_forage, 1, POSITION_STANDING, CLASS_PERCEP,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 317 */
    {skill_nesskick, 2, POSITION_FIGHTING, CLASS_COMBAT,
     TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, 0, 5, 0, 0, 0, 0},
    /* 318 */
    {skill_flee, 1, POSITION_FIGHTING, CLASS_COMBAT,
     TAR_CHAR_ROOM | TAR_FIGHT_VICT, 0, 5, 0, 0, 0, 0},
    /* 319 */
    {cast_golem, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_IGNORE, 50, 10,
     ELEMENT_STONE, SPHERE_CONJURATION, MOOD_DOMINANT, 0},
    /*  320 */
    {cast_fire_jambiya, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_FIRE, SPHERE_CONJURATION, MOOD_AGGRESSIVE, 0},
    /*  321 fletchery */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /*  322 dyeing */
    {0, 1, POSITION_STANDING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /*  323 pick-making */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 324 lumberjacking */
    {0, 1, POSITION_STANDING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /*  325 feather working */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 326 leather working */
    {0, 1, POSITION_STANDING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 327 rope making */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 328 bow making */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 329 instrument making */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 330 bandage making */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 331 woodworking */
    {0, 1, POSITION_STANDING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 332 armor making */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 333 knifemaking */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 334 component crafting */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 335 floristry */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 336 cooking */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 337 identify */
    {cast_identify, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_SELF_ONLY, 0, 15,
     ELEMENT_VOID, SPHERE_DIVINATION, MOOD_REVEALING, 0},
    /* 338 wall_of_fire */
    {cast_wall_of_fire, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_IGNORE, 10, 15,
     ELEMENT_FIRE, SPHERE_ALTERATION, MOOD_PROTECTIVE, 0},
    /* 339 stonecrafting */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 340 jewelrycrafting */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 341 tentmaking */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 342 wagonmaking */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 343 swordmaking */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 344 spearmaking */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 345 sleight of hand */
    {0, 1, POSITION_SITTING, CLASS_MANIP,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 346 clothworking */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 347 wall_of_wind */
    {cast_wall_of_wind, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_IGNORE, 10, 15,
     ELEMENT_AIR, SPHERE_ALTERATION, MOOD_NEUTRAL, 0},
    /* 348 blade_barrier */
    {cast_blade_barrier, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_IGNORE, 10, 15,
     ELEMENT_VOID, SPHERE_ALTERATION, MOOD_PROTECTIVE, 0},
    /* 349 wall_of_thorns */
    {cast_wall_of_thorns, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_IGNORE, 10, 15,
     ELEMENT_WATER, SPHERE_ALTERATION, MOOD_PROTECTIVE, 0},
    /* 350 wind_armor */
    {cast_wind_armor, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_SELF_ONLY, 0, 15,
     ELEMENT_AIR, SPHERE_ABJURATION, MOOD_PROTECTIVE, 0, 0},
    /* 351 fire_armor */
    {cast_fire_armor, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_SELF_ONLY, 0, 15,
     ELEMENT_FIRE, SPHERE_ABJURATION, MOOD_PROTECTIVE, 0, 0},
    /* 352 armor repair */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 353 weapon repair */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 354 psi sense presence */
    {psi_sense_presence, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 355 psi thoughtsense */
    {0, 1, POSITION_SITTING, CLASS_PSIONICS,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 356 psi mute */
    {psi_suggestion, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_CHAR_CONTACT, 0, 5, 0, 0, 0, 0},
    /* 357 psi babble */
    {psi_babble, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_CHAR_CONTACT, 0, 5, 0, 0, 0, 0},
    /* 358 psi disorient */
    {psi_disorient, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_CHAR_CONTACT, 0, 5, 0, 0, 0, 0},
    /* 359 psi clairvoyance */
    {psi_clairvoyance, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_CHAR_CONTACT, 0, 5, 0, 0, 0, 0},
    /* 360 psi imitate */
    {psi_imitate, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_CHAR_CONTACT, 0, 5, 0, 0, 0, 0},
    /* 361 psi project emotion */
    {psi_project_emotion, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_CHAR_CONTACT, 0, 5, 0, 0, 0, 0},
    /* 362 psi vanish */
    {psi_vanish, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 363 psi beast affinity */
    {0, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 364 psi coercion */
    {psi_coercion, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_CHAR_CONTACT, 0, 5, 0, 0, 0, 0},
    /* 365 psi wild contact */
    {0, 1, POSITION_SITTING, CLASS_PSIONICS,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 366 psi wild barrier */
    {0, 1, POSITION_SITTING, CLASS_PSIONICS,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 367 skill_analyze */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 368 skill_alchemy */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 369 skill_toolmaking */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 370 spell haunt */
    {cast_haunt, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 25, 15,
     ELEMENT_SHADOW, SPHERE_CONJURATION, MOOD_DOMINANT, COMP_CONJURATION},
    /* 352 wagon repair */
    {0, 1, POSITION_STANDING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 372 create wine */
    {cast_create_wine, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_OBJ_INV | TAR_OBJ_ROOM, 0, 15,
     ELEMENT_WATER, SPHERE_CREATION, MOOD_DOMINANT, 0},
    /* 373 wind_fist */
    {cast_wind_fist, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 33, 15,
     ELEMENT_AIR, SPHERE_INVOCATION, MOOD_HARMFUL, 0},
    /* 374 lightning_whip */
    {cast_lightning_whip, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_ELECTRIC, SPHERE_CREATION, MOOD_NEUTRAL, 0},
    /* 375 shadow sword */
    {cast_shadow_sword, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_SHADOW, SPHERE_CREATION, MOOD_NEUTRAL, 0},
    /* 376 psionic_drain */
    {cast_psionic_drain, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15,
     ELEMENT_VOID, SPHERE_INVOCATION, MOOD_DESTRUCTIVE, 0},
    {cast_intoxication, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_WATER, SPHERE_ALTERATION, MOOD_HARMFUL, 0},
    {cast_sober, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_WATER, SPHERE_ALTERATION, MOOD_NEUTRAL, 0},
    {cast_wither, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 20, 15,
     ELEMENT_WATER, SPHERE_NATURE, MOOD_HARMFUL, 0},
    {cast_delusion, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_AIR, SPHERE_ILLUSION, MOOD_PASSIVE, 0},
    {cast_illuminant, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_ELECTRIC, SPHERE_ILLUSION, MOOD_PASSIVE, 0},
    {cast_mirage, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_WATER, SPHERE_ILLUSION, MOOD_PASSIVE, 0},
    {cast_phantasm, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_VOID, SPHERE_ILLUSION, MOOD_PASSIVE, 0},
    {cast_shadowplay, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_SHADOW, SPHERE_ILLUSION, MOOD_PASSIVE, 0},
    /* 385 scent doodie */
    {0, 1, POSITION_SITTING, CLASS_SCENT, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 386 scent roses */
    {0, 1, POSITION_SITTING, CLASS_SCENT, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 387 scent  musk */
    {0, 1, POSITION_SITTING, CLASS_SCENT, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 388 scent citron */
    {0, 1, POSITION_SITTING, CLASS_SCENT, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 389 scent spice */
    {0, 1, POSITION_SITTING, CLASS_SCENT, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 390 scent shrintal */
    {0, 1, POSITION_SITTING, CLASS_SCENT, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 391 scent lanturin */
    {0, 1, POSITION_SITTING, CLASS_SCENT, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 392 scent glimmergrass */
    {0, 1, POSITION_SITTING, CLASS_SCENT, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 393 scent belgoikiss */
    {0, 1, POSITION_SITTING, CLASS_SCENT, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 394 scent lavender */
    {0, 1, POSITION_SITTING, CLASS_SCENT, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 395 scent jasmine */
    {0, 1, POSITION_SITTING, CLASS_SCENT, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 396 scent clove */
    {0, 1, POSITION_SITTING, CLASS_SCENT, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 397 scent mint */
    {0, 1, POSITION_SITTING, CLASS_SCENT, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 398 scent pymlithe */
    {0, 1, POSITION_SITTING, CLASS_SCENT, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 399 skill drawing */
    {0, 1, POSITION_SITTING, CLASS_CRAFT, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 400 language rinthi */
    {0, 1, POSITION_STANDING, CLASS_LANG, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 401 */
    {0, 1, POSITION_STANDING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 402 language North Accent */
    {0, 1, POSITION_STANDING, CLASS_LANG, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 403 */
    {0, 1, POSITION_STANDING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 404 language South Accent */
    {0, 1, POSITION_STANDING, CLASS_LANG, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 405 */
    {0, 1, POSITION_STANDING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 406 */
    {0, 1, POSITION_STANDING, CLASS_WEAPON,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 407 */
    {0, 1, POSITION_STANDING, CLASS_WEAPON,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 408 */
    {0, 1, POSITION_STANDING, CLASS_WEAPON,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 409 */
    {0, 1, POSITION_STANDING, CLASS_WEAPON,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 410 */
    {0, 1, POSITION_STANDING, CLASS_WEAPON,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 411 sandstatue */
    {cast_sandstatue, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_OBJ_INV, 20, 15, ELEMENT_STONE, SPHERE_CREATION, MOOD_PASSIVE, 0},
    /* 412 headshrinking */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 413 */
    {psi_magicksense, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 414 */
    {psi_mindblast, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_CHAR_CONTACT, 0, 5, 0, 0, 0, 0},
    /* 415 */
    {skill_charge, 2, POSITION_FIGHTING, CLASS_COMBAT,
     TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, 0, 5, 0, 0, 0, 0},
    /* 416 */
    {skill_split_opponents, 2, POSITION_FIGHTING, CLASS_COMBAT,
     TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, 0, 5, 0, 0, 0, 0},
    /* 417 */
    {0, 1, POSITION_STANDING, CLASS_COMBAT,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 418 language Anyar Accent  */
    {0, 1, POSITION_STANDING, CLASS_LANG, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 419 */
    {0, 1, POSITION_STANDING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 420 language Bendune Accent */
    {0, 1, POSITION_STANDING, CLASS_LANG, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 421 */
    {0, 1, POSITION_STANDING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 422 spell disembody */
    {cast_disembody, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15, ELEMENT_VOID, SPHERE_NATURE, MOOD_AGGRESSIVE, 0},
    /* 423 skill axe making */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 424 skill club making */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 425 skill_weaving */
    {0, 1, POSITION_SITTING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 426 spell shield of wind */
    {cast_shield_of_wind, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15, ELEMENT_AIR, SPHERE_CONJURATION, MOOD_PROTECTIVE, 0},
    /* 427 spell shield of mist */
    {cast_shield_of_mist, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15, ELEMENT_WATER, SPHERE_CONJURATION, MOOD_PROTECTIVE, 0},
    /* 428 spell drown */
    {0, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15, 0, 0, 0, 0},
    /* 429 spell healing mud */
    {cast_healing_mud, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15, ELEMENT_WATER, SPHERE_NATURE, MOOD_NEUTRAL, 0},
    /* 430 spell cause disease */
    {0, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15, 0, 0, 0, 0},
    /* 431 spell cure_disease */
    {0, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15, ELEMENT_WATER, SPHERE_ENCHANTMENT, MOOD_BENEFICIAL, 0},
    /* 432 spell acid spray */
    {0, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15, 0, 0, 0, 0},
    /* 433 spell puddle */
    {0, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15, 0, 0, 0, 0},
    /* 434 shadow_armor */
    {cast_shadow_armor, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_SELF_ONLY, 0, 15,
     ELEMENT_SHADOW, SPHERE_ABJURATION, MOOD_PROTECTIVE, 0, 0},
    /* 435 fireseed */
    {cast_fireseed, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15, ELEMENT_FIRE, SPHERE_CREATION, MOOD_AGGRESSIVE, 0, 0},
    /* 436 water accent */
    {0, 1, POSITION_STANDING, CLASS_LANG, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 437 fire accent */
    {0, 1, POSITION_STANDING, CLASS_LANG, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 438 wind accent */
    {0, 1, POSITION_STANDING, CLASS_LANG, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 439 earth accent */
    {0, 1, POSITION_STANDING, CLASS_LANG, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 440 shadow accent */
    {0, 1, POSITION_STANDING, CLASS_LANG, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 441 lightning accent */
    {0, 1, POSITION_STANDING, CLASS_LANG, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 442 nilaz accent */
    {0, 1, POSITION_STANDING, CLASS_LANG, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 443 breathe water */
    {cast_breathe_water, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15, ELEMENT_WATER, SPHERE_ABJURATION, MOOD_BENEFICIAL, 0, 0},
    /* 444 lightning_spear */
    {cast_lightning_spear, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15, ELEMENT_ELECTRIC, SPHERE_CREATION, MOOD_AGGRESSIVE, 0, 0},
    /* 445 */
    {0, 1, POSITION_STANDING, CLASS_PERCEP,
     TAR_IGNORE, 0, 15, 0, 0, 0, 0},
    /* 446 */
    {psi_mesmerize, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_CHAR_CONTACT, 0, 5, 0, 0, 0, 0},
    /* 447 shatter */
    {cast_shatter, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_OBJ_INV | TAR_OBJ_ROOM, 20, 15,
     ELEMENT_STONE, SPHERE_ENCHANTMENT, MOOD_HARMFUL, 0},
    /* 448 messenger */
    {cast_messenger, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15,
     ELEMENT_AIR, SPHERE_CONJURATION, MOOD_NEUTRAL, COMP_CONJURATION},
    /* 449 gather */
    {skill_gather, 0, POSITION_STANDING, CLASS_MANIP,
     TAR_IGNORE, 0, 15,
     0, 0, 0, 0},
    /* 450 rejuvenate */
    {psi_rejuvenate, 1, POSITION_RESTING, CLASS_PSIONICS,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 451 blind fighting */
    {0, /* blind fighting */ 1, POSITION_FIGHTING, CLASS_COMBAT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 452 daylight */
    {cast_daylight, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 0, 15, ELEMENT_FIRE, SPHERE_CREATION, MOOD_BENEFICIAL, 0},
    /* 453 dispel invisibility */
    {cast_dispel_invisibility, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM, 0, 15,
     ELEMENT_AIR, SPHERE_ENCHANTMENT, MOOD_DESTRUCTIVE, 0},
    /* 454 dispel ethereal */
    {cast_dispel_ethereal, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_CHAR_ROOM | TAR_OBJ_INV | TAR_OBJ_ROOM | TAR_ETHEREAL_PLANE, 0, 15,
     ELEMENT_SHADOW, SPHERE_ENCHANTMENT, MOOD_DESTRUCTIVE, 0},
    /* 455 repair item */
    {cast_repair_item, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_OBJ_INV | TAR_OBJ_ROOM, 0, 15,
     ELEMENT_EARTH, SPHERE_ALTERATION, MOOD_DOMINANT, 0},
    /* 456 create rune */
    {cast_create_rune, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 50, 15,
     ELEMENT_AIR, SPHERE_CREATION, MOOD_NEUTRAL, 0},
    /* 457 watch */
    {0, /* watch */ 1, POSITION_STANDING, CLASS_PERCEP,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 458 immolate */
    {cast_immolate, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 20, 15,
     ELEMENT_FIRE, SPHERE_ENCHANTMENT, MOOD_AGGRESSIVE, 0},
    /* 459 reach un (local) */
    {0, 1, POSITION_STANDING, CLASS_REACH,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 460 reach nil (none) */
    {0, 1, POSITION_STANDING, CLASS_REACH,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 461 reach oai (potion) */
    {0, 1, POSITION_STANDING, CLASS_REACH,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 462 reach tuk (scroll) */
    {0, 1, POSITION_STANDING, CLASS_REACH,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 463 reach phan (wand) */
    {0, 1, POSITION_STANDING, CLASS_REACH,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 464 reach mur (staff) */
    {0, 1, POSITION_STANDING, CLASS_REACH,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 465 reach kizn (room) */
    {0, 1, POSITION_STANDING, CLASS_REACH,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 466 reach xaou (silent) */
    {0, 1, POSITION_STANDING, CLASS_REACH,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 467 rot items */
    {cast_rot_items, 1, POSITION_FIGHTING, CLASS_MAGICK,
     TAR_CHAR_ROOM, 0, 15,
     ELEMENT_VOID, SPHERE_NATURE, MOOD_DOMINANT, 0},
    /* 468 vampiric blade */
    {cast_vampiric_blade, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_OBJ_INV | TAR_OBJ_ROOM, 25, 15,
     ELEMENT_VOID, SPHERE_ENCHANTMENT, MOOD_AGGRESSIVE, 0},
    /* 469 dead speak */
    {cast_dead_speak, 1, POSITION_STANDING, CLASS_MAGICK,
     TAR_IGNORE, 33, 15,
     ELEMENT_VOID, SPHERE_NECROMANCY, MOOD_DOMINANT, 0},
    /* 470 DISEASE_BOILS */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 471 DISEASE_CHEST_DECAY */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 472 DISEASE_CILOPS_KISS */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 473 DISEASE_GANGRENE */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 474 DISEASE_HEAT_SPLOTCH */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 475 DISEASE_IVORY_SALT_SICKNESS */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 476 DISEASE_KANK_FLEAS */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 477 DISEASE_KRATHS_TOUCH */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 478 DISEASE_MAAR_POX */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 479 DISEASE_PEPPERBELLY */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 480 DISEASE_RAZA_RAZA */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 481 DISEASE_SAND_FEVER */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 482 DISEASE_SAND_FLEAS */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 483 DISEASE_SAND_ROT */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 484 DISEASE_SCRUB_FEVER */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 485 DISEASE_SLOUGH_SKIN */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 486 DISEASE_YELLOW_PLAGUE */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 487 psi immersion */
    {0, 1, POSITION_SITTING, CLASS_PSIONICS,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 488 hero sword */
    {cast_hero_sword, 1, POSITION_SITTING, CLASS_MAGICK, TAR_OBJ_INV | TAR_OBJ_ROOM, 10, 15, ELEMENT_SHADOW, SPHERE_CONJURATION, MOOD_PROTECTIVE, 0},

    /* 489 recite */
    {cast_recite, 1, POSITION_FIGHTING, CLASS_MAGICK, TAR_CHAR_ROOM, 0, 15, ELEMENT_VOID, SPHERE_ALTERATION, MOOD_DOMINANT, 0},
    /* 480 SCENT_EYNANA  */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 491 SCENT_KHEE  */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 492 SCENT_FILANYA */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 493 SCENT_LADYSMANTLE */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 494 SCENT_LIRATHUFAVOR */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 485 SCENT_PFAFNA */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 496 SCENT_REDHEART */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 497 SCENT_SWEETBREEZE */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 498 SCENT_TEMBOTOOTH */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 499 SCENT_THOLINOC */
    {0, 1, POSITION_SITTING, CLASS_NONE, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 500 trample */
    {skill_trample, 2, POSITION_FIGHTING, CLASS_COMBAT,
     TAR_CHAR_ROOM | TAR_FIGHT_VICT | TAR_SELF_NONO, 0, 5, 0, 0, 0, 0},
    /* 501 language desert accent  */
    {0, 1, POSITION_STANDING, CLASS_LANG, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 502 language eastern accent */
    {0, 1, POSITION_STANDING, CLASS_LANG, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 503 language western accent */
    {0, 1, POSITION_STANDING, CLASS_LANG, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 504 scent medichi */
    {0, 1, POSITION_SITTING, CLASS_SCENT, TAR_IGNORE, 0, 5, 0, 0, 0, 0},
    /* 505 clayworking */
    {0, 1, POSITION_STANDING, CLASS_CRAFT,
     TAR_IGNORE, 0, 5, 0, 0, 0, 0},

};


// Defiling modifiers
#define NEG_ECO_DEFILE_MODIFIER		0.005
#define POS_ECO_DEFILE_MODIFIER		0.005
#define GATHER_SKILL_DEFILE_MODIFIER	0.01
#define GATHER_SKILL_DEFILE_BASE	1.50

// Preserving modifiers
#define NEG_ECO_PRESERVE_MODIFIER	0.030
#define POS_ECO_PRESERVE_MODIFIER	0.008
#define GATHER_SKILL_PRESERVE_MODIFIER	0.01
#define GATHER_SKILL_PRESERVE_BASE	2.00


void
cmd_gathe(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    send_to_char("If you want to gather mana, type out the full word!\n\r", ch);
}

/* skill_gather()
 *
 * This implementation of gathering looks at the gatherer's skill% and eco and
 * determines how much the activity costs.
 */
void
skill_gather(byte level, CHAR_DATA * ch, const char *argument, int si, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{
    CHAR_DATA *p;
    int maxlen;
    char buf[MAX_STRING_LENGTH], buf1[256], buf2[256], ch_comm[256];
    struct weather_node *wn = 0;
    float mana = 0, sk = 0, eco = 0;
    float cost_defiling_modifier, cost_defiling = 0;
    float cost_preserving_modifier, cost_preserving = 0;
    float dam_perc = 1;
    bool defiling = TRUE;
    struct affected_type af;
    const char *gather_msg_self, *gather_msg_room;

    memset(&af, 0, sizeof(af));

    if (!has_skill(ch, SKILL_GATHER)
     && (is_guild_elementalist(GET_GUILD(ch))
         || (GET_GUILD(ch) == GUILD_TEMPLAR)
         || (GET_GUILD(ch) == GUILD_LIRATHU_TEMPLAR)
         || (GET_GUILD(ch) == GUILD_JIHAE_TEMPLAR))
     && (GET_RACE(ch) != RACE_VAMPIRE)) {
        send_to_char("You must regain your magick through time.\n\r", ch);
        return;
    }

    argument = two_arguments(argument, buf1, sizeof(buf1), buf2, sizeof(buf2));

    if (!*buf1) {
        send_to_char("You must specify how much you wish to gather.\n\r", ch);
        return;
    }
    if (!*buf2) {
        send_to_char("You must specify where you wish to gather from.\n\r", ch);
        return;
    }
    if (GET_MANA(ch) == GET_MAX_MANA(ch)) {
        send_to_char("You already possess your maximum spell power.\n\r", ch);
        return;
    }


    mana = (float) atoi(buf1);
    sk = (float) get_skill_percentage(ch, SKILL_GATHER);
    eco = (float) ch->specials.eco;

    if (mana <= 0) {
        cprintf(ch, "Mana must be greater than zero.\n\r");
        return;

    }
    // Make sure they don't try to gather more than they can handle
    mana = MIN(GET_MAX_MANA(ch) - GET_MANA(ch), mana);

    maxlen = MAX(2, strlen(buf2));
    if (!strnicmp(buf2, "me", maxlen) || !strnicmp(buf2, "self", maxlen)) {
        defiling = FALSE;
    } else if (!strnicmp(buf2, "land", maxlen) || !strnicmp(buf2, "earth", maxlen)) {
        defiling = TRUE;
    } else {
        send_to_char("You can only gather from the land or yourself.\n\r", ch);
        return;
    }

    // Gathering can be learned from trying, similar to ride and pilot
    if (!has_skill(ch, SKILL_GATHER)) {
        if ((number(1, 100) <= wis_app[GET_WIS(ch)].learn) && GET_MANA(ch) != GET_MAX_MANA(ch)) {
            init_skill(ch, SKILL_GATHER, 5);

            /* if they don't normally get the skill, set it as taught */
            if (get_max_skill(ch, SKILL_GATHER) <= 0)
                set_skill_taught(ch, SKILL_GATHER);
        } else {
            send_to_char("You know nothing about gathering magick.\n\r", ch);
            return;
        }
    }

    // Vampires gather differently, check and see if they're a vampire
    if (GET_RACE(ch) == RACE_VAMPIRE) {
        if (defiling) {
            send_to_char("You are unable to gather from the land.\n\r", ch);
            return;
        }
        if ((GET_COND(ch, FULL) < 7)) {
            send_to_char("You're too hungry to gather mana from your blood.\n\r", ch);
            return;
        }
        if (mana > ((GET_COND(ch, FULL) - 7) * 10)) {
            send_to_char("You don't have enough blood to generate that much magick.\n\r", ch);
            return;
        }

        sprintf(buf, "%s gathers %.f from %s blood.\n\r", GET_NAME(ch), mana, HSHR(ch));
        send_to_monitor(ch, buf, MONITOR_MAGICK);
        snprintf(buf, sizeof(buf), "%s gathers for magick from %sself.", MSTR(ch, short_descr),
                 HMHR(ch));
        send_to_empaths(ch, buf);

        adjust_mana (ch, mana);
//        GET_MANA(ch) += mana;
        gain_condition(ch, FULL, -(mana / 10));
        send_to_char
            ("You concentrate on the difficult task of converting your blood into magickal energy.\n\r",
             ch);
//        adjust_ch_infobar(ch, GINFOBAR_MANA);
        return;
    }

    // Figure out the cost based on their alignment to the land
    if (eco < 0) {
        cost_defiling_modifier =
            GATHER_SKILL_DEFILE_BASE - (sk * GATHER_SKILL_DEFILE_MODIFIER) -
            (fabs(eco) * NEG_ECO_DEFILE_MODIFIER);
        cost_preserving_modifier =
            GATHER_SKILL_PRESERVE_BASE - (sk * GATHER_SKILL_PRESERVE_MODIFIER) +
            (fabs(eco) * NEG_ECO_PRESERVE_MODIFIER);
    } else {
        cost_defiling_modifier =
            GATHER_SKILL_DEFILE_BASE - (sk * GATHER_SKILL_DEFILE_MODIFIER) +
            (fabs(eco) * POS_ECO_DEFILE_MODIFIER);
        cost_preserving_modifier =
            GATHER_SKILL_PRESERVE_BASE - (sk * GATHER_SKILL_PRESERVE_MODIFIER) -
            (fabs(eco) * POS_ECO_PRESERVE_MODIFIER);
    }
    cost_defiling = ceilf(mana * cost_defiling_modifier);
    cost_preserving = ceilf(mana * cost_preserving_modifier);

    // Now for everyone else
    if (defiling) {             // Defiling
        // Galith and halflings can't defile
        if ((GET_RACE(ch) == RACE_GALITH) || (GET_RACE(ch) == RACE_HALFLING)) {
            send_to_char("You cannot make yourself harm the land!\n\r", ch);
            return;

        }
        // Some planes / sectors can't be defiled
        switch (ch->in_room->sector_type) {
        case SECT_AIR:
        case SECT_FIRE_PLANE:
        case SECT_WATER_PLANE:
        case SECT_EARTH_PLANE:
        case SECT_SHADOW_PLANE:
        case SECT_AIR_PLANE:
        case SECT_LIGHTNING_PLANE:
        case SECT_NILAZ_PLANE:
            send_to_char("You cannot seem to gather from the land here.\n\r", ch);
            return;
            // Very difficult to defile these sectors
        case SECT_SEWER:
        case SECT_DESERT:
        case SECT_SALT_FLATS:
        case SECT_SILT:
        case SECT_SHALLOWS:
            cost_defiling *= 1.0;       // base cost
            break;
            // Somewhat difficult to defile these sectors
        case SECT_INSIDE:
        case SECT_CITY:
            cost_defiling *= 0.25;      // -75% of base cost
            break;
            // Less difficult to defile these sectors
        case SECT_CAVERN:
        case SECT_SOUTHERN_CAVERN:
        case SECT_ROAD:
        case SECT_RUINS:
            cost_defiling *= 0.5;       // -50% of base cost
            break;
            // Hardly much extra effort to defile these sectors
        case SECT_HILLS:
        case SECT_MOUNTAIN:
            cost_defiling *= 0.75;      // -25% of base cost
            break;
            // Easy to defile these sectors
        case SECT_FIELD:
        case SECT_LIGHT_FOREST:
        case SECT_HEAVY_FOREST:
        case SECT_THORNLANDS:
        case SECT_COTTONFIELD:
        case SECT_GRASSLANDS:
            cost_defiling *= 0.10;      // -90% of base cost
            break;
        default:
            break;
        }

        /* If it's already been defiled, it's harder to continue.  This
         * will affect the potentially modified cost from sector type above.
         */
        if (IS_SET(ch->in_room->room_flags, RFL_ASH))
            cost_defiling *= 1.05;      // +5% of modified cost

        // Can't defile in code-generated / transient rooms
        if (ch->in_room->zone == 73) {
            send_to_char("You cannot seem to gather from the land here.\n\r", ch);
            return;
        }
        // Don't let them overextend themselves
        if (cost_defiling > GET_HIT(ch)) {
            send_to_char("Gathering that much could be fatal.\n\r", ch);
            return;
        }
        // Find the right weather node for where the character is
        wn = find_weather_node(ch->in_room);
        if (wn && wn->life > 0) {
            // There's still life here to sap from the zone's weather-node
            wn->life = MAX(0, wn->life - 1);
        } else if (!IS_SET(ch->in_room->room_flags, RFL_PLANT_HEAVY)
                   && !IS_SET(ch->in_room->room_flags, RFL_PLANT_SPARSE)) {
            /* There weren't any PLANT_ flags to sap life from.  If the room
             * has already been defiled, tell the defilier it's barren.
             */
            if (IS_SET(ch->in_room->room_flags, RFL_ASH))
                send_to_char("The land cannot yield any more energy to your will.\n\r", ch);
            else
                // No weather-node, no plants, no ash: just can't gather here
                send_to_char("You cannot seem to gather from the land here.\n\r", ch);
            return;             // Unable to defile
        }

        gather_msg_self = find_ex_description("[GATHER_LAND_MSG_SELF]", ch->ex_description, TRUE);
        gather_msg_room = find_ex_description("[GATHER_LAND_MSG_ROOM]", ch->ex_description, TRUE);

        if (gather_msg_self && gather_msg_room) {
            act(gather_msg_self, FALSE, ch, 0, 0, TO_CHAR);
            act(gather_msg_room, TRUE, ch, 0, 0, TO_ROOM);
        } else {
            act("You lower your hand to the ground, letting energy flow into it.", FALSE, ch, 0, 0,
                TO_CHAR);
            act("$n lowers $s hand to the ground, letting energy flow into it.", TRUE, ch, 0, 0,
                TO_ROOM);
        }

        if (!IS_SET(ch->in_room->room_flags, RFL_ASH)) {
            sprintf(buf1, "Grey, lifeless ash collects on the ground.");
            act(buf1, FALSE, ch, 0, 0, TO_ROOM);
            act(buf1, FALSE, ch, 0, 0, TO_CHAR);
            MUD_SET_BIT(ch->in_room->room_flags, RFL_ASH);
        }
        // Set an event to remove the ash
        new_event(EVNT_REMOVE_ASH, (int) (mana * 6 * 60), 0, 0, 0, ch->in_room, 0);

        for (p = character_list; p; p = p->next) {
            if (p->in_room && (p->in_room != ch->in_room)
                && (p->in_room->zone == ch->in_room->zone)
                && (-1 != choose_exit_name_for(p->in_room, ch->in_room, ch_comm, 5, 0))) {
                /*might want to make defiler hunting mobiles do the run-to here */
                if (IS_AFFECTED(p, CHAR_AFF_DETECT_MAGICK)) {
                    sprintf(buf1, "You sense defiling to the %s.\n\r", ch_comm);
                    send_to_char(buf1, p);
                } else
                    send_to_char("You sense defiling in the area.\n\r", p);
            }
        }
        check_gather_flag(ch);

        sprintf(buf, "%s gathers %.f from the land.\n\r", GET_NAME(ch), mana);
        send_to_monitor(ch, buf, MONITOR_MAGICK);
        snprintf(buf, sizeof(buf), "%s gathers for magick from the land.", MSTR(ch, short_descr));
        send_to_empaths(ch, buf);

        // Give 'em the mana
        adjust_mana(ch, mana);

        // Do the damage
        dam_perc = cost_defiling / (float) GET_MAX_HIT(ch);
        set_hit(ch, MAX(GET_HIT(ch) - cost_defiling, 1));
        set_stun(ch, MAX(GET_STUN(ch) - (GET_MAX_STUN(ch) * dam_perc), 0));
        set_move(ch, MAX(GET_MOVE(ch) - (GET_MAX_MOVE(ch) * dam_perc), 0));
        if (!affected_by_spell(ch, SKILL_GATHER)) {
            if (ch->specials.eco > 0)   // Preserver is defiling
                ch->specials.eco = MAX(0, ch->specials.eco - 25);
            else
                ch->specials.eco = MAX(-100, ch->specials.eco - 1);
            level = MIN((sk / 15) + 1, 7);      // Reflect how adept they are
            // Add a defiling affect
            af.type = SKILL_GATHER;
            af.location = 0;
            af.duration = level + number(0, 2);
            af.modifier = 0;
            af.bitvector = 0;
            af.power = level;

            affect_to_char(ch, &af);
        }
//        adjust_ch_infobar(ch, GINFOBAR_HP);
//        adjust_ch_infobar(ch, GINFOBAR_MANA);
//        adjust_ch_infobar(ch, GINFOBAR_STAM);
//        adjust_ch_infobar(ch, GINFOBAR_STUN);

    } else {                    // Preserving
        // Don't let them overextend themselves
        if (cost_preserving > GET_HIT(ch)) {
            send_to_char("Gathering that much could be fatal.\n\r", ch);
            return;
        }

        check_gather_flag(ch);      /* Preservers are hunted/felt also */

        gather_msg_self = find_ex_description("[GATHER_SELF_MSG_SELF]", ch->ex_description, TRUE);
        gather_msg_room = find_ex_description("[GATHER_SELF_MSG_ROOM]", ch->ex_description, TRUE);

        if (gather_msg_self && gather_msg_room) {
            act(gather_msg_self, FALSE, ch, 0, 0, TO_CHAR);
            act(gather_msg_room, TRUE, ch, 0, 0, TO_ROOM);
        } else if (GET_RACE(ch) == RACE_GALITH) {
            act("You begin to dance, swirling and flowing on top of your bond to the land.\n\r"
                "You collapse in weakness as you rip yourself away from the life giving bond.",
                FALSE, ch, 0, 0, TO_CHAR);
            act("$n begins to dance, swirling and flowing in a state of ecstasy.\n\r"
                "$n lurches suddenly and collapses weakly to the ground.", TRUE, ch, 0, 0, TO_ROOM);
            set_char_position(ch, POSITION_RESTING);
        } else {
            act("You double over in pain, as you wrench the lifeforce from your body.", FALSE, ch,
                0, 0, TO_CHAR);
            act("$n suddenly doubles over, clutching $s stomach in pain.", TRUE, ch, 0, 0, TO_ROOM);
        }

        sprintf(buf, "%s gathers %.f from self.\n\r", GET_NAME(ch), mana);
        send_to_monitor(ch, buf, MONITOR_MAGICK);
        snprintf(buf, sizeof(buf), "%s gathers for magick from %sself.", MSTR(ch, short_descr),
                 HMHR(ch));
        send_to_empaths(ch, buf);

        // Give 'em the mana
        adjust_mana(ch, mana);

        // Do the damage
        dam_perc = cost_preserving / (float) GET_MAX_HIT(ch);
        set_hit(ch, MAX(GET_HIT(ch) - cost_preserving, 1));
        set_stun(ch, MAX(GET_STUN(ch) - (GET_MAX_STUN(ch) * dam_perc), 0));
        set_move(ch, MAX(GET_MOVE(ch) - (GET_MAX_MOVE(ch) * dam_perc), 0));

    }

    update_pos(ch);             // Always need to do this after adjusting stats

    // At this point they've actually gathered and we need to do a skillcheck
    // to see if they became more proficient as a result.
    if (has_skill(ch, SKILL_GATHER) && !skill_success(ch, NULL, ch->skills[SKILL_GATHER]->learned)
        && !number(0, 2))
        gain_skill(ch, SKILL_GATHER, 1);
}

void
check_gather_flag(CHAR_DATA * ch)
{
    int ct;
    char buf[256];
    CHAR_DATA *tmp;
    struct descriptor_data *d;

    extern void send_empathic_msg(CHAR_DATA *, char *);

    ct = room_in_city(ch->in_room);

#ifdef TEMPLARS_MAY_GATHER
    // Templars are immune from retribution when gathering in their own city
    if (is_templar_in_city(ch)) 
        return;
#endif

    /* if Sorcerer-Kings detect gathering, they flag forever */

    if ((ct == CITY_TULUK) || (ct == CITY_ALLANAK)) {
        if (!affected_by_spell(ch, PSI_BARRIER)) {
            act("A cold, sudden presence warns you that you have been observed.", FALSE, ch, 0,
                0, TO_CHAR);
            for (d = descriptor_list; d; d = d->next)
                if (!d->connected && d->character && IS_TRIBE(d->character, city[ct].tribe)) {
                    sprintf(buf, "%s has gathered magick in the city.", MSTR(ch, short_descr));
                    send_empathic_msg(d->character, buf);
                }
            if (!IS_SET(ch->specials.act, city[ct].c_flag))
                MUD_SET_BIT(ch->specials.act, city[ct].c_flag);
        }
    }

    /* search local rooms for observers */

    for (tmp = ch->in_room->people; tmp; tmp = tmp->next_in_room) {
        if (tmp->master == ch && affected_by_spell(tmp, SPELL_CHARM_PERSON))
            continue;

        if ((tmp != ch) && (CAN_SEE(tmp, ch)) && (!(IS_SET(tmp->specials.act, CFL_UNDEAD)))
            && (!(GET_RACE(tmp) == RACE_SHADOW)) && (IS_NPC(tmp))
            && (GET_GUILD(tmp) != GUILD_DEFILER) && (!IS_IN_SAME_TRIBE(tmp, ch))
            && ((GET_RACE_TYPE(tmp) == RTYPE_HUMANOID) || (GET_RACE(tmp) == RACE_MANTIS))) {
            if (tmp)
                cmd_shout(tmp, "Sorcerer!", 0, 0);
            if (city[ct].c_flag) {
                if (!IS_SET(ch->specials.act, city[ct].c_flag))
                    MUD_SET_BIT(ch->specials.act, city[ct].c_flag);
            } else {
                hit(tmp, ch, TYPE_UNDEFINED);
            }
        }
    }
}

