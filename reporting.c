/*
 * File: reporting.c */
#include <malloc.h>
#include <stdio.h>
#include <time.h>
#include <string.h>

#include "core_structs.h"
#include "structs.h"
#include "info.h"
#include "reporting.h"
#include "object_list.h"
#include "utility.h"

struct game_stats global_game_stats;

extern int perf_logging;

void
init_game_stats_table()
{
return;
}

void
init_game_stats()
{
    memset(&global_game_stats, 0, sizeof(global_game_stats));
}

void
collect_global_stats()
{
    struct mallinfo info = mallinfo();
    OBJ_DATA *obj;
    struct timeval start;

    perf_enter(&start, "collect_global_stats()");
    global_game_stats.memory_alloc = info.uordblks;
    gather_population_stats();

    global_game_stats.objects_instances = 0;
    for (obj = object_list; obj; obj = obj->next)
        global_game_stats.objects_instances++;

    global_game_stats.npcs = top_of_npc_t;
    global_game_stats.objects = top_of_obj_t;
    perf_exit("collect_global_stats()", start);
}

void
report_game_stats()
{
	return; // send to log file if wanted
    char sql[2048];
    static time_t last_time = 0;
    time_t now = time(0);
    struct timeval start_time;

    if (!perf_logging) return;
    if (last_time == now || (now % 10) != 0)
        return;

    perf_enter(&start_time, "report_game_stats()");
    last_time = now;

    collect_global_stats();
/*
    snprintf(sql, sizeof(sql),
        "INSERT INTO game_statistics SET "
           // Gauges
           "player_count   = '%d', "
           "staff_count    = '%d', "
           "memory_alloc   = '%d', "
           "avg_main_loop  = '%d', "
           "edescs         = '%d', "
           "objects        = '%d', "
           "objects_instances = '%d', "
           "npcs           = '%d', "
           "npc_instances  = '%d', "

           // Absolute counts (cleared each interval)
           "net_bytes_sent = '%d', "
           "net_bytes_recv = '%d', "
           "new_accounts   = '%d', "
           "spells_cast    = '%d', "
           "combat_start   = '%d', "
           "deaths         = '%d', "
           "pc_deaths      = '%d', "
           "damage         = '%d', "
           "pc_damage      = '%d', "
           "coin_spent     = '%d', "
           "wishes         = '%d', "
           "comm_spoken    = '%d', "
           "comm_emote     = '%d'",
           global_game_stats.player_count,
           global_game_stats.staff_count,
           global_game_stats.memory_alloc,
           global_game_stats.avg_main_loop,
           global_game_stats.edescs,
           global_game_stats.objects,
           global_game_stats.objects_instances,
           global_game_stats.npcs,
           global_game_stats.npc_instances,

           // Absolute counts (cleared each interval)
           global_game_stats.net_bytes_sent,
           global_game_stats.net_bytes_recv,
           global_game_stats.new_accounts,
           global_game_stats.spells_cast,
           global_game_stats.combat_start,
           global_game_stats.deaths,
           global_game_stats.pc_deaths,
           global_game_stats.damage,
           global_game_stats.pc_damage,
           global_game_stats.coin_spent,
           global_game_stats.wishes,
           global_game_stats.comm_spoken,
           global_game_stats.comm_emote);

    db_query_handle_error(sql);
    perf_exit("report_game_stats()", start_time);
    */
}

