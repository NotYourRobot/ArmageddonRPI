/*
 * File: WATCH.H
 * Usage: Defines for Watching directions & people
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
 * 03/17/2006: Created -Morgenes
 */

/* Check to see if the char is watching anything */
bool is_char_watching(CHAR_DATA * ch);

/* returns if the char is watching a particular direction */
bool is_char_watching_dir(CHAR_DATA * ch, int dir);

/* returns if the character is watching in any direction */
bool is_char_watching_any_dir(CHAR_DATA * ch);

/* Get the direction the character is watching */
int get_char_watching_dir(CHAR_DATA * ch);

/* Get the victim the character is watching */
CHAR_DATA *get_char_watching_victim(CHAR_DATA * ch);

/* returns if the character is watching the specified character */
bool is_char_watching_char(CHAR_DATA * ch, CHAR_DATA * victim);

/* returns if the character is watching and can see the specified character */
bool is_char_actively_watching_char(CHAR_DATA * ch, CHAR_DATA * victim);

/* returns if the character is watching another character */
bool is_char_watching_any_char(CHAR_DATA * ch);

/* returns if the character is being watched */
bool is_char_being_watched(CHAR_DATA * ch);

/* Stop all watching for a character 
 * ch - character to stop watching
 * show_failure - shows if they aren't watching anything
 */
void stop_watching(CHAR_DATA * ch, bool show_failure);

/* stop watching without any echoes */
void stop_watching_raw(CHAR_DATA * ch);

/* stop anyone who is watching the specified character */
void stop_reverse_watching(CHAR_DATA * ch);

/* start watching an exit */
void add_exit_watched(CHAR_DATA * ch, int dir);

/* start watching the specified victim */
void start_watching_victim(CHAR_DATA * ch, CHAR_DATA * victim);

/* Start watching without any echoes */
void start_watching_victim_raw(CHAR_DATA * watcher, CHAR_DATA * target);

/* Check for success on watching the character */
bool watch_success(CHAR_DATA * watcher, CHAR_DATA * actor, CHAR_DATA * target, int bonus,
                   int opposingSkill);

/* Checks to see if anyone in the room was watching victim, but can't see him
 * if so, stops them from watching (without echo)
 */
void watch_vis_check(CHAR_DATA * victim);

/* Gives a watch status message to viewer for the specified character */
void watch_status(CHAR_DATA * ch, CHAR_DATA * viewer);

/* Derive a skill penalty (in %) for ch watching you */
int get_watched_penalty(CHAR_DATA * ch);

