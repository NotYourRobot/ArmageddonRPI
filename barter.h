/*
 * File: BARTER.H
 * Usage: Definitions for buying, trading, and selling.
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

#ifndef __BARTER_INCLUDED
#define __BARTER_INCLUDED

#define MAX_TRADE 10
#define MAX_PROD 30
#define MAX_NEW  30

#define SHOP_FILE   "data_files/shops"
#define SHOP_FILES  "data_files/shop_files"

#define CASH_FLOW

#define LOSS_PERC    50
#define REFUSE_PERC  70
#define HOUSE_PERC   50

#define MEAN_MERCHANTS

#define NESS_MERCHANTS 1

/*
 * no_barter added 4/3/98 -Morg
 * used to stop merchants from bartering with people for timer or permanent
 */
struct no_barter_data
{
    char *name;
    int duration;
    struct no_barter_data *next;
};


struct shop_data
{
    int producing[MAX_PROD];    /* which item to produce           */
    int new_item[MAX_NEW];      /* random list of new items        */
    float prof_buy[MAX_TRADE];  /* factor to multiply cost with    */
    float prof_sel[MAX_TRADE];  /* factor to multiply cost with    */
    byte buy_type[MAX_TRADE];   /* which item to trade             */
    byte sel_type[MAX_TRADE];   /* which item to trade             */

    char c_no_item[256];        /* message if merchant hasn't got an item  */
    char m_no_item[256];        /* message if player hasn't got an item    */
    char c_no_cash[256];        /* message if merchant hasn't got cash     */
    char m_no_cash[256];        /* message if player hasn't got cash       */
    char m_not_buy[256];        /* if merchant doesn't buy such things     */
    char m_not_sell[256];       /* if merchant doesn't sell such things    */
    char m_no_barter[256];      /* if merchant gets mad                    */
    char m_accept[256];         /* if merchant agrees to barter            */
    char m_no_mat[256];         /* if merchant doesn't buy material type   */
    char m_close[256];          /* Shop closes command                     */
    char m_open[256];           /* Shop opens command                      */

    int buy_mats[24];           /* on/off for each mat type +1 for 'all'   */

    int merchant;               /* the npc who owns the shop               */
    int clan_only;              /* who does the shop trade with            */
    int barter;                 /* will merchant barter                    */

    int open_hour;              /* Hour shop opens                         */
    int close_hour;             /* Hour shop closes                        */

    struct no_barter_data *no_barter;   /* who they won't deal with 4/3/98 -M */
};


struct barter_data
{
    int shop_nr;

    struct char_data *merchant;
    struct char_data *customer;

    /* one or the other */
    struct obj_data *m_object;
    int m_amount;

    /* one or the other */
    struct obj_data *c_object;
    int c_amount;

    byte irritation;

    struct barter_data *next;
};

void boot_shops();
void unboot_shops();

const struct shop_data *get_shop_data(CHAR_DATA *merchant);
void add_random_items(CHAR_DATA *shopkeeper, const struct shop_data *shopdata);
void remove_random_items(CHAR_DATA *shopkeeper, const struct shop_data *shopdata);
void remove_items_for_economy(CHAR_DATA *shopkeeper, const struct shop_data *shopdata);

CHAR_DATA *get_first_vis_merchant(CHAR_DATA *ch);

#endif

