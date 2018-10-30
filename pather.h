/*
 * File: pather.h */
#ifdef __cplusplus
extern "C"
{
#endif

struct PathPruner
{
    int searchkey;
    int room_fl_ex;
    int exit_fl_ex;
    int wagon_fl_ex;
};

void cmd_xpath(CHAR_DATA * ch, const char *argument, Command cmd, int count);

int generic_astar(ROOM_DATA *from, ROOM_DATA *to, int max_plys, float max_cost,
                  int exclude_room_fl, int exclude_exit_fl, int exclude_wagon_fl,
                  void (*visit_func)(ROOM_DATA *room, int depth, void *data), void *visit_data,
                  int  (*prune_func)(ROOM_DATA *room, int depth, void *data), void *prune_data,
                  char **pathtxt);

#ifdef __cplusplus
}
#endif

