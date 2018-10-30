/*
 * File: MOUNT.H
 * Usage: Defines for dealing with mounts
 *
 * Copyright (C) 2006 by Catherine Rambo, Mark Tripp, and the creators
 * of DikuMUD.  May Vivadu bring them good health and many children!
 *
 *   /\    ____                    PART OF          __     ____   |\   |
 *  /__\   |___|   /\  /\    /\    __  ___  +--\   |  \   /    \  | \  |
 * /    \  |   \  /  \/  \  /--\  /    [_   |   \  |   |  |    |  |  \ |
 * \    /  |   /  \      /  \  /  \_]  [__  |___/  |__/   \____/  |   \|
 * ---------------------------------------------------------------------
 */

/* Revision history
 * 04/29/2006: Created -Morgenes
 */
bool mount_hitched_to_char(CHAR_DATA * ch, CHAR_DATA * mount);

bool mount_owned_by_ch(CHAR_DATA * ch, CHAR_DATA * mount);

CHAR_DATA *find_mount(CHAR_DATA * ch);

CHAR_DATA *get_mount_room(CHAR_DATA * ch, char *arg);

bool mount_owned_by_someone_else(CHAR_DATA * ch, CHAR_DATA * mount);

