/*
 * File: reporting.h */
struct game_stats {
    unsigned int player_count;
    unsigned int staff_count;
    unsigned int memory_alloc;
    unsigned int avg_main_loop;
    unsigned int edescs;
    unsigned int objects;
    unsigned int objects_instances;
    unsigned int npcs;
    unsigned int npc_instances;

    // Absolute counts (cleared each interval)
    unsigned int net_bytes_sent;
    unsigned int net_bytes_recv;
    unsigned int new_accounts;
    unsigned int spells_cast;
    unsigned int combat_start;
    unsigned int deaths;
    unsigned int pc_deaths;
    unsigned int damage;
    unsigned int pc_damage;
    unsigned int coin_spent;
    unsigned int wishes;
    unsigned int comm_spoken;
    unsigned int comm_emote;
};

extern struct game_stats global_game_stats;

void init_game_stats();
void init_game_stats_table();
void report_game_stats();

