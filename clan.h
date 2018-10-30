/*
 * File: CLAN.H
 * Usage: Definitions for the clans and tribes of Zalanthas.
 *
 * Copyright (C) 1997 by Mike Zeman and the creators ArmageddonMud.
 * May the fiery pits of Suk-krath ravage any who tresspass!
 *
 *   /\    ____                    PART OF          __     ____   |\   |
 *  /__\   |___|   /\  /\    /\    __  ___  +--\   |  \   /    \  | \  |
 * /    \  |   \  /  \/  \  /--\  /    [_   |   \  |   |  |    |  |  \ |
 * \    /  |   /  \      /  \  /  \_]  [__  |___/  |__/   \____/  |   \|
 * ---------------------------------------------------------------------
 */

#ifndef __CLAN_INCLUDED
#define __CLAN_INCLUDED

/* tribe defs... */
#define TRIBE_ANY            -1 /* beware trying to use this in an INDEX */
#define TRIBE_NONE            0
#define TRIBE_KUTPTEI         1 /* Tuluk noble clan */
#define TRIBE_KADIUS          2
#define TRIBE_SALARR          3
#define TRIBE_KURAC           4
#define TRIBE_VORYEK          5
#define TRIBE_BLACK_MOON      6
#define TRIBE_CAI_SHYZN       7
#define TRIBE_UTEP            8 /* Tuluk city clan */
#define TRIBE_PTAR_KEN        9
#define TRIBE_ORDER_VIVADU   10


#define TRIBE_HARUCH         11
#define TRIBE_ALLANAK_MIL    12 /* Allanak city clan */
#define TRIBE_BLACK_HAND     13
#define TRIBE_CONCLAVE       14
#define TRIBE_ORIYA          15

#define TRIBE_ORDER_ELEMENTS 16
#define TRIBE_ORDER_OF_THE_ELEMENTS       16
#define TRIBE_HOUSE_TOR      17
#define TRIBE_SUN_RUNNERS                 18

#define TRIBE_HOUSE_FALE     22
#define TRIBE_HOUSE_BORSAIL  23
#define TRIBE_AZIA           24 /* gypsy clan */
#define TRIBE_BW             25 /* Blackwing */
#define TRIBE_BLACKWING                   25
#define TRIBE_KOHMAR         28

#define TRIBE_FAOLE          33 /* Galith tribe */

#define TRIBE_ALLANAKI_LIBERATION_ALLIANCE 34
#define TRIBE_STEINEL_ELVES  35

#define TRIBE_OASH           36
#define TRIBE_HOUSE_OASH           36

#define TRIBE_HAND_DRAGON    40 /* allanaki elementalist slave */
#define TRIBE_PLAINSFOLK     45
#define TRIBE_HOUSE_BORSAIL_SLAVE         50
#define TRIBE_ALLANAK_SLAVE               51
#define TRIBE_HOUSE_REYNOLTE_SLAVE        52
#define TRIBE_HOUSE_FALE_SLAVE            53
#define TRIBE_SEVEN_SPEARS                54
#define TRIBE_MOON_DANCERS                55
#define TRIBE_THORNWALKERS               56
#define TRIBE_HOUSE_DELANN                57
#define TRIBE_TOR_SCORPIONS               58
/* TRIBE 59 is defined to be the same as TRIBE 51, I have no idea why */
#define TRIBE_GOL_KRATHU_SLAVE            60
#define TRIBE_TAR_KROH                    61
#define TRIBE_KURAC_SPECIAL_OPS           62
#define TRIBE_THE_REBELLION               63    /* Tuluki Rebel-resistance clan */
#define TRIBE_AKEI_TA_VAR                 64    /* Desert elf magicky/preserver clan */
#define TRIBE_RED_STORM_MILITIA           65    /* RSV and RSE */
#define TRIBE_HOUSE_KURAC_MILITARY_OPERATIONS    66     /* House Kurac Guards and Mercs */
#define TRIBE_DESERT_TALKERS              67
#define TRIBE_NENYUK                      68
#define TRIBE_DUST_RUNNERS                69    /* Secret Kuraci smugglers */
#define TRIBE_AL_SEIK                     70    /* Tablelands tribe */
#define TRIBE_BENJARI                     71    /* Tablelands tribe */
#define TRIBE_ARABET                      72    /* Tablelands tribe */
#define TRIBE_SPIRIT_TRACKERS             73    /* Spirit Trackers */
#define TRIBE_HOUSE_MORLAINE              74    /* Allanak Noble */
#define TRIBE_HOUSE_MORLAINE_SERVANTS     75    /* Allanak Noble Servants */

#define TRIBE_WINROTHOL                   76    /* Tuluki Noble House */
#define TRIBE_WINROTHOL_SERVANTS          77    /* Their servants     */

#define TRIBE_TWO_MOONS                   78    /* desert elves */
#define TRIBE_PRIDE                       79    /* desert elves */
#define TRIBE_KRE_TAHN                    80    /* still more desert elves */
#define TRIBE_SOH_LANAH_KAH               81    /* yep, desert elves */
#define SERVANTS_OF_NENYUK                82    /* Nenyuki servants */
#define TRIBE_TULUK_TEMPLAR               83    /* Tuluki Templar */
#define TRIBE_HOUSE_TENNESHI              84    /* House Tenneshi */
#define TRIBE_TENNESHI_SERVANTS           85    /* House Tenneshi Servants */
#define TRIBE_SILT_WINDS                  86    /* desert elves */
#define TRIBE_BARDS_OF_POETS_CIRCLE       87    /* Bards of Poets' Circle */
#define TRIBE_BLACKMARKETEERS             88    /* Blackmarketeers of the rinth */
#define TRIBE_SERVANTS_OF_THE_DRAGON      89    /* Servants of the Dragon */
#define TRIBE_CENYR                       90    /* Cenyr - 3/8/2003 */
#define TRIBE_FAH_AKEEN                   91    /* Fah Akeen - delf clan */
#define TRIBE_ATRIUM                      92    /* Pearl's Atrium */
#define TRIBE_RUENE                       93    /* Ruene */
#define TRIBE_DUNE_STALKER                94    /* DELF tribe */
#define TRIBE_SAND_JAKHALS                95    /* DELF tribe */
#define TRIBE_RED_FANGS                   96    /* DELF tribe */
#define TRIBE_SANDAS                      97    /* DELF tribe */
#define TRIBE_JUL_TAVAN                   98    /* Nomad Tribe */
#define TRIBE_ZIMAND_DUR                  99    /* Zimand Gur, raider tribe, added for Nidhogg */
#define TRIBE_ALLANAK_MILITIA_RECRUITS   100    /* AoD recruits.  Added for Naiona */
#define TRIBE_GUESTS_OF_THE_ATRIUM	 101    /* Added by Rae for Adhira to keep clan crafts separate from students etc */
#define TRIBE_HOUSE_NEGEAN               102    /* Northern noble house Negean */
#define TRIBE_SERVANTS_OF_NEGEAN         103    /* Servant clan of House Negean */
#define TRIBE_HOUSE_DASARI               104    /* Northern noble house Dasari */
#define TRIBE_SERVANTS_OF_DASARI         105    /* Servant clan of House Dasari */
#define TRIBE_HOUSE_KASSIGARH            106    /* Northern noble house Kassigarh */
#define TRIBE_SERVANTS_OF_KASSIGARH      107    /* Servant clan of House Kassigarh */
#define TRIBE_HOUSE_UAPTAL               108    /* Northern noble house Uaptal */
#define TRIBE_SERVANTS_OF_UAPTAL         109    /* Servant clan of House Uaptal */
#define TRIBE_HOUSE_LYKSAE               110    /* Northern noble house Lyksae */
#define TRIBE_SERVANTS_OF_LYKSAE         111    /* Servant clan of House Lyksae */
#define TRIBE_COUNCIL_OF_ALLANAKI_MAGES  112    /* Council of Allanaki Mages */
#define TRIBE_RECRUITS_OF_UTEP_SUN_CLAN  113    /* Recruits of Utep Sun Clan */
#define TRIBE_ELAN_PAH_ALLIANCE          114    /* Elan Pah Alliance */
#define TRIBE_MALARN_TRIBALS             115    /* Malarn Tribals */
#define TRIBE_AJINN_ACADEMY              116    /* Ajinn Academy in Tuluk */
#define TRIBE_THE_TOWER			 117	/* Tower of Hope and Change */
#define TRIBE_AKAI_SJIR			 118	/* Northern elven clan */
#define TRIBE_FAITHFULS_OWN              119    /* Aides to Tuluki Templarate */

#define TRIBE_SALARR_FAMILY              126    /* Family members of House Salarr */
#define TRIBE_NENYUK_FAMILY              127    /* Family members of House Nenyuk */
#define TRIBE_KADIUS_FAMILY              128    /* Family members of House Kadius */
#define TRIBE_KURAC_FAMILY               129    /* Family members of House Kurac */


#define MAX_CLAN_FROM_FILE 1 // need to add your own clan data

extern unsigned int MAX_CLAN;

/*
[11] Haruch Kemad
[12] Arm of the Dragon
[13] Black Hand
[14] Conclave
[15] Oriya Clan
[16] Order of the Elements
[17] House Tor
[18] Sun Runners
[19] Ironsword
[20] The Guild
[21] Society of the Vault
[22] House Fale
[23] House Borsail
[24] Tan Muark
[25] Blackwing Tribe
[26] House Fale Guards
[27] Obsidian Order
[28] House Kohmar
[29] T'zai Byn
[30] Silt-Jackals
[31] Southland Gith Clan
[32] Servants of the Shifting Sands
[33] Daine
[34] Allanaki Liberation Alliance
[35] Steinel Ruins Elves
[36] House Oash
[37] Slaves of House Oash
[38] Slaves of House Tor
[39] Servants of Borsail
[40] Hand of the Dragon
[41] Servants of Oash
[42] Cult of the Dragon
[43] Followers of the Lady
[44] Feydakin
[45] Plainsfolk
[46] Hillpeople
[47] House Reynolte
[48] Gith Mesa Clan
[49] Guards of House Reynolte
[50] Slaves of House Borsail
[51] Slaves of Allanak
[52] Slaves of House Reynolte
[53] Slaves of House Fale
[54] Seven Spear
[55] Baizan Nomadic Tribes
[56] Thornwalkers
[57] House Delann
[58] Tor Scorpions
[59] Slaves of Allanak
[60] Slaves of Gol Krathu
[61] Tar Kroh
[62] Kuraci Special Operations
[63] The Rebellion
*/

struct clan_rank_struct
{
    int num_ranks;         /* number of ranks in the clan */
    char **rank;           /* array of strings being the names of the ranks */
};

#define CLAN_RANK_FILE "data_files/clan_ranks/clan_%d.ranks"
#define CLAN_BANK_FILE "data_files/clan_banks/clan_%d.bank"

bool set_num_clan_ranks(int clan, int num);
bool set_clan_rank_name(int clan, int num, const char *name);
char *get_clan_rank_name(int clan, int num);

void send_clan_ranks_to_char(int clan, CHAR_DATA * ch);

void save_clan_rank_names(int clan);
void unboot_clan_rank_names();
void boot_clan_rank_names();

#endif /* __CLAN_INCLUDED */

