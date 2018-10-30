/*
 * File: PARSER.H
 * Usage: Definitions and prototypes for parser functions.
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

#ifdef __cplusplus
extern "C" {
#endif

#ifndef __PARSER_INCLUDED
#define __PARSER_INCLUDED

int get_command(char *command);
int parse_command(CHAR_DATA * ch, const char *arg);
int search_block(char *arg, const char * const list[], bool exact);
int old_search_block(const char *argument, int begin, int length, const char * const list[], int mode);
char lower(char c);
int fill_word(char *argument);
void menu_interpreter(struct descriptor_data *d, char *arg);
int is_abbrev(const char *arg1, const char *arg2);
int karma_race_ok(unsigned long int rflags, char race);

/*
extern char *strdup(const char *s1);*/


struct command_info
{
    void (*command_pointer) (CHAR_DATA *, const char *, Command, int);
    byte minimum_position;
    byte minimum_level;
    char *name;
    char *js_filename;
    char *js_function;
};

/*void cmd_echoall(CHAR_DATA *ch, char *arg, int cmd);*/


void cmd_action(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_addkeywor(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_addkeyword(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_advance(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_afind(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_ainfo(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_alias(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_analyze(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_areas(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_arrange(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_aset(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_asl(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_assess(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_assist(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_at(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_award(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_backsave(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_ban(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_barter(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_beep(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_behead(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_biography(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_bite(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_break(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_brief(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_bsave(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_bset(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_bug(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_bury(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_buy(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_call(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_cast(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_cat(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_cd(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_cdelete(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_cease(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_cgive(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_change(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_chardelet(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_chardelete(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_cityset(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_citystat(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_clanleader(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_clansave(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_clanset(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_clean(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_close(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_commune(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_compact(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_consider(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_count(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_craft(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_credits(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_crevoke(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_criminal(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_cset(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_cstat(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_cunset(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_cwho(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_damage(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_delete(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_delayed_emote(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_demote(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_directions(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_discuss(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_disengage(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_dismount(CHAR_DATA * ch, const char *argment, Command cmd, int count);
//void cmd_dnstat(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_doas(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_donate(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_draw(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_drink(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_drop(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_dump(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_dwho(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_eat(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_echo(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_edit(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_email(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_emote(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_empty(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_enter(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_equipment(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_etwo(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_examine(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_execute(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_exit(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_exits(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_extinguish(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_feel(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_fill(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_flee(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_fload(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_fly(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_follow(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_force(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_freeze(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_frob(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_get(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_give(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_godcom(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_gone(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_goto(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_grab(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_grp(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_guard(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_harness(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_hayyeh(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_heal(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_hear(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_help(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_hit(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_hitch(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_idea(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_idle(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_immcmds(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_immcom(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_incriminate(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_infobar(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_insult(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_inventory(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_invis(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_iooc(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_junk(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_jseval(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_keyword(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_kill(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_knock(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_land(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_last(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_leave(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_light(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_list(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_listen(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_load(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_lock(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_log(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_look(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_lower(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_ls(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_mail(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_make(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_mcopy(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_mcreate(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_mdup(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_memory(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_mercy(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_mldesc(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_mname(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_monitor(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_motd(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_mount(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_move(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_msave(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_msdesc(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_mupdate(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_news(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_nonet(CHAR_DATA * ch, const char *argument, Command cmd, int count);
// void cmd_nonews(CHAR_DATA * ch, const char *argument, Command cmd, int count);
//void cmd_warnnews(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_nosave(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_not_here(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_nowish(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_nuke(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_oaffect1(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_oaffect2(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_ocopy(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_ocreate(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_odelete(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_offer(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_oindex(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_oldesc(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_oname(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_ooc(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_open(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_order(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_osave(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_osdesc(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_oset(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_ostat(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_otype(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_ounset(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_oupdate(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_pack(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_page(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_pagelength(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_pardon(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_path(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_peek(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_pemote(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_pfind(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_pilot(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_pinfo(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_plant(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_play(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_players(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_plist(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_plmax(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_poofin(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_poofout(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_pose(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_pour(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_promote(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_prompt(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_psi(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_pulse(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_purge(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_put(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_quaff(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_qui(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_quiet(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_quit(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_races(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_raise(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_rcopy(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_rcreate(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_rdelete(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_read(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_rebel(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_recite(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_recruit(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_release(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_relieve(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_remove(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_repair(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_reply(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_reroll(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_reset(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_rest(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_restore(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_resurrect(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_return(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_review(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_rm(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_roll(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_rp(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_rpcomment(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_rsave(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_rs(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_rset(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_rstat(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_rtwo(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_run(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_runset(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_rupdate(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_rzclear(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_rzreset(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_rzsave(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_salvage(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_save(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_say(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_scan(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_score(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_scribble(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_send(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_sense_presence(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_server(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_setguild(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_sethp(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_setsex(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_shave(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_sheathe(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_shoot(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_shout(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_show(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_shutdow(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_shutdown(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_sing(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_sip(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_sit(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_skill(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_skillreset(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_skills(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_skin(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_slee(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_sleep(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_smoke(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_sneak(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_sniff(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_snoop(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_stand(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_stat(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_stop(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_store(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_switch(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_symbol(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_systat(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_system(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_talk(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_taste(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_tax(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_tdesc(CHAR_DATA *ch, const char *argument, Command cmd, int count);
void cmd_teach(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_tell(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_tenebrae(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_think(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_time(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_touch(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_tran(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_trans(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_tribes(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_trigger(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_typo(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_unapprove(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_unban(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_unharness(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_unhitch(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_unload(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_unlock(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_unpack(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_unstore(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_unveil(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_use(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_users(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_view(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_vis(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_wake(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_walk(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_watch(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_wclear(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_wear(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_weather(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_where(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_whisper(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_who(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_wield(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_wish(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_wizhelp(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_wizlist(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_wizlock(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_wreset(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_write(CHAR_DATA * ch, const char *arg, Command cmd, int count);
void cmd_wsave(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_wset(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_wupdate(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_wzsave(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_zassign(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_zclear(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_zclose(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_zopen(CHAR_DATA * ch, const char *argment, Command cmd, int count);
void cmd_zorch(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_zreset(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_zsave(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_zset(CHAR_DATA * ch, const char *argument, Command cmd, int count);
void cmd_bldcom(CHAR_DATA * ch, const char *argument, Command cmd, int count);

extern struct command_info cmd_info[];

int is_cmd_oob_command(CHAR_DATA *ch, const char *cmd);
int is_oob_command(DESCRIPTOR_DATA * desc, const char *input);
const char *get_alias(CHAR_DATA * ch, const char *arg);

int parse_command_with_flags(CHAR_DATA * ch, const char *arg, int queue_flags);
void push_queued_command(CHAR_DATA *ch, const char *cmd, int queue_flags);
char * pop_queued_command(CHAR_DATA *ch);
void free_queued_command( QUEUED_CMD *qcmd );
int flush_queued_commands(CHAR_DATA *ch, bool force_all);
void show_queued_commands( CHAR_DATA *viewer, CHAR_DATA *ch );
int num_queued_commands( CHAR_DATA *ch );


#endif

#if 0
void cmd_zcommand(CHAR_DATA * ch, const char *argument, int cmd);
#endif /* 0 */

#ifdef __cplusplus
}
#endif
