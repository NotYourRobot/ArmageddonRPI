/*
 * File: ECONOMY.H
 * Usage: Definitions for the global economy / pricing of Zalanthas.
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

#ifndef __ECONOMY_INCLUDED
#define __ECONOMY_INCLUDED

#include "structs.h"
#include "cities.h"

// Data structure containing information about items sold based upon the
// material and type of item.
struct economy_data
{
    int last_demand[MAX_COMMOD][MAX_ITEM_TYPE]; // Previous period
    int last2_demand[MAX_COMMOD][MAX_ITEM_TYPE];        // 2nd Previous period
    int current_demand[MAX_COMMOD][MAX_ITEM_TYPE];      // Current period
    int average_demand[MAX_COMMOD][MAX_ITEM_TYPE];      // Historical average

    int last_supply[MAX_COMMOD][MAX_ITEM_TYPE]; // Previous period
    int last2_supply[MAX_COMMOD][MAX_ITEM_TYPE];        // 2nd previous period
    int current_supply[MAX_COMMOD][MAX_ITEM_TYPE];      // Current period
    int average_supply[MAX_COMMOD][MAX_ITEM_TYPE];      // Historical average
};

// Data structure containing purchase data for each city
struct economy_data economy[MAX_CITIES];

// Collects point-of-sale purchase data
void update_purchase_data(int, int, int, int);

// Refreshes the purchase data statistics
void update_economy();

// Helper functions for calculating % changes in supply or demand
float calc_supply(int, int, int);
float calc_demand(int, int, int);

// Returns a suggested retail price for the item based on demand
float adjusted_cost(int, OBJ_DATA *);

// Returns a character buffer displaying the statistics for the city
char *show_city_economy(int, int);
#endif

