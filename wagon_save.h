/*
 * File: WAGON_SAVE.H
 * Usage: Defines for wagon saves
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

/*#undef WAGON_SAVE */
#define WAGON_SAVE

typedef struct wagon_data WAGON_DATA;
typedef struct attached_mount_data ATTACHED_MOUNT_DATA;

struct wagon_data
{
    int wagon_nr;
    int wagon_vnum;
    int to_room_vnum;
    int room_vnum;
    int direction;
    int hull_damage;
    int wheel_damage;
    int tongue_damage;
    ATTACHED_MOUNT_DATA *mounts;
    WAGON_DATA *next;
};

struct attached_mount_data
{
    int mount_vnum;
    ATTACHED_MOUNT_DATA *next;
};

extern WAGON_DATA *wagon_list;

OBJ_DATA *find_exitable_wagon_for_room(ROOM_DATA *room);
OBJ_DATA *find_pilotable_wagon_for_room(ROOM_DATA *room);
OBJ_DATA *find_wagon_for_room(ROOM_DATA *room);
WAGON_DATA *find_wagon_save(OBJ_DATA * wagon);
WAGON_DATA *find_wagon_save_by_room(OBJ_DATA * wagon, int room_vnum);
WAGON_DATA *find_wagon_save_by_compare(OBJ_DATA * wagon, int room_vnum);
bool compare_wagon_save(WAGON_DATA * pWS, OBJ_DATA * wagon, int room_vnum);
void add_wagon_save(OBJ_DATA * wagon);
void remove_wagon_save(OBJ_DATA * wagon);

void add_attached_mount(WAGON_DATA * pWS, CHAR_DATA * mount);
void remove_attached_mount(WAGON_DATA * pWS, CHAR_DATA * mount);

void write_wagon_save(FILE * fp, WAGON_DATA * pWS);
void save_wagon_saves();
void load_wagon_saves();
void boot_wagons(int zone);     /* use -1 for all zones */
void unboot_wagons();

