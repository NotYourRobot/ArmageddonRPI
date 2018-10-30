/*
 * File: object.h */
int get_room_skill_bonus(ROOM_DATA * search_room, int skillno);

void extract_wagon(OBJ_DATA * obj);
void sflag_to_char(CHAR_DATA * ch, long sflag);
void value_object(CHAR_DATA *ch, OBJ_DATA *tar_obj, bool showValue, bool showWeight, bool showMaker);

