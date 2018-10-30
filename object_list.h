/*
 * File: object_list.h */
struct obj_data;
typedef int obj_eq_func(struct obj_data *obj, void *user_data);

void object_list_insert(struct obj_data *obj);

struct obj_data *object_list_find_by_func(obj_eq_func func, void *user_data);

void unboot_object_list(void);

extern struct obj_data *object_list;    // FIXME:  TO BE REMOVED

