/*
 * File: special_js.h */

#ifndef SPECIAL_JS_H
#define SPECIAL_JS_H

#define XP_UNIX

#ifdef __cplusplus
extern "C" {
#endif

int special_js_init(void);
int special_js_uninit(void);
int exec_js_string(CHAR_DATA *ch, const char *arg);

int start_npc_javascript_prog(CHAR_DATA * ch, CHAR_DATA * vict, int cmd, char *arg, char *file);
int start_obj_javascript_prog(CHAR_DATA * ch, CHAR_DATA * vict, OBJ_DATA * obj, int cmd, char *arg,
                              char *file);
int start_room_javascript_prog(CHAR_DATA * ch, ROOM_DATA * rm, int cmd, char *arg, char *file);
int start_room_prog(CHAR_DATA * ch, ROOM_DATA * rm, int cmd, char *arg, char *file);

#ifdef __cplusplus
}
#endif

#endif // SPECIAL_JS_H

