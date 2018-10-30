/*
 * File: WAGON_SAVE.C
 * Usage: Saving & manipulating the wagon_data structure
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
/* #define FD_SETSIZE 512
*/


#include <sys/types.h>
#include <sys/dir.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>

#include "structs.h"
#include "clan.h"
#include "comm.h"
#include "parser.h"
#include "db.h"
#include "utils.h"
#include "limits.h"
#include "guilds.h"
#include "skills.h"
#include "vt100c.h"
#include "cities.h"
#include "handler.h"
#include "event.h"
#include "pc_file.h"
#include "modify.h"
#include "wagon_save.h"
#include "object_list.h"

#ifdef WAGON_SAVE

WAGON_DATA *wagon_list;

OBJ_DATA *
find_wagon_for_room( ROOM_DATA *room ) {
    OBJ_DATA *obj;

    if( !room ) return NULL;

    for( obj = object_list; obj; obj = obj->next ) {
       if( obj->obj_flags.type == ITEM_WAGON
        && obj->in_room
        && (room->number == obj->obj_flags.value[0]
         || room->number == obj->obj_flags.value[2])) {
           return obj;
       }
    }

    return NULL;
}

OBJ_DATA *
find_exitable_wagon_for_room( ROOM_DATA *room ) {
    OBJ_DATA *obj = NULL;
    OBJ_DATA *wagon = NULL;

    if( !room ) return NULL;

    for( obj = object_list; obj; obj = obj->next ) {
       if( obj->obj_flags.type == ITEM_WAGON
        && obj->in_room
        && room->number == obj->obj_flags.value[0]) {
           if( wagon == NULL 
            || (wagon != NULL 
             && IS_SET(wagon->obj_flags.value[3], WAGON_NO_LEAVE)
             && !IS_SET(obj->obj_flags.value[3], WAGON_NO_LEAVE)))
              wagon = obj;
       }
    }

    return wagon;
}

OBJ_DATA *
find_pilotable_wagon_for_room( ROOM_DATA *room ) {
    OBJ_DATA *obj;

    if( !room ) return NULL;

    for( obj = object_list; obj; obj = obj->next ) {
       if( obj->obj_flags.type == ITEM_WAGON
        && obj->in_room
        && room->number == obj->obj_flags.value[2]) {
           return obj;
       }
    }

    return NULL;
}

/* look for a wagon save in a specified room, regardless of mounts */
WAGON_DATA *
find_wagon_save_by_room(OBJ_DATA * wagon, int room_vnum)
{
    WAGON_DATA *pWagonSave;

    if (!wagon)
        return NULL;

    for (pWagonSave = wagon_list; pWagonSave; pWagonSave = pWagonSave->next)
        if (pWagonSave->wagon_vnum == obj_index[wagon->nr].vnum
            && pWagonSave->room_vnum == room_vnum)
            return pWagonSave;

    return NULL;
}


/* look for a wagon save, using compare_wagon_save() to analyze mounts */
WAGON_DATA *
find_wagon_save_by_compare(OBJ_DATA * wagon, int room_vnum)
{
    WAGON_DATA *pWagonSave;

    if (!wagon)
        return NULL;

    for (pWagonSave = wagon_list; pWagonSave; pWagonSave = pWagonSave->next)
        if (compare_wagon_save(pWagonSave, wagon, room_vnum))
            return pWagonSave;

    return NULL;
}


/* look for a wagon save, based on object alone */
WAGON_DATA *
find_wagon_save(OBJ_DATA * wagon)
{
    WAGON_DATA *pWagonSave;

    if (!wagon)
        return NULL;

    for (pWagonSave = wagon_list; pWagonSave; pWagonSave = pWagonSave->next)
        if (pWagonSave->wagon_vnum == obj_index[wagon->nr].vnum)
            return pWagonSave;

    return NULL;
}


/* find an attached mount in a wagon save */
ATTACHED_MOUNT_DATA *
find_attached_mount(ATTACHED_MOUNT_DATA * pAM, CHAR_DATA * npc)
{
    if (!pAM || !npc)
        return NULL;

    for (; pAM; pAM = pAM->next)
        if (pAM->mount_vnum == npc_index[npc->nr].vnum)
            return pAM;

    return NULL;
}


/* add an attached mount to a wagon save */
void
add_attached_mount(WAGON_DATA * pWS, CHAR_DATA * npc)
{
    ATTACHED_MOUNT_DATA *pAM;

    if (!pWS || !npc)
        return;

    CREATE(pAM, ATTACHED_MOUNT_DATA, 1);
    pAM->mount_vnum = npc_index[npc->nr].vnum;
    pAM->next = pWS->mounts;
    pWS->mounts = pAM;
}


/* add an attached mount by number */
void
add_attached_mount_nr(WAGON_DATA * pWS, int vnum)
{
    ATTACHED_MOUNT_DATA *pAM;

    if (!pWS)
        return;

    CREATE(pAM, ATTACHED_MOUNT_DATA, 1);
    pAM->mount_vnum = vnum;
    pAM->next = pWS->mounts;
    pWS->mounts = pAM;
}


/* remove an attached mount from the list of mounts on a wagon */
void
remove_attached_mount(WAGON_DATA * pWS, CHAR_DATA * npc)
{
    ATTACHED_MOUNT_DATA *pAM, *pAMNext;
    char buf[MAX_STRING_LENGTH];

    if (!pWS || !npc)
        return;

    if (pWS->mounts->mount_vnum == npc_index[npc->nr].vnum) {
        pAM = pWS->mounts;
        pWS->mounts = pWS->mounts->next;
        free(pAM);
        return;
    } else {
        for (pAM = pWS->mounts; pAM; pAM = pAMNext) {
            pAMNext = pAM->next;

            if (pAMNext && (pAMNext->mount_vnum == npc_index[npc->nr].vnum)) {
                pAM->next = pAMNext->next;
                free(pAMNext);
                pAMNext = NULL;
                return;
            }
        }
    }

    sprintf(buf, "remove_attached_mount: unable to find mount #%d for wagon" " #%d in room #%d",
            npc_index[npc->nr].vnum, pWS->wagon_vnum, pWS->room_vnum);

    gamelog(buf);
}


/* compares a wagon save vs objects, to determine if they are the same */
bool
compare_wagon_save(WAGON_DATA * pWS, OBJ_DATA * wagon, int room_vnum)
{
    ROOM_DATA *room;
    CHAR_DATA *mount;

    if (pWS->wagon_vnum != obj_index[wagon->nr].vnum
        || pWS->to_room_vnum != wagon->obj_flags.value[0]
        || pWS->room_vnum != room_vnum)
        return FALSE;

    if ((room = get_room_num(room_vnum)) == NULL)
        return FALSE;

    for (mount = room->people; mount; mount = mount->next_in_room)
        if (mount->specials.harnessed_to == wagon && !find_attached_mount(pWS->mounts, mount))
            return FALSE;

    return TRUE;
}

int
number_wagons_same_room(WAGON_DATA * pWS)
{
    WAGON_DATA *pTmp;
    int count;

    count = 1;
    for (pTmp = wagon_list; pTmp; pTmp = pTmp->next)
        if (pTmp != pWS && pTmp->room_vnum == pWS->room_vnum && pTmp->wagon_vnum == pWS->wagon_vnum)
            count++;

    return count;
}

/* adds a wagon and the mounts attached to it to the list of wagons */
void
add_wagon_save(OBJ_DATA * wagon)
{
    WAGON_DATA *pWagonSave;
    struct harness_type *harnassed;

    if (!wagon)
        return;

    if (!wagon->in_room)
        return;

    if (wagon->obj_flags.type != ITEM_WAGON)
        return;

    CREATE(pWagonSave, WAGON_DATA, 1);
    pWagonSave->wagon_nr = wagon->nr;
    pWagonSave->wagon_vnum = obj_index[wagon->nr].vnum;
    pWagonSave->to_room_vnum = wagon->obj_flags.value[0];
    pWagonSave->room_vnum = wagon->in_room->number;
    pWagonSave->direction = wagon->obj_flags.temp;
    pWagonSave->hull_damage = wagon->obj_flags.value[4];
    pWagonSave->wheel_damage = 0;
    pWagonSave->tongue_damage = 0;
    pWagonSave->next = wagon_list;
    wagon_list = pWagonSave;

    for (harnassed = wagon->beasts; harnassed; harnassed = harnassed->next)
        add_attached_mount(pWagonSave, harnassed->beast);
}


/* removes a wagon and the mounts attached to it from the list of wagons */
void
remove_wagon_save(OBJ_DATA * wagon)
{
    WAGON_DATA *pWagonSave, *pWSNext;
    ATTACHED_MOUNT_DATA *pMount;

    if (!wagon || !wagon_list)
        return;

    if (!wagon->in_room) {
        gamelog("Tried to remove wagon from wagon_list, not in a room");
        return;
    }

    /* first in list */
    if (compare_wagon_save(wagon_list, wagon, wagon->in_room->number)) {
        pWagonSave = wagon_list;

        while (pWagonSave->mounts) {
            pMount = pWagonSave->mounts;
            pWagonSave->mounts = pMount->next;
            free(pMount);
        }

        wagon_list = wagon_list->next;
        free(pWagonSave);
        return;
    } else {                    /* search the list */

        for (pWagonSave = wagon_list; pWagonSave->next; pWagonSave = pWSNext) {
            pWSNext = pWagonSave->next;
            if (compare_wagon_save(pWSNext, wagon, wagon->in_room->number)) {
                while (pWagonSave->mounts) {
                    pMount = pWagonSave->mounts;
                    pWagonSave->mounts = pMount->next;
                    free(pMount);
                }

                pWagonSave->next = pWSNext->next;
                pWSNext->next = NULL;
                free(pWSNext);
                return;
            }
        }
    }
}


/*
 * recursive call to save wagons inorder
 * (not necessary, but a decent way to go)
 */
void
write_wagon_save(FILE * fp, WAGON_DATA * pWS)
{
    ATTACHED_MOUNT_DATA *pAM;

    if (!pWS)
        return;

    if (pWS->next)
        write_wagon_save(fp, pWS->next);

    if (pWS->wagon_vnum) {
        fprintf(fp, "%d %d %d %d %d %d %d", pWS->wagon_vnum, pWS->to_room_vnum, pWS->room_vnum,
                pWS->direction, pWS->hull_damage, pWS->wheel_damage, pWS->tongue_damage);

        for (pAM = pWS->mounts; pAM; pAM = pAM->next)
            fprintf(fp, " %d", pAM->mount_vnum);

        fprintf(fp, "\n");
    }
}


/* save all the wagons */
void
save_wagon_saves()
{
    FILE *fp;

    if ((fp = fopen("data_files/wagons", "w")) == NULL) {
        gamelog("Unable to open data_files/wagons for write");
        exit(0);
    }

    write_wagon_save(fp, wagon_list);
    fclose(fp);
}


/* load all the wagon saves into memory */
void
load_wagon_saves()
{
    FILE *fp;
    const char *pline;
    char line[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];
    int wagon_vnum, room_vnum, to_room_vnum;
    int direction, hull_damage, wheel_damage, tongue_damage;
    WAGON_DATA *pWS;

    if ((fp = fopen("data_files/wagons", "r")) == NULL) {
        gamelog("No wagons found to load.");
        return;
    }

    wagon_list = NULL;
    while (!feof(fp)) {
        fgets(line, MAX_STRING_LENGTH, fp);
        pline = line;

        pline = one_argument(pline, arg, sizeof(arg));
        wagon_vnum = atoi(arg);
        pline = one_argument(pline, arg, sizeof(arg));
        to_room_vnum = atoi(arg);
        pline = one_argument(pline, arg, sizeof(arg));
        room_vnum = atoi(arg);

        pline = one_argument(pline, arg, sizeof(arg));
        if (arg[0] != '\0') {
            direction = atoi(arg);
            pline = one_argument(pline, arg, sizeof(arg));
            hull_damage = atoi(arg);
            pline = one_argument(pline, arg, sizeof(arg));
            wheel_damage = atoi(arg);
            pline = one_argument(pline, arg, sizeof(arg));
            tongue_damage = atoi(arg);
        } else {
            direction = -1;
            hull_damage = 0;
            wheel_damage = 0;
            tongue_damage = 0;
        }

        CREATE(pWS, WAGON_DATA, 1);
        pWS->wagon_vnum = wagon_vnum;
        pWS->to_room_vnum = to_room_vnum;
        pWS->room_vnum = room_vnum;
        pWS->direction = direction;
        pWS->hull_damage = hull_damage;
        pWS->wheel_damage = wheel_damage;
        pWS->tongue_damage = tongue_damage;
        pWS->next = wagon_list;
        wagon_list = pWS;
        while (*pline != '\0') {
            pline = one_argument(pline, arg, sizeof(arg));
            add_attached_mount_nr(pWS, atoi(arg));
        }
    }

    fclose(fp);
}


int add_harness(CHAR_DATA * mount, OBJ_DATA * wagon);

/* load the wagon objects into the world */
void
boot_wagons(int zone)
{
    OBJ_DATA *obj;
    ROOM_DATA *room;
    WAGON_DATA *pWS;
    CHAR_DATA *mount;
    ATTACHED_MOUNT_DATA *pAM;

    for (pWS = wagon_list; pWS; pWS = pWS->next) {
        if (pWS->room_vnum / 1000 != zone && zone != -1)
            continue;

        if ((room = get_room_num(pWS->room_vnum)) == NULL)
            continue;

        if (zone != -1) {
            int existing;
            int nr_in_room;

            nr_in_room = number_wagons_same_room(pWS);

            existing = 0;
            for (obj = room->contents; obj; obj = obj->next_content)
                if (obj_index[obj->nr].vnum == pWS->wagon_vnum)
                    existing++;

            if (existing >= nr_in_room)
                continue;
        }

        if ((obj = read_object(pWS->wagon_vnum, VIRTUAL)) == NULL)
            continue;

        if (!IS_WAGON(obj)) {
            shhlogf("Bogus non-wagon object in wagons list:  %d", pWS->wagon_vnum);
            extract_obj(obj);
            continue;
        }

        pWS->wagon_nr = obj->nr;        // Keep tabs on the actual object

        obj_to_room(obj, room);

        obj->obj_flags.value[0] = pWS->to_room_vnum;
        /* pWS->to_room_vnum = obj->obj_flags.value[0]; */

        if (pWS->direction == -1) {
            pWS->direction = obj->obj_flags.temp;
        }

        /* use the values we saved */
        obj->obj_flags.temp = pWS->direction;
        obj->obj_flags.value[4] = pWS->hull_damage;

        for (pAM = pWS->mounts; pAM; pAM = pAM->next) {
            if ((mount = read_mobile(pAM->mount_vnum, VIRTUAL)) != NULL) {
                char_to_room(mount, room);
                add_harness(mount, obj);
            }
        }
    }

    /* kill this later */
    /*
     * gamelog( "Writing wagons" );
     * save_wagon_saves(); */
}

void
unboot_wagons()
{
    while (wagon_list) {
        WAGON_DATA *wd = wagon_list;
        wagon_list = wd->next;

        while (wd->mounts) {
            ATTACHED_MOUNT_DATA *md = wd->mounts;
            wd->mounts = md->next;
            free(md);
        }

        free(wd);
    }
}

#endif

