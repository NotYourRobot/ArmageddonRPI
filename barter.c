/*
 * File: BARTER.C
 * Usage: Routines and commands for NPC merchants.
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
/*
 * Revision history Please list changes to this file here for everyone's
 * benefit. 12/16/1999 Revision history added.  -Sanvean 
 */
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>

#include "constants.h"
#include "structs.h"
#include "comm.h"
#include "handler.h"
#include "db.h"
#include "parser.h"
#include "utils.h"
#include "utility.h"
#include "skills.h"
#include "barter.h"
#include "cities.h"
#include "event.h"
#include "guilds.h"
#include "clan.h"
#include "utils.h"
#include "utility.h"
#include "db_file.h"
#include "modify.h"
#include "limits.h"
#include "memory.h"
#include "object.h"

struct barter_data *barter_list = 0;
struct shop_data *shop_index = 0;
int max_shops = 0;
int get_race_average(char *input);
int get_race_average_number(int race);
void update_shop(CHAR_DATA *shopkeeper, const struct shop_data *shop_data);


// int shop_material_check (CHAR_DATA *merchant, OBJ_DATA *item)
// {
// }

int
shop_open(int shop_nr, CHAR_DATA * merchant)
{
    // char buf[256];
    bool closed = FALSE;

    char msg[256];

    // sprintf (buf, "shop_nr: %d merchant: %s",
    // shop_nr,MSTR( merchant, short_descr ));
    // gamelog (buf);



    if (!(shop_index[shop_nr].open_hour == shop_index[shop_nr].close_hour)) {
        // sprintf (buf, "current: %d open: %d close: %d", 
        // time_info.hours, shop_index[shop_nr].open_hour,
        // shop_index[shop_nr].close_hour);
        // gamelog (buf);

        // if the shop opens before it closes
        // i.e. open at dawn, close at night
        if (shop_index[shop_nr].open_hour < shop_index[shop_nr].close_hour) {
            if (!
                (time_info.hours >= shop_index[shop_nr].open_hour
                 && time_info.hours < shop_index[shop_nr].close_hour))
                closed = TRUE;
        }
        // Shop close hour is less than open hour 
        // (i.e. close at dawn open at noon)
        else if (time_info.hours >= shop_index[shop_nr].close_hour
                 && time_info.hours < shop_index[shop_nr].open_hour)
            closed = TRUE;

        if (closed) {
            sprintf(msg, "say I am closed, come back ");

            switch (shop_index[shop_nr].open_hour) {
            case NIGHT_EARLY:
                strcat(msg, "a little before dawn.");
                break;
            case DAWN:
                strcat(msg, "at dawn.");
                break;
            case RISING_SUN:
                strcat(msg, "in the early morning.");
                break;
            case MORNING:
                strcat(msg, "a little before high sun.");
                break;
            case HIGH_SUN:
                strcat(msg, "at high sun.");
                break;
            case AFTERNOON:
                strcat(msg, "after high sun, in the early afternoon.");
                break;
            case SETTING_SUN:
                strcat(msg, "in the late afternoon.");
                break;
            case DUSK:
                strcat(msg, "at dusk.");
                break;
            case NIGHT_LATE:
                strcat(msg, "late at night.");
                break;
            default:
                sprintf(msg, "say INVALID HOUR.");
                break;
            }

            //            parse_command(merchant, "say I am closed, come back later.");
            parse_command(merchant, msg);
            // parse_command (merchant, "change ldesc is not open for
            // business");
            return FALSE;
        }
    }
    return TRUE;
}


int
get_race_average_number(int race)
{
    switch (race) {
    case RACE_HALFLING:
        return 5;
    case RACE_DWARF:
        return 8;
    case RACE_MUL:
        return 10;
    case RACE_HUMAN:
        return 10;
    case RACE_HALF_ELF:
        return 11;
    case RACE_DESERT_ELF:
    case RACE_ELF:
        return 12;
    case RACE_HALF_GIANT:
        return 31;
    default:
        return 0;
    }
}

int
get_race_average(char *input)
{
    int i;
    /*
     * char msg[MAX_STRING_LENGTH]; 
     */

    struct race_attrib
    {
        char *name;
        int size;
    };
    struct race_attrib race_list[] = {
        {"halfling", 5},
        {"dwarf", 8},
        {"mul", 10},
        {"human", 10},
        {"elf", 12},
        {"half-elf", 10},
        {"half-giant", 31},
        {" ", -1},
    };


    for (i = 0; race_list[i].size != -1; i++) {
        if (STRINGS_SAME(input, race_list[i].name))
            break;
    }

    return (MAX(0, race_list[i].size));
}


int
will_sell(OBJ_DATA * item, int shop_nr)
{
    int counter;

    if (item->obj_flags.cost < 1)
        return (FALSE);

    for (counter = 0; counter < MAX_TRADE; counter++)
        if (shop_index[shop_nr].sel_type[counter] == item->obj_flags.type)
            return (TRUE);
    return (FALSE);
}

#define NO_LEFTOVERS
int
will_buy(OBJ_DATA * item, int shop_nr)
{
    int counter;
    int rval = TRUE;
    int found;
    /*
     * char buf[256]; 
     */

    if (rval == TRUE)
        if (item->obj_flags.cost < 1)
            rval = FALSE;

    if (rval == TRUE) {
        found = FALSE;
        for (counter = 0; counter < MAX_TRADE; counter++)
            if (shop_index[shop_nr].buy_type[counter] == item->obj_flags.type)
                found = TRUE;

        if (!found)
            rval = FALSE;
#ifdef NO_LEFTOVERS
        /*
         * Code to keep merchants from buying half-eaten food 
         */
        if ((item->obj_flags.type == ITEM_FOOD)
            && (item->obj_flags.value[0] < obj_default[item->nr].value[0])) {
            /*
             * unfortunately, will_buy() does not do anything but return
             * whether or not a merchant will buy a given item, you can't
             * return or send a message to the buyer. -Morg sprintf (buf,
             * "$N says to you:\n\r \"%s\"", "Get away! Why would I want
             * to buy some halfeaten food?"); 
             */
            rval = FALSE;
        };
        /*
         * Added so they will not buy poisonous food 
         */
        if ((item->obj_flags.type == ITEM_FOOD)
            && (item->obj_flags.value[3] != 0)) {
            rval = FALSE;
        };
#endif
    };

    return (rval);
}

int
merchant_has_to_many(OBJ_DATA * trade, CHAR_DATA * merchant)
{
    int vnum;
    int count;
    OBJ_DATA *tmp;

    vnum = trade->nr;

    if (vnum == -1)
        return (FALSE);

    count = 0;
    for (tmp = merchant->carrying; tmp; tmp = tmp->next_content) {
        if (tmp->nr == vnum)
            count++;
    };

    if (count > 4)
        return (TRUE);
    else
        return (FALSE);
};


int
raw_shop_producing(OBJ_DATA *item, const struct shop_data *shopdata ) 
{
    int counter;

    if (item->nr < 0)
        return (FALSE);

    for (counter = 0; counter < MAX_PROD; counter++)
        if (shopdata->producing[counter] == item->nr)
            return TRUE;
    return FALSE;
}

int
shop_producing(OBJ_DATA * item, int shop_nr)
{
    return raw_shop_producing(item, &shop_index[shop_nr]);
}


float
city_commod_mult(OBJ_DATA * obj, int ct)
{
    float mult = 1.0;

    switch (obj->obj_flags.type) {
    case ITEM_WEAPON:
        mult *= city[ct].price[COMMOD_WEAPON];
        break;
    case ITEM_ART:
        mult *= city[ct].price[COMMOD_ART];
        break;
    case ITEM_TREASURE:
        mult *= city[ct].price[COMMOD_GEM];
        break;
    case ITEM_ARMOR:
        mult *= city[ct].price[COMMOD_ARMOR];
        break;
    case ITEM_WORN:
        mult *= city[ct].price[COMMOD_FUR];
        break;
    case ITEM_SPICE:
        mult *= city[ct].price[COMMOD_SPICE];
        break;
    }
    switch (obj->obj_flags.material) {
    case MATERIAL_SILK:
        mult *= city[ct].price[COMMOD_SILK];
        break;
    case MATERIAL_IVORY:
        mult *= city[ct].price[COMMOD_IVORY];
        break;
    case MATERIAL_TORTOISESHELL:
        mult *= city[ct].price[COMMOD_TORTOISESHELL];
        break;
    case MATERIAL_DUSKHORN:
        mult *= city[ct].price[COMMOD_DUSKHORN];
        break;
    case MATERIAL_WOOD:
        mult *= city[ct].price[COMMOD_WOOD];
        break;
    case MATERIAL_METAL:
        mult *= city[ct].price[COMMOD_METAL];
        break;
    case MATERIAL_OBSIDIAN:
        mult *= city[ct].price[COMMOD_OBSIDIAN];
        break;
    case MATERIAL_CHITIN:
        mult *= city[ct].price[COMMOD_CHITIN];
        break;
    case MATERIAL_BONE:
        mult *= city[ct].price[COMMOD_BONE];
        break;
    case MATERIAL_ISILT:
        mult *= city[ct].price[COMMOD_ISILT];
        break;
    case MATERIAL_SALT:
        mult *= city[ct].price[COMMOD_SALT];
        break;
    case MATERIAL_GWOSHI:
        mult *= city[ct].price[COMMOD_GWOSHI];
        break;
    case MATERIAL_PAPER:
        mult *= city[ct].price[COMMOD_PAPER];
        break;
    case MATERIAL_FOOD:
        mult *= city[ct].price[COMMOD_FOOD];
        break;
    case MATERIAL_FEATHER:
        mult *= city[ct].price[COMMOD_FEATHER];
        break;

    }

    if (IS_SET(obj->obj_flags.extra_flags, OFL_MAGIC) || (obj->obj_flags.type == ITEM_STAFF)
        || (obj->obj_flags.type == ITEM_WAND)
        || (obj->obj_flags.type == ITEM_POTION)
        || (obj->obj_flags.type == ITEM_COMPONENT))
        mult *= city[ct].price[COMMOD_MAGICK];

    return mult;
}

/*
 * determines percent off object->cost the player can sell the object to
 * the merchant for 
 */
float
prof_sell_percent(OBJ_DATA * item, int shop_nr)
{
    int i, ct;
    float mult = 1.0;

    if (item->nr < 0)
        return 0.0;

    if (!item->carried_by)
        return 1.0;

    ct = room_in_city(item->carried_by->in_room);

    if (ct < MAX_CITIES && ct > CITY_NONE)
        mult = city_commod_mult(item, ct);

#define BUY_WHAT_PRODUCEING_AT_DISCOUNT TRUE
#ifdef BUY_WHAT_PRODUCEING_AT_DISCOUNT
    if (shop_producing(item, shop_nr))
        mult = mult * .5;
#endif

    for (i = 0; i < MAX_TRADE; i++)
        if (shop_index[shop_nr].buy_type[i] == item->obj_flags.type)
            return (mult * shop_index[shop_nr].prof_sel[i]);

    return 0.0;
}


float
raw_prof_buy_percent(OBJ_DATA *item, const struct shop_data *shopdata )
{
    int i, ct;
    float mult = 1.0;

    if (item->nr < 0)
        return 0.0;

    if (!item->carried_by)
        return 1.0;

    ct = room_in_city(item->carried_by->in_room);

    if (ct < MAX_CITIES && ct > CITY_NONE)
        mult = city_commod_mult(item, ct);

    for (i = 0; i < MAX_TRADE; i++)
        if (shopdata->sel_type[i] == item->obj_flags.type)
            return (mult * shopdata->prof_buy[i]);

    return 0.0;
}

/*
 * determines the percent of object->cost the player can buy the object
 * for from the merchant 
 */
float
prof_buy_percent(OBJ_DATA * item, int shop_nr)
{
   return raw_prof_buy_percent(item, &shop_index[shop_nr]);
}

double floor(double);
/*
 * get the cost of the item for ch to buy the item from merchant 
 */
int
item_buy_cost(CHAR_DATA * ch, CHAR_DATA * merchant, OBJ_DATA * obj, int shop_nr)
{
    float cost;
    float pb = prof_buy_percent(obj, shop_nr);
    float ps = prof_sell_percent(obj, shop_nr);

    if (IS_IN_SAME_TRIBE(ch, merchant)) {
        if (IS_TRIBE(ch, TRIBE_KOHMAR))
            pb = ((pb + ps) * .75);
        else
            pb = ((pb + ps) / 2.0);
    }

    cost = (obj->obj_flags.cost * pb);


    /*
     * The value of armor is now a proportional to how repaired it is.  I
     * can never be worth less than 1/FULL_VALUE, however, to keep it from 
     * being entirely worthless and to block division by zero. -Nessalin
     * 9/7/2002 
     */
    if (obj->obj_flags.type == ITEM_ARMOR) {
        char errbuf[MAX_STRING_LENGTH];

        if (obj->obj_flags.value[1] < 1) {      // Bogus "points" value
            // for armor
            snprintf(errbuf, sizeof(errbuf), "%s being sold by %s has 0 armor points",
                     OSTR(obj, short_descr), MSTR(merchant, short_descr));

            gamelog(errbuf);
        }

        if (obj->obj_flags.value[1] < 1)
            cost = cost;
        else if (obj->obj_flags.value[0] < 1)
            // use val3 which was the 'pristine' armor value
            cost = (cost * (100 / obj->obj_flags.value[3])) / 100;
        else
            // use val3 which was the 'pristine' armor value
            cost = cost * ((obj->obj_flags.value[0] * 100) / obj->obj_flags.value[3]) / 100;
    }

    if (IS_SET(obj->obj_flags.state_flags, OST_SWEATY))
        cost *= .95;
    if (IS_SET(obj->obj_flags.state_flags, OST_DUSTY))
        cost *= .95;
    if (IS_SET(obj->obj_flags.state_flags, OST_REPAIRED))
        cost *= .90;
    if (IS_SET(obj->obj_flags.state_flags, OST_STAINED))
        cost *= .90;
    if (IS_SET(obj->obj_flags.state_flags, OST_SWEATY))
        cost *= .90;
    if (IS_SET(obj->obj_flags.state_flags, OST_BLOODY))
        cost *= .80;
    if (IS_SET(obj->obj_flags.state_flags, OST_SEWER))
        cost *= .80;
    if (IS_SET(obj->obj_flags.state_flags, OST_TORN))
        cost *= .70;
    if (IS_SET(obj->obj_flags.state_flags, OST_TATTERED))
        cost *= .60;
    if (IS_SET(obj->obj_flags.state_flags, OST_BURNED))
        cost *= .60;

    // if (IS_SET(obj->obj_flags.state_flags, OST_GITH))
    // if (IS_SET(obj->obj_flags.state_flags, OST_OLD))

    return (int) floor(cost + 0.5);
}

/*
 * get the cost of the item for ch to sell the item to the merchant 
 */
int
item_sell_cost(CHAR_DATA * ch, CHAR_DATA * merchant, OBJ_DATA * obj, int shop_nr)
{

    float cost;

    cost = (obj->obj_flags.cost * prof_sell_percent(obj, shop_nr));

    if (IS_SET(obj->obj_flags.state_flags, OST_SWEATY))
        cost *= .95;
    if (IS_SET(obj->obj_flags.state_flags, OST_DUSTY))
        cost *= .95;
    if (IS_SET(obj->obj_flags.state_flags, OST_REPAIRED))
        cost *= .90;
    if (IS_SET(obj->obj_flags.state_flags, OST_STAINED))
        cost *= .90;
    if (IS_SET(obj->obj_flags.state_flags, OST_SWEATY))
        cost *= .90;
    if (IS_SET(obj->obj_flags.state_flags, OST_BLOODY))
        cost *= .80;
    if (IS_SET(obj->obj_flags.state_flags, OST_SEWER))
        cost *= .80;
    if (IS_SET(obj->obj_flags.state_flags, OST_TORN))
        cost *= .70;
    if (IS_SET(obj->obj_flags.state_flags, OST_TATTERED))
        cost *= .60;
    if (IS_SET(obj->obj_flags.state_flags, OST_BURNED))
        cost *= .60;

    /*
     * if (item_buy_cost(ch, merchant, obj, shop_nr) >= cost) { cost =
     * item_buy_cost(ch, merchant, obj, shop_nr ) - 1; } 
     */

    return (int) floor(cost + 0.5);
}

bool
is_merchant(CHAR_DATA * ch)
{
    int i;

    for (i = 0; i < max_shops; i++)
        if (shop_index[i].merchant == ch->nr)
            return TRUE;
    return FALSE;
}


int
get_shop_number(CHAR_DATA * merchant)
{
    int shop_nr;

    if( !merchant ) return max_shops;

    for (shop_nr = 0; shop_nr < max_shops; shop_nr++)
        if (shop_index[shop_nr].merchant == merchant->nr)
            break;

    return shop_nr;
}


const struct shop_data *
get_shop_data(CHAR_DATA *merchant)
{
    int shop_nr;

    for (shop_nr = 0; shop_nr < max_shops; shop_nr++)
        if (shop_index[shop_nr].merchant == merchant->nr)
            return &shop_index[shop_nr];

    return 0;
}


typedef struct __inv_item
{
    OBJ_DATA *obj;
    char inv_desc[256];
    char keywords[256];
    int cost;
    int available_count;
}
inventory_item;


OBJ_DATA *
get_obj_in_inv_list(inventory_item * list, int list_size, const char *name)
{
    OBJ_DATA *obj;
    int i, j, number;
    char tmpname[MAX_INPUT_LENGTH];
    const char *tmp;

    strcpy(tmpname, name);
    tmp = tmpname;
    if (!(number = get_number(&tmp)))
        return NULL;

    for (i = 0, j = 1; i < list_size; i++) {
        obj = list[i].obj;
        if ((isname(tmp, OSTR(obj, name))) || (isname(tmp, real_sdesc_keywords_obj(obj, NULL)))) {
            if (j == number)
                return obj;
            j++;
        }
    }
    return (0);
}


int
cmp_inv_item(const void *inv1, const void *inv2)
{
    const inventory_item *item1 = (const inventory_item *) inv1;
    const inventory_item *item2 = (const inventory_item *) inv2;
    int i, cmp;
    const char *desc1, *desc2, *tmp1, *tmp2;


    // Skip over these types of words, for sorting purposes
    const char *skiplist[] = { "a ", "an " };   // , "a pair of ", "a length of ", "a piece of " };

// "a blackened ", "a bloodied ", "a burned ", "a dusty ", "a embroidered ", "a fringed ", "a glowing ", "a humming ", "a lace-edged ", "a mud-caked ", "an ashen ", "an empty ", "an ethereal ", "an invisible ", "an unlit ", "a old ", "a patched ", "a shimmering ", "a smelly ", "a stained ", "a sweat-stained ", "a tattered ", "a torn "

    if (!item1->obj && item2->obj)
        return -1;
    if (!item1->obj && !item2->obj)
        return -1;              // never say "equal" for these
    if (item1->obj && !item2->obj)
        return 1;

    desc1 = OSTR(item1->obj, short_descr);
    desc2 = OSTR(item2->obj, short_descr);
    tmp1 = desc1;
    tmp2 = desc2;

    for (i = 0; i < sizeof(skiplist) / sizeof(skiplist[0]); i++) {
        size_t len = strlen(skiplist[i]);
        if (strncmp(skiplist[i], desc1, len) == 0 &&    // Matches
            len > (tmp1 - desc1))       // But take only the biggest match
            tmp1 = desc1 + len;
        if (strncmp(skiplist[i], desc2, len) == 0 &&    // Matches
            len > (tmp2 - desc2))       // But take only the biggest match
            tmp2 = desc2 + len;
    }

    if ((cmp = strcmp(tmp1, tmp2)) != 0)
        return cmp;

    if (item1->cost != item2->cost)
        return (item1->cost < item2->cost);

    if (get_obj_size(item1->obj) != get_obj_size(item2->obj))
        return (get_obj_size(item1->obj) < get_obj_size(item2->obj));

    return 0;                   // Must be equal
}


// Shallow copy
void
copy_inventory_item(inventory_item * dst, inventory_item * src)
{
    memcpy(dst, src, sizeof(inventory_item));
}


int
sort_unique_inventory(CHAR_DATA * merchant, CHAR_DATA * customer, inventory_item * inv,
                      int inv_size)
{
    OBJ_DATA *obj;
    int shop_nr, cost, number;
    inventory_item this_item;

    // Figure out our shop-number, and 
    if ((shop_nr = get_shop_number(merchant)) == max_shops)
        return 0;

    for (obj = merchant->carrying, number = 0; obj; obj = obj->next_content) {
        if (CAN_SEE_OBJ(customer, obj) && will_sell(obj, shop_nr)
            && (cost = item_buy_cost(customer, merchant, obj, shop_nr)) > 0) {
            int i, found = 0;
            char buf[512];
            snprintf(buf, sizeof(buf), "%s%s%s", OBJS(obj, customer),
                     obj->obj_flags.type == ITEM_DRINKCON && obj->obj_flags.value[1]
                     ? " of " : "", obj->obj_flags.type == ITEM_DRINKCON && obj->obj_flags.value[1]
                     ? drinks[obj->obj_flags.value[2]] : "");

            this_item.obj = obj;
            strncpy(this_item.inv_desc, buf, sizeof(this_item.inv_desc));
            strncpy(this_item.keywords, real_sdesc_keywords_obj(obj, customer), sizeof(this_item.keywords));
            strncat(this_item.keywords, " ", sizeof(this_item.keywords));
            strncat(this_item.keywords, OSTR(obj, name), sizeof(this_item.keywords));
            this_item.cost = cost;
            this_item.available_count = 1;

            /* Fix b/c of null descriptions -Tenebrius 2004/06/05 */

            // Let's make this bad boy O(n^2).  Efficiency is for the weak
            for (i = 0, found = 0; i < number; i++) {
               if (cmp_inv_item(&inv[i], &this_item) == 0 && // Comparison function
                 strcmp((inv[i].inv_desc) ? (inv[i].inv_desc) : "", (this_item.inv_desc) ? (this_item.inv_desc) : "") == 0 && // inventory desc
                 strcmp(OSTR(inv[i].obj, description) ? OSTR(inv[i].obj, description) : "", OSTR(this_item.obj, description) ? OSTR(this_item.obj, description) : "") == 0) {        // full desc
                    found = 1;
                    inv[i].available_count++;
                    break;
                }
            }

            // This is a new item
            if (found == 0) {
                if (inv && number < inv_size)
                    copy_inventory_item(&inv[number++], &this_item);
                else
                    gamelogf("a merchant tried to build an inventory list larger than %d!",
                             inv_size);
            }
        }
    }

    if (inv)
        qsort(inv, number, sizeof(inventory_item), cmp_inv_item);

    return number;
}


CHAR_DATA *
get_first_vis_merchant(CHAR_DATA *ch)
{
    CHAR_DATA *merchant = 0;

    for (merchant = ch->in_room->people; merchant; merchant = merchant->next_in_room) {
        if (char_can_see_char(ch, merchant) && is_merchant(merchant))
            break;
    }

    return merchant;
}

#define MAX_INVENTORY 512

void
cmd_list(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *merchant;
    char buf[256], arg1[256], match[256], list[MAX_STRING_LENGTH], units[32], buf2[256], avail[256];
    char open_hour_str[30], close_hour_str[30];
    int i, shop_nr = 0, total_matches = 0;
    inventory_item inventory[MAX_INVENTORY];
    int uniq_count;

    *list = '\0';

    argument = one_argument(argument, arg1, sizeof(arg1));
    argument = one_argument(argument, match, sizeof(match));
    if (*arg1) {
        if (!(merchant = get_char_room_vis(ch, arg1))) {
            send_to_char("You do not see that merchant here.\n\r", ch);
            return;
        }
    } else {
        merchant = get_first_vis_merchant(ch);
    }

    if (!merchant) {
        send_to_char("You see no trader here with whom to barter.\n\r", ch);
        return;
    }

    if ((shop_nr = get_shop_number(merchant)) == max_shops) {
        act("$N has nothing to trade.", FALSE, ch, 0, merchant, TO_CHAR);
        return;
    }

    if (IS_IMMORTAL(ch)) {
      switch (shop_index[shop_nr].open_hour) {
      case NIGHT_EARLY: sprintf(open_hour_str, "before dawn"); break;
      case DAWN: sprintf(open_hour_str, "dawn"); break;
      case RISING_SUN: sprintf(open_hour_str, "early morning"); break;
      case MORNING: sprintf(open_hour_str, "late morning"); break;
      case HIGH_SUN: sprintf(open_hour_str, "high sun"); break;
      case AFTERNOON: sprintf(open_hour_str, "early afternoon"); break;
      case SETTING_SUN: sprintf(open_hour_str, "late afternoon"); break;
      case DUSK: sprintf(open_hour_str, "dusk"); break;
      case NIGHT_LATE: sprintf(open_hour_str, "late at night"); break;
      default: sprintf(open_hour_str, "invalid hour"); break;
      }
      switch (shop_index[shop_nr].close_hour) {
      case NIGHT_EARLY: sprintf(close_hour_str, "before dawn"); break;
      case DAWN: sprintf(close_hour_str, "dawn"); break;
      case RISING_SUN: sprintf(close_hour_str, "early morning"); break;
      case MORNING: sprintf(close_hour_str, "late morning"); break;
      case HIGH_SUN: sprintf(close_hour_str, "high sun"); break;
      case AFTERNOON: sprintf(close_hour_str, "early afternoon"); break;
      case SETTING_SUN: sprintf(close_hour_str, "late afternoon"); break;
      case DUSK: sprintf(close_hour_str, "dusk"); break;
      case NIGHT_LATE: sprintf(close_hour_str, "late at night"); break;
      default: sprintf(close_hour_str, "invalid hour"); break;
      }
      sprintf(buf2, "Shop hours are from %s to %s.\n\r", open_hour_str, close_hour_str);
      send_to_char(buf2, ch);
    }
    else
      if (!shop_open(shop_nr, merchant))
        return;

    /*
     * Check if merchant can see the character - Kelvik 
     */
    if (!CAN_SEE(merchant, ch) && !IS_IMMORTAL(ch)) {
        cmd_say(merchant, "I'll not deal with those I cannot see.", 0, 0);
        return;
    }

    if (!IS_IMMORTAL(ch))
	if ((shop_index[shop_nr].clan_only > 0)
	    && (!(in_same_tribe(merchant, ch)))) {
	  cmd_say(merchant, "I only deal with my own House.", 0, 0);
	  return;
	}

    snprintf(units, sizeof(units), "obsidian coins");

    snprintf(list, sizeof(list), "%s has the following goods to trade:\n\r", PERS(ch, merchant));

    uniq_count =
        sort_unique_inventory(merchant, ch, inventory, sizeof(inventory) / sizeof(inventory_item));

    for (i = 0; i < uniq_count; i++) {
        if (*match) {
            if (!isname(match, inventory[i].keywords))
                continue;
            else
                total_matches++;
        }

        if( inventory[i].available_count > 1 ) {
           snprintf( avail, sizeof(avail), ", %s are available", 
            inventory[i].available_count == 2 ? "a couple" : 
            (inventory[i].available_count < 6 ? "a few" : "many"));
        } else if( shop_producing(inventory[i].obj, shop_nr) ) {
           // infinite load
           snprintf( avail, sizeof(avail), ", many are available");
        } else {
           avail[0] = '\0';
        }

        snprintf(buf, sizeof(buf), "%2.2d) %s for %d %s%s.\n\r",
         i + 1, inventory[i].inv_desc, inventory[i].cost, units, avail);
        strncat(list, buf, sizeof(list));
    }

    if (*match) {
        snprintf(buf, sizeof(buf), "Your keyword matched %d items.\n\r", total_matches);
        strncat(list, buf, sizeof(list));
    }
    strncat(list, "\n\r", sizeof(list));

    if (ch->desc) {
        if (uniq_count > 0)
            page_string(ch->desc, list, 1);
        else
            act("$N has nothing to trade.", FALSE, ch, 0, merchant, TO_CHAR);
    }
}


void
unboot_shops(void)
{

    /*
     * int shop_nr, i, j, l, m; double k; FILE *fp; 
     */
    free(shop_index);
    /*
     * 
     * for (shop_nr = 0; shop_nr < max_shops; shop_nr++) {
     * free(shop_index[shop_nr].c_no_item);
     * free(shop_index[shop_nr].m_no_item);
     * free(shop_index[shop_nr].c_no_cash);
     * free(shop_index[shop_nr].m_no_cash);
     * free(shop_index[shop_nr].m_not_buy);
     * free(shop_index[shop_nr].m_not_sell);
     * free(shop_index[shop_nr].m_no_barter);
     * free(shop_index[shop_nr].m_accept);
     * 
     * } 
     */
}


/*
 * Modified 10/24/2003 by Nessalin to read from individual shop files and 
 * for ne values 
 */
void
boot_shops(void)
{
    DIR *shopdir;
    struct dirent *shopentry;
    int shop_nr, i, j, l, m, counter;
    double k;

    char buf[256];
    char fstr[512];
    char *tmp;
    FILE *fp;

    if (shop_index) {
        free(shop_index);
        shop_index = 0;
        shop_nr = 0;
    }

    shop_nr = 0;

    shopdir = opendir("data_files/shop_files");
    if (!shopdir) {
        gamelogf("Couldn't open data_files/shop_files, NO SHOPS FOR YOU!");
        return;
    }

    while ((shopentry = readdir(shopdir)) != 0) {
        counter = strtol(shopentry->d_name, NULL, 10);
        if (errno == ERANGE || errno == EINVAL)
            continue;

        snprintf(fstr, sizeof(fstr), "data_files/shop_files/%d", counter);

        if ((fp = fopen(fstr, "r"))) {
            if (!shop_nr)
                CREATE(shop_index, struct shop_data, 1);
            else {
                RECREATE(shop_index, struct shop_data, shop_nr + 1);
                memset(&shop_index[shop_nr], 0, sizeof(struct shop_data));
            }

            for (i = 0; i < MAX_PROD; i++) {
                fscanf(fp, " %d ", &j);
                shop_index[shop_nr].producing[i] = j;
            }
            fscanf(fp, " \n ");

            for (i = 0; i < MAX_NEW; i++) {
                fscanf(fp, " %d ", &j);
                shop_index[shop_nr].new_item[i] = j;
            }
            fscanf(fp, " \n ");

            for (i = 0; i < 24; i++) {
                fscanf(fp, " %d ", &j);
                shop_index[shop_nr].buy_mats[i] = j;
            }
            fscanf(fp, " \n ");

            for (i = 0; i < MAX_TRADE; i++) {
                fscanf(fp, " %lf ", &k);
                shop_index[shop_nr].prof_buy[i] = k;
            }
            fscanf(fp, " \n ");

            for (i = 0; i < MAX_TRADE; i++) {
                fscanf(fp, " %lf ", &k);
                shop_index[shop_nr].prof_sel[i] = k;
            }
            fscanf(fp, " \n ");

            for (i = 0; i < MAX_TRADE; i++) {
                fscanf(fp, " %d ", &j);
                shop_index[shop_nr].buy_type[i] = j;
            }
            fscanf(fp, " \n ");

            for (i = 0; i < MAX_TRADE; i++) {
                fscanf(fp, " %d ", &j);
                shop_index[shop_nr].sel_type[i] = j;
            }
            fscanf(fp, " \n ");

            tmp = fread_string(fp);
            strcpy(shop_index[shop_nr].c_no_item, tmp);
            free(tmp);
            tmp = fread_string(fp);
            strcpy(shop_index[shop_nr].m_no_item, tmp);
            free(tmp);
            tmp = fread_string(fp);
            strcpy(shop_index[shop_nr].c_no_cash, tmp);
            free(tmp);
            tmp = fread_string(fp);
            strcpy(shop_index[shop_nr].m_no_cash, tmp);
            free(tmp);
            tmp = fread_string(fp);
            strcpy(shop_index[shop_nr].m_not_buy, tmp);
            free(tmp);
            tmp = fread_string(fp);
            strcpy(shop_index[shop_nr].m_not_sell, tmp);
            free(tmp);
            tmp = fread_string(fp);
            strcpy(shop_index[shop_nr].m_no_barter, tmp);
            free(tmp);
            tmp = fread_string(fp);
            strcpy(shop_index[shop_nr].m_accept, tmp);
            free(tmp);
            tmp = fread_string(fp);
            strcpy(shop_index[shop_nr].m_no_mat, tmp);
            free(tmp);
            tmp = fread_string(fp);
            strcpy(shop_index[shop_nr].m_close, tmp);
            free(tmp);
            tmp = fread_string(fp);
            strcpy(shop_index[shop_nr].m_open, tmp);
            free(tmp);

            fscanf(fp, " %d %d ", &j, &l);
            fscanf(fp, " \n ");

            shop_index[shop_nr].open_hour = j;
            shop_index[shop_nr].close_hour = l;

            fscanf(fp, " %d %d ", &l, &m);
            fscanf(fp, " \n ");

            shop_index[shop_nr].clan_only = l;
            shop_index[shop_nr].barter = m;

            for (i = 0; i < MAX_PROD; i++)
                if (shop_index[shop_nr].producing[i] >= 0)
                    shop_index[shop_nr].producing[i] =
                        real_object(shop_index[shop_nr].producing[i]);

            /* These are for the random items */

            // Commented out by Nessalin 1/17/2004
            // Changing to use virtual nums instead of real numbers
            //      for (i = 0; i < MAX_NEW; i++)
            //        if (shop_index[shop_nr].new_item[i] >= 0)
            //          shop_index[shop_nr].new_item[i] = 
            //            real_object (shop_index[shop_nr].new_item[i]);

            shop_index[shop_nr].merchant = real_mobile(counter);

            shop_index[shop_nr].no_barter = (void *) 0;

            fclose(fp);
            shop_nr = shop_nr + 1;
        }
    }
    closedir(shopdir);

    max_shops = shop_nr;
    sprintf(buf, "TOTAL_SHOPS: %d", max_shops);
    gamelog(buf);
    update_shops();
}


void
boot_shops_old(void)
{
    int shop_nr, i, j, l, m;
    double k;
    char *tmp;
    FILE *fp;

    if (!(fp = fopen(SHOP_FILE, "r"))) {
        perror("Error in boot_shops");
        exit(0);
    }
    if (shop_index) {
        free(shop_index);
        shop_index = 0;
        shop_nr = 0;
    };

    fscanf(fp, " %d \n", &j);
    max_shops = j;


    for (shop_nr = 0; shop_nr < max_shops; shop_nr++) {
        if (!shop_nr)
            CREATE(shop_index, struct shop_data, 1);
        else {
            RECREATE(shop_index, struct shop_data, shop_nr + 1);
            memset(&shop_index[shop_nr], 0, sizeof(struct shop_data));
        }

        for (i = 0; i < MAX_PROD; i++) {
            fscanf(fp, " %d ", &j);
            shop_index[shop_nr].producing[i] = j;
        }
        fscanf(fp, " \n ");

        for (i = 0; i < MAX_NEW; i++) {
            fscanf(fp, " %d ", &j);
            shop_index[shop_nr].new_item[i] = j;
        }
        fscanf(fp, " \n ");

        for (i = 0; i < MAX_TRADE; i++) {
            fscanf(fp, " %lf ", &k);
            shop_index[shop_nr].prof_buy[i] = k;
        }
        fscanf(fp, " \n ");

        for (i = 0; i < MAX_TRADE; i++) {
            fscanf(fp, " %lf ", &k);
            shop_index[shop_nr].prof_sel[i] = k;
        }
        fscanf(fp, " \n ");

        for (i = 0; i < MAX_TRADE; i++) {
            fscanf(fp, " %d ", &j);
            shop_index[shop_nr].buy_type[i] = j;
        }
        fscanf(fp, " \n ");

        for (i = 0; i < MAX_TRADE; i++) {
            fscanf(fp, " %d ", &j);
            shop_index[shop_nr].sel_type[i] = j;
        }
        fscanf(fp, " \n ");

        tmp = fread_string(fp);
        strcpy(shop_index[shop_nr].c_no_item, tmp);
        free(tmp);
        tmp = fread_string(fp);
        strcpy(shop_index[shop_nr].m_no_item, tmp);
        free(tmp);
        tmp = fread_string(fp);
        strcpy(shop_index[shop_nr].c_no_cash, tmp);
        free(tmp);
        tmp = fread_string(fp);
        strcpy(shop_index[shop_nr].m_no_cash, tmp);
        free(tmp);
        tmp = fread_string(fp);
        strcpy(shop_index[shop_nr].m_not_buy, tmp);
        free(tmp);
        tmp = fread_string(fp);
        strcpy(shop_index[shop_nr].m_not_sell, tmp);
        free(tmp);
        tmp = fread_string(fp);
        strcpy(shop_index[shop_nr].m_no_barter, tmp);
        free(tmp);
        tmp = fread_string(fp);
        strcpy(shop_index[shop_nr].m_accept, tmp);
        free(tmp);

        // fscanf(fp, " %d %d ", &j, &l);
        // fscanf (fp, " \n ");
        // shop_index[shop_nr].open_hour = j;
        // shop_index[shop_nr].close_hour = l;

        fscanf(fp, " %d %d %d ", &j, &l, &m);
        fscanf(fp, " \n ");

        shop_index[shop_nr].merchant = j;
        shop_index[shop_nr].open_hour = l;
        shop_index[shop_nr].close_hour = m;


        // shop_index[shop_nr].with_who = l;
        // shop_index[shop_nr].barter = m;



        for (i = 0; i < MAX_PROD; i++)
            if (shop_index[shop_nr].producing[i] >= 0)
                shop_index[shop_nr].producing[i] = real_object(shop_index[shop_nr].producing[i]);
        for (i = 0; i < MAX_NEW; i++)
            if (shop_index[shop_nr].new_item[i] >= 0)
                shop_index[shop_nr].new_item[i] = real_object(shop_index[shop_nr].new_item[i]);
        shop_index[shop_nr].merchant = real_mobile(shop_index[shop_nr].merchant);
        shop_index[shop_nr].no_barter = (void *) 0;
    }

    fclose(fp);
}


void
update_shopkeepers(int nr)
{
    int i;

    for (i = 0; i < max_shops; i++)
        if (shop_index[i].merchant >= nr)
            shop_index[i].merchant++;
}

void
update_shopkeepers_d(int nr)
{
    int i;

    for (i = 0; i < max_shops; i++)
        if (shop_index[i].merchant >= nr)
            shop_index[i].merchant--;
}

void
update_producing(int nr)
{
    int i, j;

    for (i = 0; i < max_shops; i++)
        for (j = 0; j < MAX_PROD; j++)
            if (shop_index[i].producing[j] >= nr)
                shop_index[i].producing[j]++;
}

void
update_producing_d(int nr)
{
    int i, j;

    for (i = 0; i < max_shops; i++)
        for (j = 0; j < MAX_PROD; j++)
            if (shop_index[i].producing[j] >= nr)
                shop_index[i].producing[j]--;
}

bool
is_in_barter(CHAR_DATA * ch)
{
    struct barter_data *tmp;

    for (tmp = barter_list; tmp; tmp = tmp->next)
        if (tmp->customer == ch)
            return TRUE;
    return FALSE;
}

struct barter_data *
get_barter(CHAR_DATA * ch, CHAR_DATA * merch)
{
    struct barter_data *tmp;

    for (tmp = barter_list; tmp; tmp = tmp->next)
        if ((tmp->customer == ch) && (tmp->merchant == merch))
            break;
    return tmp;
}

void
add_to_barter(CHAR_DATA * ch, CHAR_DATA * merchant, OBJ_DATA * c_obj, OBJ_DATA * m_obj,
              int c_amount, int m_amount, int shop_nr, int irate)
{

    struct barter_data *new;

    CREATE(new, struct barter_data, 1);
    new->shop_nr = shop_nr;
    new->merchant = merchant;
    new->customer = ch;
    new->m_object = m_obj;
    new->c_object = c_obj;
    new->m_amount = m_amount;
    new->c_amount = c_amount;
    new->irritation = irate;

    new->next = barter_list;
    barter_list = new;
}

void
remove_from_barter(CHAR_DATA * ch, CHAR_DATA * merch)
{
    struct barter_data *remove, *before;

    for (remove = barter_list; remove; remove = remove->next)
        if ((remove->customer == ch) && (remove->merchant == merch))
            break;
    if (!remove)
        return;

    if (remove == barter_list) {
        barter_list = remove->next;
        free((char *) remove);
    } else {
        for (before = barter_list; before->next != remove; before = before->next);
        before->next = remove->next;
        free((char *) remove);
    }
}
void
remove_char_all_barter(CHAR_DATA * ch)
{
    struct barter_data *remove;
    int found;

    do {
        found = FALSE;
        for (remove = barter_list; remove && !found; remove = remove->next)
            if ((remove->customer == ch) || (remove->merchant == ch)) {
                found = TRUE;
                remove_from_barter(remove->customer, remove->merchant);
            };
    }
    while (found);
}

void
remove_obj_all_barter(OBJ_DATA * obj)
{
    struct barter_data *remove;
    int found;

    do {
        found = FALSE;
        for (remove = barter_list; remove && !found; remove = remove->next)
            if ((remove->c_object == obj) || (remove->m_object == obj)) {
                found = TRUE;
                remove_from_barter(remove->customer, remove->merchant);
            };
    }
    while (found);
}


void
update_shops()
{
    int shop_nr;
    CHAR_DATA *tmp;
    struct timeval start;
    // char buf[256];


    // sprintf(buf, "Adjusted hour: %d Real Hour: %d.",
    // adjusted_hour, time_info.hours);
    // gamelog (buf);

    perf_enter(&start, "update_shops()");
    for (tmp = character_list; tmp != 0; tmp = tmp->next) {
        for (shop_nr = 0; shop_nr < max_shops; shop_nr++) {
            if (shop_index[shop_nr].merchant == tmp->nr) {
                update_shop(tmp, &shop_index[shop_nr]);
            }
        }
    }
    perf_exit("update_shops()", start);
}



void
update_shop(CHAR_DATA *shopkeeper, const struct shop_data *shopdata)
{
    /* time used to change after this func, now it changes before */
    int adjusted_hour = time_info.hours;
    // gamelog ("update_shops() running.");
    if (adjusted_hour == 9)
        adjusted_hour = 0;


    if (shopdata->open_hour != shopdata->close_hour) {

        if (shopdata->open_hour == adjusted_hour) {
            parse_command(shopkeeper, shopdata->m_open);
            add_random_items(shopkeeper, shopdata);
        }

        if (shopdata->close_hour == adjusted_hour) {
            parse_command(shopkeeper, shopdata->m_close);
            remove_random_items(shopkeeper, shopdata);
        }
    } else {        /* open hour equals close hour */
        if (adjusted_hour == 8) {   /* midnight */
            remove_random_items(shopkeeper, shopdata);
            add_random_items(shopkeeper, shopdata);
        }
    }

    int open_hour = shopdata->open_hour;
    int close_hour = shopdata->close_hour;

    int shop_always_open = open_hour == close_hour;
    int between_open_and_close_for_early_open = (open_hour < close_hour &&
						 open_hour < adjusted_hour &&
						 close_hour > adjusted_hour);

    int morning_but_before_close = open_hour > adjusted_hour && close_hour > adjusted_hour;
    int evening_but_after_open = close_hour < adjusted_hour && open_hour < adjusted_hour;
    int between_open_and_close_for_late_open = (open_hour > close_hour && 
						(morning_but_before_close || evening_but_after_open));
       
    // if we're within selling hours
    if( shop_always_open
	|| between_open_and_close_for_early_open
	|| between_open_and_close_for_late_open) {
        // every hour have a small chance to virtually sell any item
        remove_items_for_economy(shopkeeper, shopdata);
    }
}


void
add_random_items(CHAR_DATA *shopkeeper, const struct shop_data *shopdata)
{
    if (!shopdata) return;

    OBJ_DATA *tmp_obj;
    int i;
    for (i = 0; i < MAX_NEW; i++) {
        if ((number(1, 10) == 1) && shopdata->new_item[i] > 0) {
            tmp_obj = read_object(shopdata->new_item[i], VIRTUAL);

            if (!tmp_obj) {
                shhlogf("Attempted to load %d onto %s as random item, but item does not exist.",
                        shopdata->new_item[i], MSTR(shopkeeper, short_descr));
                continue;
            }

            if (tmp_obj->obj_flags.type == ITEM_WAGON) {
                gamelogf("Tried to randomly load a wagon (item #%d) onto a shopkeeper (but didn't):\n\r\t%s, %s",
                         shopdata->new_item[i], MSTR(shopkeeper, short_descr),
                         OSTR(tmp_obj, short_descr));
                extract_obj(tmp_obj);
                tmp_obj = 0;
            } else if (tmp_obj) {
                char thinkbuf[512];
                snprintf(thinkbuf, sizeof(thinkbuf), "Adding %s to inventory...", OSTR(tmp_obj, short_descr));
                cmd_think(shopkeeper, thinkbuf, 0, 0);
                obj_to_char(tmp_obj, shopkeeper);
            }
        }
    }
}


void
remove_random_items(CHAR_DATA *shopkeeper, const struct shop_data *shopdata)
{
    if (!shopdata) return;

    int i;
    OBJ_DATA *tmp_obj, *tmp_obj2;

    for (tmp_obj = shopkeeper->carrying; tmp_obj; tmp_obj = tmp_obj2) {
        tmp_obj2 = tmp_obj->next_content;
        for (i = 0; tmp_obj && i < MAX_NEW; i++) {
            if (shopdata->new_item[i] > 0 && (obj_index[tmp_obj->nr].vnum == shopdata->new_item[i])) {
                char thinkbuf[512];
                snprintf(thinkbuf, sizeof(thinkbuf), "Removing %s from inventory...", OSTR(tmp_obj, short_descr));
                cmd_think(shopkeeper, thinkbuf, 0, 0);
                extract_obj(tmp_obj);
                tmp_obj = 0;
            }
        }
    }
}

void
remove_items_for_economy(CHAR_DATA *shopkeeper, const struct shop_data *shopdata)
{
    if (!shopdata) return;
    if( shopkeeper->in_room && IS_SET(shopkeeper->in_room->room_flags, RFL_IMMORTAL_ROOM)) return;
    if( find_ex_description("[NO_VIRTUAL_SALES]", shopkeeper->ex_description, TRUE) != NULL) return;

    OBJ_DATA *tmp_obj, *tmp_obj2;
    int items_sold = 0;

    for (tmp_obj = shopkeeper->carrying; tmp_obj; tmp_obj = tmp_obj2) {
        tmp_obj2 = tmp_obj->next_content;
        int cost = tmp_obj->obj_flags.cost 
          * raw_prof_buy_percent(tmp_obj, shopdata);
        // one in 10 chance, ideally this would be based on supply/demand
        if( !number(0, MAX(cost * 10, 999))) {
            // give the merchant 3/4 cost
            GET_OBSIDIAN(shopkeeper) += cost * 3 / 4;            
            items_sold++;

            // never remove produced items
            if( !raw_shop_producing( tmp_obj, shopdata)) {
                shhlogf("%s (%d) sold %s (%d) for %d coins from inventory for economy...", 
                 MSTR(shopkeeper, short_descr), npc_index[shopkeeper->nr].vnum,
                 OSTR(tmp_obj, short_descr), obj_index[tmp_obj->nr].vnum, cost * 3 / 4);
                extract_obj(tmp_obj);
                tmp_obj = 0;
            } else {
                shhlogf("%s (%d) sold %s (%d) for %d coins from producing for economy...", 
                 MSTR(shopkeeper, short_descr), npc_index[shopkeeper->nr].vnum,
                 OSTR(tmp_obj, short_descr), obj_index[tmp_obj->nr].vnum, cost * 3 / 4);
            }
            // no more than one per game hour
            break;
        }
    }

    if( items_sold > 0 ) {
       parse_command(shopkeeper, "emote sells to a passerby.");
    }
}


void
cmd_buy(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int shop_nr = 0, which, size_obj = 0;
    char arg1[256], arg2[256], arg3[256], buf[256];
    char units[20];
    inventory_item inventory[MAX_INVENTORY];
    int uniq_count;

    OBJ_DATA *wanted;
    int w_val;

    CHAR_DATA *merchant;

    argument = one_argument(argument, arg1, sizeof(arg1));
    argument = one_argument(argument, arg2, sizeof(arg2));
    argument = one_argument(argument, arg3, sizeof(arg3));

    if (!*arg1) {
        act("What would you like to buy?", FALSE, ch, 0, 0, TO_CHAR);
	return;
    }

    if (*arg2) {
        if (!(merchant = get_char_room_vis(ch, arg2))) {
            act("You see no trader by that name with whom to barter.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }

        if (!is_merchant(merchant)) {
            act("You may only trade with a merchant.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }

        if (*arg3) {
            if (!(size_obj = get_race_average(arg3))) {
                sprintf(buf, "say I do not carry that size.");
                parse_command(merchant, buf);
                return;
            }
        }
    } else {
        merchant = get_first_vis_merchant(ch);
    }

    if (!merchant) {
        act("You see no trader here with whom to barter.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (!CAN_SEE(merchant, ch) && !IS_IMMORTAL(ch)) {
        cmd_say(merchant, "I'll not deal with those I cannot see.", 0, 0);
        return;
    }

    if ((shop_nr = get_shop_number(merchant)) == max_shops) {
        act("$N has nothing to trade.", FALSE, ch, 0, merchant, TO_CHAR);
        return;
    }

    if (!(shop_open(shop_nr, merchant)))
        return;

    if (!IS_IMMORTAL(ch))
      if ((shop_index[shop_nr].clan_only > 0)
	  && (!(in_same_tribe(merchant, ch)))) {
        cmd_say(merchant, "I only deal with my own House.", 0, 0);
        return;
      }

    if (!IS_NPC(merchant)) {
        act("I think you better talk to $N.", FALSE, ch, 0, merchant, TO_CHAR);
        return;
    }

    if (merchant->player.race == RACE_GALITH)
        sprintf(units, "hours of service");
    else
        sprintf(units, "obsidian coins");

    uniq_count =
        sort_unique_inventory(merchant, ch, inventory, sizeof(inventory) / sizeof(inventory_item));
    if (*arg1 == '#') {
        which = atoi(&arg1[1]);

        wanted = NULL;

        if (which > 0 && which < uniq_count + 1)
            wanted = inventory[which - 1].obj;

        if (!wanted) {
            act("$N is not carrying an item with that number.", FALSE, ch, 0, merchant, TO_CHAR);
            return;
        }
    } else if (!(wanted = get_obj_in_inv_list(inventory, uniq_count, arg1))) {
        act("$N is not carrying an item with that name.", FALSE, ch, 0, merchant, TO_CHAR);
        return;
    }

    w_val = item_buy_cost(ch, merchant, wanted, shop_nr);

    if (!will_sell(wanted, shop_nr)) {
        sprintf(buf, "$N says to you:\n\r     \"%s\"", shop_index[shop_nr].m_not_sell);
        act(buf, FALSE, ch, 0, merchant, TO_CHAR);
        return;
    }

    if (merchant->player.race == RACE_GALITH) {
        if ((ch->specials.eco < 100) && (w_val > ch->specials.eco)) {
            sprintf(buf, "$N says to you:\n\r     \"%s\"", shop_index[shop_nr].c_no_cash);
            act(buf, FALSE, ch, 0, merchant, TO_CHAR);
            remove_from_barter(ch, merchant);
            return;
        }
    } else if (GET_OBSIDIAN(ch) < w_val) {
        sprintf(buf, "$N says to you:\n\r     \"%s\"", shop_index[shop_nr].c_no_cash);
        act(buf, FALSE, ch, 0, merchant, TO_CHAR);
        return;
    }

    if (merchant->player.race == RACE_GALITH) {
        ch->specials.eco = (w_val > 100) ? 0 : ch->specials.eco - w_val;
        merchant->specials.eco =
            (merchant->specials.eco + w_val > 100) ? 100 : merchant->specials.eco + w_val;
        w_val = (w_val > 100) ? 100 : w_val;
    } else {
        GET_OBSIDIAN(ch) -= w_val;
        GET_OBSIDIAN(merchant) += w_val;
    }

    if (!size_obj)
        size_obj = get_race_average_number(GET_RACE(ch));

    if (shop_producing(wanted, shop_nr)) {
        int orig_size = -1;
 
        if (wanted->obj_flags.type == ITEM_ARMOR) {
            orig_size = wanted->obj_flags.value[2];
            wanted->obj_flags.value[2] = size_obj;
        }
        if (wanted->obj_flags.type == ITEM_WORN) {
            orig_size = wanted->obj_flags.value[0];
            wanted->obj_flags.value[0] = size_obj;
        }

        if( orig_size > 0 && orig_size != size_obj ) {
            // adjust weight for size change
            wanted->obj_flags.weight = wanted->obj_flags.weight * size_obj / orig_size;
        }
    }

    obj_from_char(wanted);
    obj_to_char(wanted, ch);

    sprintf(buf, "%s trades %s to %s\n\r", MSTR(merchant, name), OSTR(wanted, short_descr),
            MSTR(ch, name));
    send_to_monitor(ch, buf, MONITOR_MERCHANT);

    sprintf(buf, "You give $N %d %s for $p.", w_val, units);
    act(buf, FALSE, ch, wanted, merchant, TO_CHAR);
    sprintf(buf, "$N trades $p to $n.");
    act(buf, FALSE, ch, wanted, merchant, TO_NOTVICT);

    if (shop_producing(wanted, shop_nr))
        obj_to_char(read_object(wanted->nr, REAL), merchant);
}

void
cmd_offer(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    int shop_nr = 0, which, haggle = 0, accept_at;
    int diff_perc, hf = 0, current_off = 0, c_amount = 0, c_val = 0, w_val = 0, new_amount = 0;
    char arg1[256], arg2[256], arg3[256], buf[256], units[20];
    int lastIrritation = 0, uniq_count;

    inventory_item inventory[MAX_INVENTORY];
    OBJ_DATA *trade = NULL, *wanted = NULL;

    CHAR_DATA *merchant;

    struct barter_data *barter = (struct barter_data *) 0;

    argument = one_argument(argument, arg1, sizeof(arg1));
    argument = one_argument(argument, arg2, sizeof(arg2));
    argument = one_argument(argument, arg3, sizeof(arg3));

    // offer 30 helmet bulky   "bulky" is arg3
    if (*arg3) {
        if (!(merchant = get_char_room_vis(ch, arg3))) {
            act("You see no trader by that name with whom to barter.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
    } else if (*arg2) {         /* offer [item] [merchant] */
        if (!(merchant = get_char_room_vis(ch, arg2))) {
            merchant = get_first_vis_merchant(ch);
        } else {
            *arg2 = 0; // Don't treat this arg as an item later, since we found a merchant.
                       // Yay, ambigious syntax!   -- Xygax
        }
    } else {
        merchant = get_first_vis_merchant(ch);
    }

    /* No merchant found */
    if (!merchant) {
        act("You see no trader here with whom to barter.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    /* verify we have a merchant */
    if ((shop_nr = get_shop_number(merchant)) == max_shops) {
        act("$N has nothing to trade.", FALSE, ch, 0, merchant, TO_CHAR);
        return;
    }

    /*
     * Check added if merchant can see character - Kelvik 
     */
    if (!CAN_SEE(merchant, ch) && !IS_IMMORTAL(ch)) {
        cmd_ooc(merchant, "I'll not deal with those I cannot see.", 0, 0);
        return;
    }

    if (merchant->player.race == RACE_GALITH)
        sprintf(units, "hours of service");
    else
        sprintf(units, "obsidian coins");

    /*
     * Check to see if shop is open - Nessalin 10/11/2003 
     */
    if (!shop_open(shop_nr, merchant))
        return;

    if (!IS_IMMORTAL(ch))
      if ((shop_index[shop_nr].clan_only > 0)
	  && (!(in_same_tribe(merchant, ch)))) {
        cmd_say(merchant, "I only deal with my own House.", 0, 0);
        return;
      }
    // Offering coins
    if (isdigit(*arg1)) {
        // No "target" argument, so we check for an existing trade
        if (!*arg2) {
            if ((barter = get_barter(ch, merchant))) {
                if (barter->m_object != NULL) {
                    // This is an existing trade for an item
                    c_amount = atoi(arg1);
                    c_val = c_amount;
                    trade = (OBJ_DATA *) 0;
                    wanted = barter->m_object;
                } else if (barter->c_object != NULL) {
                    // Character is haggling for something they're offering?
                    wanted = (OBJ_DATA *) 0;
                    w_val = atoi(arg1);
                    trade = barter->c_object;
                    c_amount = 0;
                    c_val = item_sell_cost(ch, merchant, trade, shop_nr);
                }

            } else {
                // No existing trade
                act("Which item do you wish to offer on, or how much do you desire for yours?",
                    FALSE, ch, 0, 0, TO_CHAR);
                return;
            }
        } else {
            if (isdigit(*arg2)) {
                // Someone did "offer 32 74"?
                sprintf(buf, "You may only offer %s or an item.", units);
                act(buf, FALSE, ch, 0, 0, TO_CHAR);
                return;
            }

            c_amount = atoi(arg1);
            c_val = c_amount;
            trade = (OBJ_DATA *) 0;
        }
    } else {
        // Offering an item in-trade
        if (!(trade = get_obj_in_list_vis(ch, arg1, ch->carrying))) {
            sprintf(buf, "You may only offer %s or an item.", units);
            act(buf, FALSE, ch, 0, 0, TO_CHAR);
            return;
        } else if (!will_buy(trade, shop_nr)
                   && merchant->player.race != RACE_GALITH) {
            sprintf(buf, "$N says to you:\n\r     \"%s\"", shop_index[shop_nr].m_not_buy);
            act(buf, FALSE, ch, 0, merchant, TO_CHAR);
            return;
        }
#define USE_MERCHANT_HAS_TO_MANY TRUE
#ifdef USE_MERCHANT_HAS_TO_MANY
        else if (merchant_has_to_many(trade, merchant)) {
            sprintf(buf, "$N says to you:\n\r     \"%s\"",
                    "Sorry I have too many of those already");
            act(buf, FALSE, ch, 0, merchant, TO_CHAR);

            return;
        }
#endif

        c_amount = 0;
        c_val = item_sell_cost(ch, merchant, trade, shop_nr);
    }

    uniq_count =
        sort_unique_inventory(merchant, ch, inventory, sizeof(inventory) / sizeof(inventory_item));

    if (*arg2 == '#') {
        // Offering on an item by index #
        which = atoi(arg2 + 1);

        wanted = NULL;

        if (which > 0 && which < uniq_count + 1) {
            wanted = inventory[which - 1].obj;
        }

        if (!wanted) {
            act("$N is not carrying an item with that number.", FALSE, ch, 0, merchant, TO_CHAR);
            return;
        }

        if (trade)
            w_val = 0;
        else
            w_val = item_buy_cost(ch, merchant, wanted, shop_nr);
    } else if (!*arg2 && trade && w_val == 0) {
        wanted = (OBJ_DATA *) 0;
        w_val = item_sell_cost(ch, merchant, trade, shop_nr);
    } else if (*arg2 && isdigit(*arg2)) {
        wanted = (OBJ_DATA *) 0;
        w_val = atoi(arg2);
    } else if (*arg2 && !(wanted = get_obj_in_inv_list(inventory, uniq_count, arg2))) {
        act("$N is not carrying an item with that name.", FALSE, ch, 0, merchant, TO_CHAR);
        return;
    } else if (wanted && !will_sell(wanted, shop_nr)) {
        sprintf(buf, "$N says to you:\n\r     \"%s\"", shop_index[shop_nr].m_not_sell);
        act(buf, FALSE, ch, 0, merchant, TO_CHAR);
        return;
    } else if (w_val == 0) {
        if (!trade)
            w_val = item_buy_cost(ch, merchant, wanted, shop_nr);
    }

    while (!barter) {
        if (!(barter = get_barter(ch, merchant))) {
            if (trade && wanted)
                add_to_barter(ch, merchant, trade, wanted, 0, 0, shop_nr, 0);
            else if (!trade && wanted)
                add_to_barter(ch, merchant, 0, wanted, item_buy_cost(ch, merchant, wanted, shop_nr),
                              0, shop_nr, 0);
            else if (trade && !wanted)
                add_to_barter(ch, merchant, trade, 0, 0,
                              item_sell_cost(ch, merchant, trade, shop_nr), shop_nr, 0);
        } else if ((barter->merchant != merchant) || (barter->c_object != trade) ||     /* Added these two - Kel */
                   (barter->m_object != wanted)) {
            lastIrritation = barter->irritation;
            barter = (struct barter_data *) 0;
            remove_char_all_barter(ch);

            if (trade && wanted)
                add_to_barter(ch, merchant, trade, wanted, 0, 0, shop_nr, lastIrritation);
            else if (!trade && wanted)
                add_to_barter(ch, merchant, 0, wanted, item_buy_cost(ch, merchant, wanted, shop_nr),
                              0, shop_nr, lastIrritation);
            else if (trade && !wanted)
                add_to_barter(ch, merchant, trade, 0, 0,
                              item_sell_cost(ch, merchant, trade, shop_nr), shop_nr,
                              lastIrritation);
        }
    }

    if (has_skill(ch, SKILL_HAGGLE))
        /*
         * if (ch->skills[SKILL_HAGGLE]) 
         */
        haggle = get_skill_percentage(ch, SKILL_HAGGLE);
    /*
     * haggle = ch->skills[SKILL_HAGGLE]->learned; 
     */
    else if (GET_RACE(ch) == RACE_DESERT_ELF)
        haggle = wis_app[GET_WIS(ch)].percep + 10;

    haggle = MAX(haggle + GET_WIS(ch) - 20, 5);
    haggle = MIN(haggle, 60);

    if (!trade && wanted) {
        accept_at = MAX((((w_val + (w_val * barter->irritation) / 10) * 2) / 3), 1);

        if (barter->c_amount > 0)
            current_off = barter->c_amount;
        else
            current_off = item_buy_cost(ch, merchant, wanted, shop_nr);

        if (current_off <= 0) {
            sprintf(buf, "$N says to you:\n\r     \"%s\"", shop_index[shop_nr].m_not_sell);
            act(buf, FALSE, ch, 0, merchant, TO_CHAR);
            return;
        }
#ifdef MEAN_MERCHANTS
        /*
         * won't ignore assholes if the offer equal or more 
         */
        if (c_val < current_off && find_no_barter(&shop_index[shop_nr], ch)) {
            sprintf(buf, "$N says to you:\n\r     \"%s\"", shop_index[shop_nr].m_no_barter);
            act(buf, FALSE, ch, 0, merchant, TO_CHAR);
            return;
        }
#endif
        diff_perc = ((current_off - c_val) * 100) / current_off;

        if (c_val < ((REFUSE_PERC * accept_at) / 100) + 1) {
            sprintf(buf, "$N says to you:\n\r     \"%s\"", shop_index[shop_nr].m_no_barter);
            act(buf, FALSE, ch, 0, merchant, TO_CHAR);
#ifdef MEAN_MERCHANTS
            add_to_no_barter(&shop_index[shop_nr], ch, 4);
#endif
            remove_from_barter(ch, merchant);
            return;
        }

        if (c_val >= current_off
            || (merchant->player.race == RACE_GALITH && ch->specials.eco >= 100)) {
            sprintf(buf, "$N says to you:\n\r     \"%s\"", shop_index[shop_nr].m_accept);
            act(buf, FALSE, ch, 0, merchant, TO_CHAR);
            barter->c_amount = c_amount;
            if (merchant->player.race == RACE_GALITH)
                barter->c_amount = (c_amount < 100) ? c_amount : 100;
            barter->m_object = wanted;
            return;
        }

        if (merchant->player.race == RACE_GALITH) {
            sprintf(buf, "$N says to you:\n\r     \"%s\"", shop_index[shop_nr].m_no_barter);
            act(buf, FALSE, ch, 0, merchant, TO_CHAR);
            return;
        }

        /*
         * if ch's haggle <= difference percent (+/-20) or % > ch's haggle 
         * + 20, then refuse 
         */
        if ((haggle <= number(diff_perc - 20, diff_perc + 20))
            || (hf = (number(1, 100) > (haggle + 20)))
            /*
             * difficult enough without this || !number (0, 1) -Morg 
             */
            ) {
            if (hf)
                gain_skill(ch, SKILL_HAGGLE, 1);

            barter->irritation++;
            if (barter->irritation < 3)
                act("$N insists on keeping the price the same for $p.", FALSE, ch, wanted, merchant,
                    TO_CHAR);
            else if (barter->irritation < 4)
                act("$N angrily insists on keeping the price the same for $p.", FALSE, ch, wanted,
                    merchant, TO_CHAR);
            else if (barter->irritation < 5)
                act("$N looks really irritated as $E refuses your offer for $p.", FALSE, ch, wanted,
                    merchant, TO_CHAR);
            else {
                act("$N tells you to go away.", FALSE, ch, 0, merchant, TO_CHAR);
#ifdef MEAN_MERCHANTS
                add_to_no_barter(&shop_index[shop_nr], ch, 4);
#endif
                remove_from_barter(ch, merchant);
            }
            return;
        }

        sprintf(buf, "$N says to you:\n\r     \"I shall give you %s for %d %s\"",
                OSTR(wanted, short_descr), new_amount =
                ((70 * current_off + 30 * c_val) / 100), units);

        act(buf, FALSE, ch, 0, merchant, TO_CHAR);
        new_amount = MAX(new_amount, c_val);
        barter->c_amount = new_amount;
        barter->m_amount = 0;
        barter->c_object = (OBJ_DATA *) 0;
        barter->m_object = wanted;
    } else if (trade && !wanted) {

#ifdef MEAN_MERCHANTS
        /*
         * won't buy shit from assholes 
         */
        if (find_no_barter(&shop_index[shop_nr], ch)) {
            sprintf(buf, "$N says to you:\n\r     \"%s\"", shop_index[shop_nr].m_no_barter);
            act(buf, FALSE, ch, 0, merchant, TO_CHAR);
            return;
        }
#endif
        accept_at = MAX((((c_val - (c_val * barter->irritation) / 10) * 3) / 2), 1);

        if (barter->m_amount > 0)
            current_off = barter->m_amount;
        else
            current_off = item_sell_cost(ch, merchant, trade, shop_nr);

        if (current_off <= 0) {
            sprintf(buf, "$N says to you:\n\r     \"%s\"", shop_index[shop_nr].m_not_buy);
            act(buf, FALSE, ch, 0, merchant, TO_CHAR);
            return;
        }

        diff_perc = ((current_off - w_val) * 100) / current_off;

        if (w_val > (((200 - REFUSE_PERC) * accept_at) / 100) + 1) {
            sprintf(buf, "$N says to you:\n\r     \"%s\"", shop_index[shop_nr].m_no_barter);
            act(buf, FALSE, ch, 0, merchant, TO_CHAR);
#ifdef MEAN_MERCHANTS
            add_to_no_barter(&shop_index[shop_nr], ch, 4);
#endif
            remove_from_barter(ch, merchant);
            return;
        }

        if ((w_val > current_off)
            && (merchant->player.race == RACE_GALITH)) {
            sprintf(buf, "$N says to you:\n\r     \"%s\"", shop_index[shop_nr].m_no_barter);
            act(buf, FALSE, ch, 0, merchant, TO_CHAR);
            return;
        }

        if (w_val < current_off) {
            sprintf(buf, "$N says to you:\n\r     \"%s\"", shop_index[shop_nr].m_accept);
            act(buf, FALSE, ch, 0, merchant, TO_CHAR);
            barter->m_amount = w_val;
            barter->c_object = trade;
            return;
        }

        /*
         * if ch's haggle <= difference percent (+/-20) or % > ch's haggle 
         * + 20, then refuse 
         */
        if (merchant->player.race != RACE_GALITH && w_val != current_off
            && (haggle <= number(diff_perc - 20, diff_perc + 20)
                || (hf = (number(1, 100) > haggle + 20))
                /*
                 * || !number( 0, 1 ) ) it's hard enough without a 50/50
                 * chance 
                 */
            )) {

            if (hf)
                gain_skill(ch, SKILL_HAGGLE, 1);

            barter->irritation++;
            if (barter->irritation < 3)
                act("$N insists on keeping the price the same for $p.", FALSE, ch, trade, merchant,
                    TO_CHAR);
            else if (barter->irritation < 4)
                act("$N angrily insists on keeping the price the same for $p.", FALSE, ch, trade,
                    merchant, TO_CHAR);
            else if (barter->irritation < 5)
                act("$N looks really irritated as $E refuses your offer for $p.", FALSE, ch, trade,
                    merchant, TO_CHAR);
            else {
                act("$N tells you to go away.", FALSE, ch, 0, merchant, TO_CHAR);
#ifdef MEAN_MERCHANTS
                add_to_no_barter(&shop_index[shop_nr], ch, 4);
#endif
                remove_from_barter(ch, merchant);
            }
            return;
        }

        new_amount = ((70 * current_off + 30 * w_val) / 100);


        /*
         * The value of armor is now a proportional to how repaired it is. 
         * It can never be worth less than 1/FULL_VALUE, however, to keep
         * it from being entirely worthless and to block division by
         * zero. -Nessalin 9/7/2002 
         */
        if (trade->obj_flags.type == ITEM_ARMOR) {
            if (trade->obj_flags.value[0] < 1) {
                if (trade->obj_flags.value[1] < 1)
                    new_amount = 0;
                else
                    new_amount = (new_amount * (100 / trade->obj_flags.value[1])) / 100;
            } else {
                if (trade->obj_flags.value[1] < 1)
                    new_amount = 0;
                else
                    new_amount =
                        new_amount * ((trade->obj_flags.value[0] * 100) /
                                      trade->obj_flags.value[1]) / 100;
            }
        }

        if (IS_SET(trade->obj_flags.state_flags, OST_BLOODY))
            new_amount = new_amount * .80;
        if (IS_SET(trade->obj_flags.state_flags, OST_SWEATY))
            new_amount = new_amount * .95;


        if (new_amount != barter->m_amount) {
            sprintf(buf, "$N says to you:\n\r" "     \"I shall buy %s from you for %d %s.\"",
                    OSTR(trade, short_descr), new_amount, units);
        } else {
            sprintf(buf, "$N says to you:\n\r" "     \"Let's keep the price at %d %s, shall we?\"",
                    new_amount, units);
        }

        act(buf, FALSE, ch, 0, merchant, TO_CHAR);
        new_amount = MIN(new_amount, w_val);
        barter->c_amount = 0;
        barter->m_amount = new_amount;
        barter->c_object = trade;
        barter->m_object = (OBJ_DATA *) 0;
    } else if (trade && wanted) {
#ifdef MEAN_MERCHANTS
        /*
         * won't trade shit with assholes 
         */
        if (find_no_barter(&shop_index[shop_nr], ch)) {
            sprintf(buf, "$N says to you:\n\r     \"%s\"", shop_index[shop_nr].m_no_barter);
            act(buf, FALSE, ch, 0, merchant, TO_CHAR);
            return;
        }
#endif
        c_val = item_sell_cost(ch, merchant, trade, shop_nr);
        w_val = item_buy_cost(ch, merchant, wanted, shop_nr);

        if (c_val < w_val) {
            act("$N says to you:\n\r" "     \"No deal.  That is not worth enough to me.\"", FALSE,
                ch, 0, merchant, TO_CHAR);
            remove_from_barter(ch, merchant);   /* Added to test - Kel */
            return;
        }

        sprintf(buf, "$N says to you:\n\r     \"I shall give you %s for %s.\"",
                OSTR(wanted, short_descr), OSTR(trade, short_descr));
        act(buf, FALSE, ch, 0, merchant, TO_CHAR);
        barter->c_amount = 0;
        barter->m_amount = 0;
        barter->c_object = trade;
        barter->m_object = wanted;
    }
}

void
empty_object(OBJ_DATA * obj, CHAR_DATA * ch)
{
    OBJ_DATA *temp_obj;
    if (!ch || !ch->in_room || !obj) {
        gamelog("error empty_object");
        return;
    }
    if (obj->contains) {
        if (obj->obj_flags.type == ITEM_CONTAINER) {
            act("$n empties the contents of $p onto the ground.", FALSE, ch, obj, 0, TO_ROOM);
            while ((temp_obj = obj->contains)) {
                obj_from_obj(temp_obj);
                obj_to_room(temp_obj, ch->in_room);
            }
        } else
            while ((temp_obj = obj->contains)) {
                obj_from_obj(temp_obj);
                extract_obj(temp_obj);
            }
    }
    if (obj->obj_flags.type == ITEM_DRINKCON) {
        if (obj->obj_flags.value[1] != 0) {
            act("$n pours $p on the ground.", TRUE, ch, obj, 0, TO_ROOM);
        }
        obj->obj_flags.value[1] = 0;
        obj->obj_flags.value[2] = 0;
        obj->obj_flags.value[3] = 0;
    }
}

void
cmd_barter(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    CHAR_DATA *merchant;
    CHAR_DATA *tmerchant;
    struct barter_data *barter;
    char from_char[256], from_merch[256];
    char buf[256], arg1[256];
    char units[256];
    int shop_nr = 0, found = 0;
//    OBJ_DATA *wanted = NULL;
//    int soldto = FALSE;

    merchant = (CHAR_DATA *) 0; /* Initialize the pointer */

    argument = one_argument(argument, arg1, sizeof(arg1));
    if (*arg1) {
        if (!(merchant = get_char_room_vis(ch, arg1))) {
            act("You see no trader by that name with whom to barter.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }

        for (shop_nr = 0, found = FALSE; (shop_nr < max_shops) && !found; shop_nr++)
            if (shop_index[shop_nr].merchant == merchant->nr)
                found = TRUE;

        if (!found) {
            act("$N is not a merchant.", FALSE, ch, 0, merchant, TO_CHAR);
            return;
        }
    } else {
        for (tmerchant = ch->in_room->people, found = FALSE; tmerchant && !found;
             tmerchant = tmerchant->next_in_room)
            if (IS_NPC(tmerchant)) {
                for (shop_nr = 0; (shop_nr < max_shops) && !found; shop_nr++)
                    if (shop_index[shop_nr].merchant == tmerchant->nr) {
                        merchant = tmerchant;
                        found = TRUE;
                    }
            };
        if (!found) {
            act("You see no trader here with whom to barter.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }
    }


    if (!CAN_SEE(merchant, ch)) {
        /*
         * tell them will not trade with invis and return if want 
         */
    };

    // gamelog ("Checking if shop is open in cmd_barter().");

    // Temp Fix because cmd_barter() handles shop_nr differntly
    shop_nr = shop_nr - 1;
    if (!(shop_open(shop_nr, merchant)))
        return;

    if (!IS_IMMORTAL(ch))
      if ((shop_index[shop_nr].clan_only > 0)
	  && (!(in_same_tribe(merchant, ch)))) {
        cmd_say(merchant, "I only deal with my own House.", 0, 0);
        return;
      }
    // Temp Fix because cmd_barter() handles shop_nr differnetly
    shop_nr = shop_nr + 1;

    if (!(barter = get_barter(ch, merchant))) {
        act("You don't have a deal with $N right now.", FALSE, ch, 0, merchant, TO_CHAR);
        return;
    }

    /*
     * at this point merchant and barter are valid 
     */

    if (merchant->player.race == RACE_GALITH)
        sprintf(units, "hours of service");
    else
        sprintf(units, "obsidian coins");

    shop_nr = barter->shop_nr;
    if (barter->c_object) {
        if (barter->c_object->carried_by != ch) {
            sprintf(buf, "$N says to you: '%s'", shop_index[shop_nr].c_no_item);
            act(buf, FALSE, ch, 0, merchant, TO_CHAR);
            /*
             * remove_from_barter (ch, merchant); 
             */
            return;
        } else
            strcpy(from_char, OSTR(barter->c_object, short_descr));
    } else {
        if (merchant->player.race == RACE_GALITH) {
            if ((ch->specials.eco < 100)
                && (barter->c_amount > ch->specials.eco)) {
                sprintf(buf, "$N says to you: '%s'", shop_index[shop_nr].c_no_cash);
                act(buf, FALSE, ch, 0, merchant, TO_CHAR);
                /*
                 * remove_from_barter (ch, merchant); 
                 */
                return;
            }
        } else if (barter->c_amount > GET_OBSIDIAN(ch)) {
            sprintf(buf, "$N says to you: '%s'", shop_index[shop_nr].c_no_cash);
            act(buf, FALSE, ch, 0, merchant, TO_CHAR);
            /*
             * remove_from_barter (ch, merchant); 
             */
            return;
        }
        sprintf(from_char, "%d %s", barter->c_amount, units);
    }

    if (barter->m_object) {
        if (barter->m_object->carried_by != merchant) {
            sprintf(buf, "$N says to you: '%s'", shop_index[shop_nr].m_no_item);
            act(buf, FALSE, ch, 0, merchant, TO_CHAR);
            remove_from_barter(ch, merchant);
            return;
        } else
            strcpy(from_merch, OSTR(barter->m_object, short_descr));
    } else {
        if (barter->m_amount > GET_OBSIDIAN(merchant)
            && GET_RACE(merchant) != RACE_GALITH) {
            sprintf(buf, "$N says to you:\n\r     \"%s\"", shop_index[shop_nr].m_no_cash);
            act(buf, FALSE, ch, 0, merchant, TO_CHAR);
            remove_from_barter(ch, merchant);
            return;
        } else
            sprintf(from_merch, "%d %s", barter->m_amount, units);
    }

    /*
     * at this point both merchant and player have their shit and
     * from_char and from_merch have the names of the items cannot use the 
     * act function because need 4 arguments 
     */

    if (barter->c_object) {
        obj_from_char(barter->c_object);
        obj_to_char(barter->c_object, merchant);
        empty_object(barter->c_object, merchant);
    } else {
        if (merchant->player.race == RACE_GALITH)
            ch->specials.eco = (barter->c_amount > 100) ? 0 : ch->specials.eco - barter->c_amount;
        else
            ch->points.obsidian -= barter->c_amount;

#ifdef CASH_FLOW
        merchant->points.obsidian += barter->c_amount;
#endif
    }

    if (barter->m_object) {
        obj_from_char(barter->m_object);
        obj_to_char(barter->m_object, ch);

        if (IS_SET(barter->m_object->obj_flags.state_flags, OST_DUSTY))
            REMOVE_BIT(barter->m_object->obj_flags.state_flags, OST_DUSTY);
    } else {

#ifdef CASH_FLOW
        new_event(EVNT_GAIN, number(1000, 2000), merchant, 0, 0, 0,
                  MAX(1, barter->m_amount / (number(3, 10))));
        merchant->points.obsidian -= barter->m_amount;
#endif

        if (merchant->player.race == RACE_GALITH)
            ch->specials.eco =
                (ch->specials.eco + barter->m_amount >
                 100) ? 100 : ch->specials.eco + barter->m_amount;
        else
            ch->points.obsidian += barter->m_amount;
    };

    /*
     * objects have been exchanged 
     */

    /*
     * send monitor message 
     */
    sprintf(buf, "%s gives %s %s in exchange for %s.\n\r", MSTR(ch, name), MSTR(merchant, name),
            from_char, from_merch);
    send_to_monitor(ch, buf, MONITOR_MERCHANT);

    sprintf(buf, "You give $N %s in exchange for %s.", from_char, from_merch);
    act(buf, FALSE, ch, 0, merchant, TO_CHAR);

    sprintf(buf, "$n gives you %s in exchange for %s.", from_char, from_merch);
    act(buf, FALSE, ch, 0, merchant, TO_VICT);


    if (!barter->m_object) {
        if (barter->m_amount > 1)
            sprintf(from_merch, "%s coins", numberize(barter->m_amount));
        else
            strcpy(from_merch, "a coin");
  //      wanted = (barter->c_object);
  //      soldto = TRUE;
    } else if (!barter->c_object) {
        if (barter->c_amount > 1)
            sprintf(from_char, "%s coins", numberize(barter->c_amount));
        else
            strcpy(from_char, "a coin");
  //      wanted = (barter->m_object);
  //      soldto = FALSE;
    }

    sprintf(buf, "$n gives $N %s in exchange for %s.", from_char, from_merch);
    act(buf, FALSE, ch, 0, merchant, TO_NOTVICT);

    /*
     * messages have been sent, now do the clean up work 
     */

    if ((barter->m_object) && shop_producing(barter->m_object, shop_nr))
        obj_to_char(read_object(barter->m_object->nr, REAL), merchant);
    remove_from_barter(ch, merchant);

    /*
     * I added this check to make merchants NOT stack up with objects but
     * for some reason it's not working well. - Ur
     * 
     * testobj = get_obj_in_list(merchant, OSTR( barter->c_object, name ), 
     * merchant->carr ying); if (!testobj) {
     * obj_from_char(barter->c_object); obj_to_char(barter->c_object,
     * merchant); } else { extract_obj(barter->c_object); }
     * 
     * Added this too, but again it's not working right.  -Ur
     * 
     * testobj = get_obj_in_list(merchant, OSTR( barter->c_object, name ),
     * merchant->carrying); if (!testobj) {
     * obj_from_char(barter->c_object); obj_to_char(barter->c_object,
     * merchant); } else { extract_obj(barter->c_object); } 
     */
}


void
update_no_barter(void)
{
    int shop_nr;
    struct no_barter_data *nb, *nb_next;

    for (shop_nr = 0; shop_nr < max_shops; shop_nr++) {
        if (shop_index[shop_nr].no_barter) {
            for (nb = shop_index[shop_nr].no_barter; nb; nb = nb_next) {
                nb_next = nb->next;
                if (nb->duration >= 1 && nb->duration != -1)
                    nb->duration--;

                if (nb->duration == 0)
                    remove_no_barter(&shop_index[shop_nr], nb);
            }
        }
    }
};


void show_obj_properties(OBJ_DATA * obj, CHAR_DATA * ch, int bits);
void assess_object(CHAR_DATA * ch, OBJ_DATA * obj, int verbosity);

void
cmd_view(CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
    char obj_name[MAX_INPUT_LENGTH], shopkeeper_name[MAX_INPUT_LENGTH];
    CHAR_DATA *merchant;
    OBJ_DATA *obj;
    int uniq_count, shop_nr = 0;
    inventory_item inventory[MAX_INVENTORY];

    argument = one_argument(argument, obj_name, sizeof(obj_name));
    argument = one_argument(argument, shopkeeper_name, sizeof(shopkeeper_name));

    if (shopkeeper_name[0]) {
        if (!(merchant = get_char_room_vis(ch, shopkeeper_name))) {
            send_to_char("You do not see that merchant here.\n\r", ch);
            return;
        }

        for (shop_nr = 0; shop_nr < max_shops; shop_nr++)
            if (shop_index[shop_nr].merchant == merchant->nr)
                break;

        if (shop_nr == max_shops) {
            act("$N has nothing to trade.", FALSE, ch, 0, merchant, TO_CHAR);
            return;
        }

        if (!CAN_SEE(merchant, ch) && !IS_IMMORTAL(ch)) {
            cmd_say(merchant, "I'll not deal with those I cannot see.", 0, 0);
            return;
        }
    } else {
        merchant = get_first_vis_merchant(ch);

        if (!merchant) {
            act("You see no trader here with whom to barter.", FALSE, ch, 0, 0, TO_CHAR);
            return;
        }

        if (!CAN_SEE(merchant, ch) && !IS_IMMORTAL(ch)) {
            cmd_say(merchant, "I'll not deal with those I cannot see.", 0, 0);
            return;
        }
    }

    if (!IS_NPC(merchant)) {
        act("I think you better talk to $N.", FALSE, ch, 0, merchant, TO_CHAR);
        return;
    }

    uniq_count =
        sort_unique_inventory(merchant, ch, inventory, sizeof(inventory) / sizeof(inventory_item));

    if (*obj_name == '#') {
        int which = atoi(&obj_name[1]);

        obj = NULL;
        if ((which > 0) && (which < uniq_count + 1))
            obj = inventory[which - 1].obj;

        if (!obj) {
            act("$N is not carrying an item with that number.", FALSE, ch, 0, merchant, TO_CHAR);
            return;
        }
    } else if (!(obj = get_obj_in_inv_list(inventory, uniq_count, obj_name))) {
        act("$N is not carrying an item with that name.", FALSE, ch, 0, merchant, TO_CHAR);
        return;
    }

    if (OSTR(obj, description))
        page_string(ch->desc, OSTR(obj, description), 1);
    else
        send_to_char("You don't see anything unusual about it.\n\r", ch);

    show_obj_properties(obj, ch, 0);

    assess_object(ch, obj, 1);
    if( has_skill(ch, SKILL_VALUE )) {
       value_object(ch, obj, TRUE, TRUE, TRUE);
    }

    if (obj->obj_flags.type == ITEM_ARMOR) {
        /*
         * can you wear it? 
         */
        if (!size_match(ch, obj) && (shop_producing(obj, shop_nr))) {
            send_to_char("You see several of them here.\n\r"
                         "If you are willing to buy without bartering "
                         "(use the buy command), you\n\rcan get it in " "your size.\n\r", ch);
        }
        /*
         * If the armor is wieldable / holdable, it should be a shield.
         * So, we'll let them know if it's too heavy or not to use. 
         */
        if ((obj->obj_flags.type == ITEM_ARMOR)
            && (CAN_WEAR(obj, ITEM_ES) || CAN_WEAR(obj, ITEM_EP))) {
            int weight = GET_OBJ_WEIGHT(obj);

            if (!size_match(ch, obj) && obj->obj_flags.value[2] && (shop_producing(obj, shop_nr))) {
               weight = weight * get_race_average_number(GET_RACE(ch)) / obj->obj_flags.value[2];
                if (weight > GET_STR(ch))
                    send_to_char("If you resize it, you decide it would be to heavy to use.", ch);
                else
                    send_to_char("If you resize it, you think you could use it.\n\r", ch);
            } 

            if (weight > GET_STR(ch))
                send_to_char("You test its weight and decide that it "
                             "is too heavy for you to use.\n\r", ch);
            else
                send_to_char("You test its weight and decide that you " "could use it.\n\r", ch);

        }
    }
    return;
}

