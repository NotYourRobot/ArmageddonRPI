/*
 * File: ECONOMY.C
 * Usage: Functions for the global economy / pricing of Zalanthas.
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

#include <stdio.h>
#include <string.h>
#include "economy.h"
#include "utility.h"

extern struct attribute_data obj_type[];
extern struct attribute_data obj_material[];

// Collects point-of-sale purchase data
void
update_purchase_data(int ct, int com_type, int item_type, int sold_to_npc)
{
    if ((ct >= 0 && ct < MAX_CITIES) && (com_type >= 0 && com_type < MAX_MATERIAL)
        && (item_type >= 0 && item_type < MAX_ITEM_TYPE)) {
        if (sold_to_npc)
            economy[ct].current_supply[com_type][item_type]++;
        else
            economy[ct].current_demand[com_type][item_type]++;
    }
}

// Refreshes the purchase data statistics
void
update_economy()
{
    int i, j, k;
    static time_t lastUpdate;

    /* Logic here is to only update at the start of a new week.
     *
     * Look for BEFORE_DAWN on the first day of the week (Weekday 0)
     * If it's not that time and day, don't update.
     */
    if (!(time_info.hours == NIGHT_EARLY && !(time_info.day % MAX_DOTW)))
        return;

    /* Look for the delta between now and when we last updated.  If it's at
     * least a week's worth (or more), update the time we updated and update.
     */
    if (time(0) >= (RT_ZAL_DAY * MAX_DOTW) + lastUpdate)
        lastUpdate = time(0);
    else
        return;

    for (i = 0; i < MAX_CITIES; i++)
        for (j = 0; j < MAX_MATERIAL; j++)
            for (k = 0; k < MAX_ITEM_TYPE; k++) {
                // Recalculate the average
                economy[i].average_demand[j][k] =
                    (economy[i].average_demand[j][k] + economy[i].last_demand[j][k] +
                     economy[i].last2_demand[j][k] + economy[i].current_demand[j][k]) / 4;

                // Refresh the historical collection periods to reflect we're
                // starting a new collection period.
                economy[i].last2_demand[j][k] = economy[i].last_demand[j][k];
                economy[i].last_demand[j][k] = economy[i].current_demand[j][k];

                // Zero out the current collection period values since we're
                // starting over.
                economy[i].current_demand[j][k] = 0;


                // Recalculate the average
                economy[i].average_demand[j][k] =
                    (economy[i].average_demand[j][k] + economy[i].last_supply[j][k] +
                     economy[i].last2_demand[j][k] + economy[i].current_demand[j][k]) / 4;

                // Refresh the historical collection periods to reflect we're
                // starting a new collection period.
                economy[i].last2_supply[j][k] = economy[i].last_supply[j][k];
                economy[i].last_supply[j][k] = economy[i].current_supply[j][k];

                // Zero out the current collection period values since we're
                // starting over.
                economy[i].current_supply[j][k] = 0;
            }

}

// Returns a suggested retail price for the item based on demand
float
adjusted_cost(int ct, OBJ_DATA * item)
{
    float price, cost, supply, demand;
    int material, type;

    cost = item->obj_flags.cost;
    material = item->obj_flags.material;
    type = item->obj_flags.type;

    // This is a measure of how much players desire this type of good
    demand = calc_demand(ct, material, type);

    // This is a measure of how much players supply this type of good
    supply = calc_supply(ct, material, type);

    // Adjust price to accomodate the trends in the city's supply and demand
    // for this particular type of item.
    price = cost + cost * demand + cost * supply;

    return price;
}

float
calc_demand(int ct, int material, int type)
{
    float demand;

    // This is a measure of how much players desire this type of good
    //
    // Price is directly related to desire.  Simply put, with a steady supply,
    // when desire goes up the price goes up.
    //
    // eg.  If this number is negative, it means players desire it less now
    // than they did before.
    //
    // (n1 - n2) / (n1 + n2) = Average % change
    demand =
        (economy[ct].last_demand[material][type] -
         economy[ct].last2_demand[material][type]) / MAX((economy[ct].last_demand[material][type] +
                                                          economy[ct].last2_demand[material][type]),
                                                         1);

    return demand;
}

float
calc_supply(int ct, int material, int type)
{
    float supply;

    // This is a measure of how much players supply this type of good
    //
    // Price is inversely related to supply.  Simply put, with a steady demand,
    // when supply goes up the price goes down.
    //
    // eg.  If this number is negative, it means players are selling off a lot
    // more now than they did before.  This equates to a larger amount of
    // this good being "in stock" (supplied to the merchant).
    //
    // (n2 - n1) / (n2 + n1) = Average % change
    supply =
        (economy[ct].last2_supply[material][type] -
         economy[ct].last_supply[material][type]) / MAX((economy[ct].last2_supply[material][type] +
                                                         economy[ct].last_supply[material][type]),
                                                        1);

    return supply;
}

char *
show_city_economy(int ct, int type)
{
    static char buf[MAX_STRING_LENGTH];
    char buf2[MAX_STRING_LENGTH];
    int i, j;

    if ((ct >= 0 && ct < MAX_CITIES) && (type >= 0 && type < MAX_ITEM_TYPE)) {
        strcpy(buf, "Type: Undefined      Supply\tDemand\n\r");
        for (j = 0; j < MAX_ITEM_TYPE; j++) {
            if (obj_type[j].val == type)
                sprintf(buf, "Type: %-14s  Supply\tDemand\n\r", obj_type[j].name);
        }

        //string_safe_cat (buf, buf2, MAX_STRING_LENGTH);
        for (i = 0; i < MAX_MATERIAL; i++) {
            sprintf(buf2, "%-20s: %3.1f%%\t%3.1f%%\n\r", obj_material[i].name,
                    calc_supply(ct, i, type), calc_demand(ct, i, type));

            string_safe_cat(buf, buf2, MAX_STRING_LENGTH);
        }
    } else {
        strcpy(buf, "Illegal value to citystat.\n\r");
    }

    return buf;
}

