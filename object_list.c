/*
 * File: object_list.c */
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "object_list.h"
#include "structs.h"
#include "db.h"
#include "gamelog.h"

OBJ_DATA *object_list = 0;      /* linked list of all created objects */
int validate_object_list();
int in_obj_list(OBJ_DATA * obj);


void
object_list_insert(OBJ_DATA * obj)
{
    obj->prev = (OBJ_DATA *) 0;
    obj->next = object_list;

    object_list = obj;
    if (obj->next)
        obj->next->prev = obj;
}

OBJ_DATA *
object_list_find_by_func(obj_eq_func func, void *user_data)
{
    OBJ_DATA *tmp;
    for (tmp = object_list; tmp; tmp = tmp->next) {
        if (func(tmp, user_data))
            return tmp;
    }

    return NULL;
}

int
validate_object_list()
{
    OBJ_DATA *tmp, *tmp2;
    int tind = 0, t2ind = 0;

    for (tmp = object_list; tmp; tmp = tmp->next) {
        tind++;
        for (tmp2 = object_list; tmp2 && tmp2 != tmp; tmp2 = tmp2->next) {
            t2ind++;
            if (tmp2 == tmp->next) {
                gamelogf("object_list loop at %d, %d", tind, t2ind);
                assert(0);
                return 0;
            }
        }
    }

    return 0;
}

int
in_obj_list(OBJ_DATA * obj)
{
    OBJ_DATA *tmp;
    for (tmp = object_list; tmp; tmp = tmp->next) {
        if (tmp == obj)
            return 1;
    }

    return 0;
}

void
unboot_object_list(void)
{
    int nr;

    while (object_list)
        extract_obj(object_list);

    for (nr = 0; nr < top_of_obj_t; nr++) {
        free(obj_default[nr].name);
        free(obj_default[nr].short_descr);
        free(obj_default[nr].long_descr);
        free(obj_default[nr].description);

        free_specials(obj_default[nr].programs);
        free_edesc_list(obj_default[nr].exdesc);
    }

    free(obj_default);
    free(obj_index);
}

