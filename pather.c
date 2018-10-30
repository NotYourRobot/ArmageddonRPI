/*
 * File: pather.c
 * no comments
 */

#include <stdio.h>
//#include <vector>
//#include <queue>
//#include <set>
//#include <algorithm>
#include <sys/times.h>
#include <assert.h>
#include <memory.h>

//#include <boost/pool/pool_alloc.hpp>

#include "constants.h"
#include "structs.h"
#include "comm.h"
#include "db.h"
#include "handler.h"
#include "utils.h"
#include "cmd_getopt.h"
#include "modify.h"
#include "pather.h"

static int   nodes_generated = 0;
static int   nodes_copied = 0;

void
cmd_xpath(CHAR_DATA *ch, const char *argument, Command cmd, int count)
{
    int linkfilter = 0, roomfilter = 0, wagonfilter;
    CHAR_DATA *from_ch = ch, *to_ch = NULL;
    ROOM_DATA *from = NULL, *to = NULL;
    int bhelp = 0;
    int depth = 0, maxdepth = 40;

    arg_struct pather_args[] = {
        { 'e', "exit-filter", ARG_PARSER_REQUIRED_FLAG_INT, { &linkfilter }, { (void*)exit_flag },
          "A comma-seperated list of exit flags that should be pruned from pathing." },
        { 'w', "wagon-filter", ARG_PARSER_REQUIRED_FLAG_INT, { &wagonfilter }, { (void*)wagon_flags },
          "A comma-separated list of wagon flags that should be pruned from pather." },
        { 'r', "room-filter", ARG_PARSER_REQUIRED_FLAG_INT, { &roomfilter }, { (void*)room_flag },
          "A comma-seperated list of room flags that should be pruned from pathing." },
        { 'f', "from-room", ARG_PARSER_REQUIRED_ROOM, { &from }, { 0 },
          "The room from which to begin searching." },
        { 't', "to-room", ARG_PARSER_REQUIRED_ROOM, { &to }, { 0 },
          "The room at which searching ends." },
        { 'c', "from-ch", ARG_PARSER_CHAR, { &from_ch }, { ch },
          "The character from which to begin searching." },
        { 'v', "to-ch", ARG_PARSER_REQUIRED_CHAR, { &to_ch }, { ch },
          "The target or (v)ictim of the search.  The search ends in the room they're standing in." },
        { 'h', "help",  ARG_PARSER_BOOL, { &bhelp }, { 0 }, "Show this help text." },
        { 'm', "max-depth", ARG_PARSER_REQUIRED_INT, { &maxdepth }, { 0 },
          "Maximum depth in plys to search." },
    };

    cmd_getopt(pather_args, sizeof(pather_args)/sizeof(pather_args[0]), argument);

    if (bhelp) {
        page_help(ch, pather_args, sizeof(pather_args)/sizeof(pather_args[0]));
        return;
    }

    if (!from && from_ch && from_ch->in_room)
        from = from_ch->in_room;

    if (to_ch && to_ch->in_room)
        to = to_ch->in_room;

    if (!from || !to) {
        send_to_charf(ch, "Invalid from or to rooms:  from: (%p)  to: (%p)\n\r", from, to);
        return;
    }

    nodes_generated = 0;
    nodes_copied = 0;
    cprintf(ch, "Pathing from R%d to R%d... (max plys: %d)\n\r", from->number, to->number, maxdepth);
    clock_t starttime, endtime;
    struct tms buf;
    char *pathtxt = 0;
    times(&buf);
    starttime = buf.tms_utime;
    depth = generic_astar(from, to, maxdepth, 30.0f, roomfilter, linkfilter, wagonfilter, 0, 0, 0, 0, &pathtxt);
    times(&buf);
    endtime = buf.tms_utime;
    if (depth <= 0) {
        cprintf(ch, "No path found, User Time: %llums", (unsigned long long)endtime - starttime);
    } else {
        cprintf(ch, "Path found, depth: %d, nodes: %d, copy: %d,  User Time: %llums\n\rPath:  %s\n\r", depth, nodes_generated,
                nodes_copied, (unsigned long long)endtime - starttime, pathtxt);
        free(pathtxt);
    }
}


//#include <string>
/*
struct PathNode {
    PathNode(ROOM_DATA *from, ROOM_DATA *room, int dpth, float cst, unsigned char dir, unsigned char wndx = 0) :
             from_room(from), to_room(room), cost(cst), depth(dpth), direction(dir), wagonindex(wndx) { }

    PathNode(const PathNode &rhs) { memcpy(this, &rhs, sizeof(rhs)); nodes_copied++; }

    ROOM_DATA      *from_room;
    ROOM_DATA      *to_room;
    float           cost;
    unsigned short  depth;
    unsigned char   direction;
    unsigned char   wagonindex;
    bool operator<(const PathNode &rhs) const { return depth > rhs.depth; }
};
*/
/*
struct room_num_sort {
    inline bool operator()(const PathNode &n1, const PathNode &n2) const {
        return n1.to_room->number < n2.to_room->number;
    }
};
*/

//typedef std::priority_queue<PathNode, std::vector<PathNode> > node_queue;
//typedef std::vector<PathNode>                         node_vec;
//typedef std::set<PathNode, room_num_sort>             node_set;
/*
inline bool
is_pruned(ROOM_DATA *room, const PathPruner &pruner)
{
   if (room->path_list.run_number == pruner.searchkey)
        return true;

    if (room->room_flags & pruner.room_fl_ex)
        return true;

    return false;
}

inline void
add_node(const PathNode &node, node_queue &pending_nodes, const PathPruner &pruner)
{
    node.to_room->path_list.run_number = pruner.searchkey;
    nodes_generated++;
    pending_nodes.push(node);

}


int
generate_nodes(ROOM_DATA *room, node_queue &pending_nodes, int depth, const PathPruner &pruner)
{
    char   cmd[32];
    size_t wagonindex = 0;
    for (OBJ_DATA *obj = room->contents; obj; obj = obj->next_content) {
        ROOM_DATA *newroom;
        if (!IS_WAGON(obj))
            continue;

        if (obj->obj_flags.value[3] & pruner.wagon_fl_ex) // Prune wagons by flags
            continue;

        if ((newroom = get_wagon_entrance_room(obj)) != NULL && !is_pruned(newroom, pruner)) {
            // TODO:  Make this code handle multiple objects with the same first keyword
            // more elegantly.
            add_node(PathNode(room, newroom, depth + 1, 0.1f, DIR_IN, (unsigned char)wagonindex), pending_nodes, pruner);
        }
    }

    if (room->wagon_exit && !is_pruned(room->wagon_exit, pruner)) {
        add_node(PathNode(room, room->wagon_exit, depth + 1, 0.1f, DIR_OUT), pending_nodes, pruner);
    }

    for (int i = 0; i <= DIR_DOWN; i++) {
        struct room_direction_data *ddat = room->direction[i];
        if (!ddat) continue;

        if (ddat->to_room && !is_pruned(ddat->to_room, pruner)) {
            if (ddat->exit_info & pruner.exit_fl_ex) continue;  // Prune by exit flags
            snprintf(cmd, sizeof(cmd), "%s; ", dir_name[i]);
            add_node(PathNode(room, ddat->to_room, depth + 1, 1.0f, i), pending_nodes, pruner);
        }
    }


    return 0;
}
*/
/*
static const std::string enter_str("enter ");
static const std::string leave_str("leave;");
std::string get_exit_description(const PathNode &node)
{
    std::string retval;
    size_t wagonindex = 0;

    if (!node.from_room) return retval;

    OBJ_DATA *wagon_obj = 0;
    if (node.direction == DIR_IN) {
        for (OBJ_DATA *obj = node.from_room->contents; obj; obj = obj->next_content) {
            if (obj->obj_flags.type == ITEM_WAGON) {
                if (wagonindex == node.wagonindex) {
                    wagon_obj = obj;
                    break;
                }
                wagonindex++;
            }
        }

        assert(wagon_obj);
        char objname[32];
        one_argument(OSTR(wagon_obj, name), objname, sizeof(objname));
        retval = enter_str + std::string(objname) + std::string(";");
    } else if (node.direction == DIR_OUT) {
        retval = leave_str;
    } else {
        retval = std::string(dir_name[node.direction]) + ";";
    }

    return retval;
}
*/
int
generic_astar(ROOM_DATA *from, ROOM_DATA *to, int max_plys, float max_cost,
              int exclude_room_fl, int exclude_exit_fl, int exclude_wagon_fl,
              void (*visit_func)(ROOM_DATA *room, int depth, void*), void *visit_data,
              int  (*prune_func)(ROOM_DATA *room, int depth, void*), void *prune_data,
              char **pathtxt)
{
 //   static int searchkey = 10;
 /*   node_set   visited_nodes; // Set sorted by room number
    node_queue pending_nodes;

    PathPruner pruner;
    pruner.searchkey = searchkey;
    pruner.room_fl_ex = exclude_room_fl;
    pruner.exit_fl_ex = exclude_exit_fl;
    pruner.wagon_fl_ex = exclude_wagon_fl;

    if (from == to) return 0;

    if (!from) return -1;

    from->path_list.run_number = searchkey;

    searchkey = searchkey + 1;
    pending_nodes.push(PathNode(NULL, from, 0, 0.0f, (unsigned char)0));
    std::string spath;
    do {
        if (pending_nodes.size( ) <= 0) break;

        PathNode node = pending_nodes.top( );
        pending_nodes.pop( );

        if (prune_func) {
            if (prune_func(node.to_room, node.depth, prune_data) == FALSE)
                continue;
        }

        // consider this node visited if it hasn't been pruned
        visited_nodes.insert( node );

        if (visit_func) {
            visit_func(node.to_room, node.depth, visit_data);
        }

        if (node.to_room == to) { // Success!
            if (pathtxt) {
                spath.clear();
                spath.reserve(node.depth * 10);
                node_set::const_iterator fit;
                do {
                    fit = visited_nodes.lower_bound(node);
                    if (fit == visited_nodes.end( )) break;
          
                    spath = get_exit_description(*fit) + spath;
                    node.to_room = fit->from_room;
                } while (fit->from_room != from);
          
                *pathtxt = strdup(spath.c_str());
            }
            return node.depth;
        }

        if (node.depth >= max_plys) continue;  // Don't generate nodes beyond this one
        if (node.cost  >= max_cost) continue;  // ... or this one.
        generate_nodes(node.to_room, pending_nodes, node.depth, pruner);
    } while( 1 );
*/
    return -1;
}

