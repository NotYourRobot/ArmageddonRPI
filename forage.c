/*
 * File: forage.c */
#define _GNU_SOURCE
#include <string.h>
//#include "db_utils.h"
#include "gamelog.h"
#include "structs.h"
#include "utility.h"
#include "constants.h"
#include "modify.h"

typedef struct forage_data {
  struct __frg_item *forage_item;
  struct forage_data *next;
} forage_node;


typedef enum {
    FORAGE_STONE,
    FORAGE_KINDLING,
    FORAGE_COTTON,
    FORAGE_FOOD,
    FORAGE_WOOD,
    FORAGE_SALT,
    FORAGE_ARTIFACT,
    FORAGE_FIRE,
    FORAGE_AIR,
    FORAGE_WATER,
    FORAGE_CRYSTAL,
    FORAGE_SHADOW,
    FORAGE_VOID,
    FORAGE_OTHER,
    FORAGE_SPICE
} ForageType;

static ForageType get_forage_type(const char *type);

typedef struct __frg_item {
    ForageType  type;
    int         sector;
    char       *other_name;
    int         vnum;
    int         flags;
    int         shares;
} ForageItem;

typedef enum {
    FORAGE_FLAG_NONE = 0,
    FORAGE_FLAG_COMMON = 1 << 0,
    FORAGE_FLAG_UNCOMMON = 1 << 1,
    FORAGE_FLAG_RARE = 1 << 2,
    FORAGE_FLAG_ULTRA_RARE = 1 << 3,
} ForageFlags;

const char * forage_flagname[] = {
    "deprecated",
    "common", 
    "uncommon",
    "rare",
    "ultra-rare"
};

const char *
get_forage_flagname(ForageFlags flags)
{
    if (flags & FORAGE_FLAG_ULTRA_RARE)
        return forage_flagname[4];
    if (flags & FORAGE_FLAG_RARE)
        return forage_flagname[3];
    if (flags & FORAGE_FLAG_UNCOMMON)
        return forage_flagname[2];
    if (flags & FORAGE_FLAG_COMMON)
        return forage_flagname[1];
   return forage_flagname[0];
}

long
get_forage_bit(const char *name) {
  if (name && !strcmp(name, forage_flagname[1])) {
    return FORAGE_FLAG_COMMON;
  } else if (name && !strcmp(name, forage_flagname[2])) {
    return FORAGE_FLAG_UNCOMMON;
  } else if (name && !strcmp(name, forage_flagname[3])) {
    return FORAGE_FLAG_RARE;
  } else if (name && !strcmp(name, forage_flagname[4])) {
    return FORAGE_FLAG_ULTRA_RARE;
  }
  
  return FORAGE_FLAG_NONE;
}

static ForageItem *forage_items = 0;
static size_t      num_forage_items = 0;

void
boot_forage()
{

	return;
	// NOT IMPLEMENTED - could do XML or flat file load but mysql is easier so just took it out
	/*
    MYSQL_RES *result;
    MYSQL_ROW row;

    if ((result = db_store_result("SELECT type, sector, vnum, shares, rare FROM forage_items")) == 0) {
        gamelogf( "Error reading forage_items table from DB\n\r");
        gamelogf( "MySQL Reported Error:  %s", db_error());
        return;
    }

    num_forage_items = mysql_num_rows(result);
    if (!num_forage_items) {
        db_free_result(result);
        return;
    }

    forage_items = calloc(num_forage_items, sizeof(ForageItem));

    int rownum = 0;
    while ((row = mysql_fetch_row(result))) {
        forage_items[rownum].type = get_forage_type(row[0]);
        forage_items[rownum].sector = get_attrib_val(room_sector, row[1]);
        if (forage_items[rownum].sector == -1) {
            gamelogf("invalid sector in forage data: '%s'", row[1]);
        }
        forage_items[rownum].vnum = atoi(row[2]);
        forage_items[rownum].shares = atoi(row[3]);
        forage_items[rownum].flags = atoi(row[4]);
        rownum++;
    }

    if (num_forage_items != rownum)
        gamelog("A bug has happened in loading forage items!");

    db_free_result(result);
*/
}

void
unboot_forage()
{
    int i;
    if (!forage_items)
        return;

    for (i = 0; i < num_forage_items; i++) {
        free(forage_items[i].other_name);
    }
    free(forage_items);
    forage_items = 0;
}

typedef struct {
    ForageType type;
    int sector;
    int bonus;
} ForageBonus;

ForageBonus forage_bonus_list[] = {
    /* Rock bonuses */
    { FORAGE_STONE, SECT_DESERT,	5 },
    { FORAGE_STONE, SECT_HILLS,	30 },
    { FORAGE_STONE, SECT_FIELD,	10 },
    { FORAGE_STONE, SECT_RUINS,	20 },
    { FORAGE_STONE, SECT_MOUNTAIN,	50 },
    { FORAGE_STONE, SECT_COTTONFIELD,	10 },
    { FORAGE_STONE, SECT_THORNLANDS,	10 },
    { FORAGE_STONE, SECT_SOUTHERN_CAVERN,	30 },
    { FORAGE_STONE, SECT_CAVERN,	30 },
    { FORAGE_STONE, SECT_EARTH_PLANE,	30 },
    /* kindling bonuses */
    { FORAGE_KINDLING, SECT_HEAVY_FOREST,	80 },
    { FORAGE_KINDLING, SECT_LIGHT_FOREST,	60 },
    { FORAGE_KINDLING, SECT_THORNLANDS,	40 },
    { FORAGE_KINDLING, SECT_FIELD,	40 },
    /* Cotton bonuses */
    { FORAGE_COTTON, SECT_COTTONFIELD,	20 },
    /* Food bonuses */
    { FORAGE_FOOD, SECT_FIELD,	20 },
    { FORAGE_FOOD, SECT_THORNLANDS,	10 },
    { FORAGE_FOOD, SECT_LIGHT_FOREST,	5 },
    { FORAGE_FOOD, SECT_HEAVY_FOREST,	5 },
    { FORAGE_FOOD, SECT_SOUTHERN_CAVERN,	30 },
    { FORAGE_FOOD, SECT_CAVERN,	30 },
    /* Salt bonuses */
    { FORAGE_SALT, SECT_SALT_FLATS,	5 },
    /* Wood bonuses */
    { FORAGE_WOOD, SECT_LIGHT_FOREST,	5 },
    { FORAGE_WOOD, SECT_HEAVY_FOREST,	50 },
    { FORAGE_WOOD, SECT_THORNLANDS,	10 },
    /* Spice bonuses */
    { FORAGE_SPICE, SECT_SHALLOWS,	5 },
    { FORAGE_SPICE, SECT_SILT,		5 },
};

int
get_forage_bonus(ForageType type, int sector)
{
    int i;
    for (i = 0; i < sizeof(forage_bonus_list)/sizeof(forage_bonus_list[0]); i++) {
        if (forage_bonus_list[i].type == type
            && forage_bonus_list[i].sector == sector) {
            return forage_bonus_list[i].bonus;
        }
    }

    return 0;
}

typedef struct __frg_match {
    const char *name;
    ForageType type;
} forage_match;

forage_match forage_list[] = {
    { "stones", FORAGE_STONE },
    { "rocks", FORAGE_STONE },
    { "wood", FORAGE_WOOD },
    { "branches", FORAGE_WOOD },
    { "kindling", FORAGE_KINDLING },
    { "cotton", FORAGE_COTTON },
    { "food", FORAGE_FOOD },
    { "roots", FORAGE_FOOD },
    { "mushrooms", FORAGE_FOOD },
    { "artifacts", FORAGE_ARTIFACT },
    { "salts", FORAGE_SALT },
    { "fire", FORAGE_FIRE },
    { "air", FORAGE_AIR },
    { "water", FORAGE_WATER },
    { "crystals", FORAGE_CRYSTAL },
    { "shadows", FORAGE_SHADOW },
    { "void", FORAGE_VOID },
    { "spice", FORAGE_SPICE },
};

static ForageType
get_forage_type(const char *type)
{
    int i;
    for (i = 0; i < sizeof(forage_list)/sizeof(forage_list[0]); i++) {
        if (!strnicmp(type, forage_list[i].name, strlen(type))) {
            return forage_list[i].type;
        }
    }

    return FORAGE_OTHER;
}


const char *
get_forage_typename(ForageType type)
{
    int i;
    for (i = 0; i < sizeof(forage_list)/sizeof(forage_list[0]); i++) {
        if (forage_list[i].type == type) {
            return forage_list[i].name;
        }
    }

    return "(bogus forage type)";
}


int
count_forage_matches(ForageType type, int sector)
{
  int i, matches = 0;
  
  for (i = 0; i < num_forage_items; i++) {
    if (forage_items[i].type == type &&
        forage_items[i].sector == sector) {
      matches += forage_items[i].shares;
    }
  }

  // Add a section here to look for edescs and increase matches
  
  return matches;
}


struct forage_share {
  int vnum;
  int shares;
};

int
get_forage_item(ForageType type, int sector, ForageFlags flags, ROOM_DATA *room)
{
  const char *bufptr;
  char forage_edesc[MAX_STRING_LENGTH],
    arg_type[MAX_STRING_LENGTH], arg_shares[MAX_STRING_LENGTH], 
    arg_vnum[MAX_STRING_LENGTH], arg_rarity[MAX_STRING_LENGTH];
  int i = 0,
    total_shares = 0,
    current_vnum = 0,
    current_shares = 0,
    current_flags = 0;
  ForageType current_type;
  struct forage_share vnums[100];
  
 count_total_shares: {
    int vnum_counter = 0;
    for (i = 0; i < num_forage_items; i++) {
      if (forage_items[i].type == type &&
          forage_items[i].sector == sector &&
          (!(forage_items[i].flags ^ flags))) {
        total_shares += forage_items[i].shares;
        vnums[vnum_counter].vnum = forage_items[i].vnum;
        vnums[vnum_counter++].shares = forage_items[i].shares;
      }
    }
    
    bufptr = forage_edesc;
    if (get_room_extra_desc_value(room, "[CUSTOM_FORAGE]", forage_edesc, sizeof(forage_edesc))) {
      bufptr = one_argument(bufptr, arg_type, sizeof(arg_type));
      while (strcmp(arg_type, "")) {
        current_type = get_forage_type(arg_type); 
        bufptr = one_argument(bufptr, arg_shares, sizeof(arg_shares));
        if (strcmp(arg_shares, "") && is_number(arg_shares)) {
          current_shares = atoi(arg_shares); 
          bufptr = one_argument(bufptr, arg_vnum, sizeof(arg_vnum));
          if (strcmp(arg_vnum,"") && is_number(arg_vnum)) {
            current_vnum = atoi(arg_vnum); 
            bufptr = one_argument(bufptr, arg_rarity, sizeof(arg_rarity));
            if (strcmp(arg_rarity, "")) {
              current_flags = get_forage_bit(arg_rarity);
            }
          }
        }
        if (strcmp(arg_type, "") &&
            strcmp(arg_shares, "") &&
            strcmp(arg_vnum, "") &&
            strcmp(arg_rarity, "")) {
          if (current_type == type && 
              (!(current_flags ^ flags))) {
            total_shares += current_shares;;
            vnums[vnum_counter].vnum = current_vnum;
            vnums[vnum_counter++].shares = current_shares;
          }
        }
        bufptr = one_argument(bufptr, arg_type, sizeof(arg_type));
        current_type = 0; current_flags = 0; current_shares = 0; current_vnum = 0;
      }
    }
  }
  
  if (total_shares == 0 && flags != FORAGE_FLAG_NONE) {
    flags >>= 1;
    for (i = 0; i < 100; i++) {
      vnums[i].vnum = 0;
      vnums[i].shares = 0;
    }
    goto count_total_shares;
  }
  
  if (0 == total_shares) {
    return 0;
  }
  
  int matchnum = number(0, total_shares - 1);
  
  int new_counter = 0;
  while (new_counter < 100 && vnums[new_counter].vnum != 0) {
    if (matchnum - vnums[new_counter].shares < 0) {
      return vnums[new_counter].vnum;
    }
    matchnum -= vnums[new_counter].shares;
    new_counter++;
  } 
  
  gamelog("get_forage_item() ran out fo shares, please notify a coder.");
  return 0;
}


void
forage_stat(CHAR_DATA *ch)
{
  int i;
  const char *bufptr;
  char forage_edesc[MAX_STRING_LENGTH],
    arg_type[MAX_STRING_LENGTH], arg_shares[MAX_STRING_LENGTH],  
    arg_vnum[MAX_STRING_LENGTH], arg_rarity[MAX_STRING_LENGTH];  
  int current_vnum = 0,
    current_shares = 0,
    current_flags = 0;
  ForageType current_type;
  
  for (i = 0; i < num_forage_items; i++) {
    if (forage_items[i].sector == ch->in_room->sector_type) {
      int realnr = real_object(forage_items[i].vnum);
      cprintf(ch, "  %s \t- [%d] %s (%s) %d shares\n\r",
              get_forage_typename(forage_items[i].type),
              forage_items[i].vnum,
              obj_default[realnr].name,
              get_forage_flagname(forage_items[i].flags),
              forage_items[i].shares);
    }
  }
  
  bufptr = forage_edesc;
  if (get_room_extra_desc_value(ch->in_room, "[CUSTOM_FORAGE]", forage_edesc, sizeof(forage_edesc))) {
    cprintf(ch, "[CUSTOM_FORAGE] values:\n\r");
    bufptr = one_argument(bufptr, arg_type, sizeof(arg_type));
    while (strcmp(arg_type, "")) {
      current_type = get_forage_type(arg_type);
      bufptr = one_argument(bufptr, arg_shares, sizeof(arg_shares));
      if (strcmp(arg_shares, "") && is_number(arg_shares)) {
        current_shares = atoi(arg_shares);
        bufptr = one_argument(bufptr, arg_vnum, sizeof(arg_vnum));
        if (strcmp(arg_vnum,"") && is_number(arg_vnum)) {
          current_vnum = atoi(arg_vnum);
          bufptr = one_argument(bufptr, arg_rarity, sizeof(arg_rarity));
          if (strcmp(arg_rarity, "")) {
            current_flags = get_forage_bit(arg_rarity);
          }
        }
      }
      if (strcmp(arg_type, "") &&
          strcmp(arg_shares, "") &&
          strcmp(arg_vnum, "") &&
          strcmp(arg_rarity, "")) {
        
        int realnr = real_object(current_vnum);
        cprintf(ch, "  %s \t- [%d] %s (%s) %d shares\n\r",
                get_forage_typename(current_type),
                current_vnum,
                obj_default[realnr].name,
                get_forage_flagname(current_flags),
                current_shares);
      }
      bufptr = one_argument(bufptr, arg_type, sizeof(arg_type));
      current_type = 0; current_flags = 0; current_shares = 0; current_vnum = 0;
    }
  }
}

void
add_forage_info_to_buf(char *buf, size_t bufsize, int vnum)
{
    int i;
    for (i = 0; i < num_forage_items; i++) {
        if (forage_items[i].vnum == vnum) {
            char output[1024];
            snprintf(output, sizeof(output), "Can be foraged as '%s' in '%s'\n\r",
                    get_forage_typename(forage_items[i].type),
                    get_attrib_name(room_sector, forage_items[i].sector));
            string_safe_cat(buf, output, bufsize);
        }
    }
}

// Function for hiding objects in a room to be recovered via forage
void
cmd_bury (CHAR_DATA * ch, const char *argument, Command cmd, int count)
{
  char arg1[MAX_STRING_LENGTH];
  char preHow[MAX_STRING_LENGTH];
  char postHow[MAX_STRING_LENGTH];
  char args[MAX_STRING_LENGTH];
  char keyword[MAX_STRING_LENGTH];
  char groundbuf[MAX_STRING_LENGTH];
  char to_char[MAX_STRING_LENGTH];
  char to_room[MAX_STRING_LENGTH];
  char msg[MAX_STRING_LENGTH];
 // CHAR_DATA *tmp_char;
  OBJ_DATA *obj_object = NULL;

  extract_emote_arguments (argument, preHow, sizeof (preHow), args,
			   sizeof (args), postHow, sizeof (postHow));

  argument = one_argument (args, arg1, sizeof (arg1));

  // Check for burying based on room sector; if so, set up echo phrase
  if (ch->in_room)
    {
      switch (ch->in_room->sector_type)
	{
	case SECT_CAVERN:
	case SECT_SOUTHERN_CAVERN:
	case SECT_HILLS:
	case SECT_MOUNTAIN:
	  sprintf (groundbuf, "rocky ground");
	  break;
	case SECT_LIGHT_FOREST:
	case SECT_HEAVY_FOREST:
	  sprintf (groundbuf, "earthen ground");
	  break;
	case SECT_DESERT:
	  sprintf (groundbuf, "sand");
	  break;
	case SECT_COTTONFIELD:
	case SECT_FIELD:
	case SECT_RUINS:
	case SECT_THORNLANDS:
	  sprintf (groundbuf, "dusty ground");
	  break;
	case SECT_SALT_FLATS:
	  sprintf (groundbuf, "salty ground");
	  break;
	case SECT_SEWER:
	  sprintf (groundbuf, "sludge");
	  break;
	case SECT_SILT:
	case SECT_SHALLOWS:
	  sprintf (groundbuf, "silt");
	  break;
	default:
	  cprintf(ch, "You can't bury anything here.\n\r");
	  return;
	  break;
	}
    }
  else
    sprintf (groundbuf, "ground");

  // bury 'all'
  if (strcasestr (arg1, "all") == arg1)
    {
      bool found = FALSE;
      char *search = NULL;
      char buffer[MAX_STRING_LENGTH];
      OBJ_DATA *next_obj;

      if (strlen (arg1) > 4 && strcasestr (arg1, "all.") == arg1)
	search = arg1 + 4;
      for (obj_object = ch->carrying; obj_object; obj_object = next_obj) {
	  next_obj = obj_object->next_content;
	  if (CAN_SEE_OBJ (ch, obj_object) &&
	      (search == NULL ||
	       (search && (isname (search, OSTR (obj_object, name)) ||
			   isname (search,
				   real_sdesc_keywords_obj (obj_object,
							    ch))))))
	    {
	      find_obj_keyword(obj_object, FIND_OBJ_INV, ch, keyword, sizeof(keyword));
	      sprintf (to_char, "you bury ~%s in the %s", keyword, groundbuf);
	      sprintf (to_room, "@ buries ~%s in the %s", keyword, groundbuf);
	      send_to_char_and_room_parsed (ch, preHow, postHow, to_char,
					    to_room, to_room, !*preHow &&
					    !*postHow ? MONITOR_NONE :
					    MONITOR_OTHER);
	      obj_from_char(obj_object);
	      obj_to_artifact_cache (obj_object, ch->in_room);
	      sprintf(msg, "%s buries %s in room #%d.\n\r", MSTR(ch, name), OSTR(obj_object, short_descr), ch->in_room->number);
	      send_to_monitor(ch, msg, MONITOR_CRAFTING);
	      qgamelogf(QUIET_DEBUG, "cmd_bury: %s", msg);

	      found = TRUE;

	      if (!IS_IMMORTAL (ch)) {
                  WAIT_STATE(ch, number(3, 5)); // Typical delay
		  sprintf (buffer, "bury %s", arg1);
		  parse_command (ch, buffer);
		  return;
	      }
	    }
      }
      if (found)
	send_to_char ("OK.\n\r", ch);
      else {
	  if (search == NULL)
	    send_to_char ("You are not carrying anything.\n\r", ch);
	  else
	    cprintf (ch, "You are not carrying any '%s'.\n\r", search);
      }
    } else {
      // bury 'item' from inventory
      CHAR_DATA *temp_char;
      int obj_type = generic_find(arg1, FIND_OBJ_INV, ch, &temp_char, &obj_object);
      if (!obj_object) {
	  cprintf (ch, "You are not carrying any '%s'.\n\r", arg1);
	  return;
      }
      // Put it into the cache
      sprintf (to_char, "you bury ~%s in the %s", arg1, groundbuf);
      sprintf (to_room, "@ buries ~%s in the %s", arg1, groundbuf);
      send_to_char_and_room_parsed (ch, preHow, postHow, to_char,
		      to_room, to_room, !*preHow &&
		      !*postHow ? MONITOR_NONE : MONITOR_OTHER);
      obj_from_char(obj_object);
      obj_to_artifact_cache(obj_object, ch->in_room);
      sprintf(msg, "%s buries %s in room #%d.\n\r", MSTR(ch, name), OSTR(obj_object, short_descr), ch->in_room->number);
      send_to_monitor(ch, msg, MONITOR_CRAFTING);
      qgamelogf(QUIET_DEBUG, "cmd_bury: %s", msg);
      WAIT_STATE(ch, number(3, 5)); // Typical delay
    }
  return;
}

// Skill for finding objects through searching the room
void
skill_forage(byte level, CHAR_DATA * ch, const char *arg, int si, CHAR_DATA * tar_ch,
             OBJ_DATA * tar_obj)
{
    int chance, vnum = 0;
    char msg[256];
    char buf[MAX_STRING_LENGTH];
    char arg1[MAX_STRING_LENGTH];
    char arg2[MAX_STRING_LENGTH] = "";
    bool specific_item = FALSE;
    ForageType type;
    OBJ_DATA *found = NULL, *artifact_cache = NULL;
    ForageFlags forage_flags = FORAGE_FLAG_COMMON;

    if (!ch->in_room)
        return;

    if (ch->specials.riding) {
        send_to_char("You can't possibly do this while mounted.\n\r", ch);
        return;
    }

    if (((IS_DARK(ch->in_room)) || (visibility(ch) <= 0))
        && (!IS_IMMORTAL(ch)) && (!IS_AFFECTED(ch, CHAR_AFF_INFRAVISION))) {
        send_to_char("It is too difficult to see the ground.\n\r", ch);
        return;
    }

    if (ch->specials.fighting) {
        send_to_char("You are fighting for your life!\n\r", ch);
        return;
    }

    if (hands_free(ch) < 1) {
        send_to_char("You need at least one free hand to forage.\n\r", ch);
        return;
    }

    if (!is_merchant(ch) && !IS_IMMORTAL(ch)
          && IS_CARRYING_N(ch) + 1 > CAN_CARRY_N(ch)) {
        act("You cannot carry more, you have too many items.", FALSE, ch, 0, 0, TO_CHAR);
        return;
    }

    if (GET_MOVE(ch) < 5) {
        cprintf(ch, "You are too tired to continue.\n\r");
        return;
    }

    adjust_move(ch, -1);

    if (IS_SET(ch->in_room->room_flags, RFL_NO_FORAGE)) {
        send_to_char("You look around but can't find anything.\n\r", ch);
        sprintf(msg, "%s's location has the no_forage flag.\n\r", MSTR(ch, name));
        send_to_monitor(ch, msg, MONITOR_CRAFTING);
        return;
    }
    
    arg = one_argument(arg, arg1, sizeof(arg1));
    
    type = get_forage_type(arg1);
    
    if (type == FORAGE_OTHER && !stricmp(arg1, "for")) {
      send_to_char("You must specify a category when using the 'for' syntax.\n\r", ch);
      return;
    }
    
    // Look for a specific item
    arg = one_argument(arg, arg2, sizeof(arg2));
    if (!stricmp(arg2, "for")) {
      arg = one_argument(arg, arg1, sizeof(arg1));
      specific_item = TRUE;
    }
    
    if (type == FORAGE_FOOD &&
        (GET_GUILD(ch) != GUILD_RANGER) &&
        (GET_SUB_GUILD(ch) != SUB_GUILD_SCAVENGER) &&
        (GET_SUB_GUILD(ch) != SUB_GUILD_GREBBER) &&
        (GET_SUB_GUILD(ch) != SUB_GUILD_SORC_RANGER) &&
        (GET_RACE(ch) != RACE_HALFLING)) {
      send_to_char("You look around but find no food.\n\r", ch);
      return;
    }
    
    sprintf(msg, "%s begins foraging for %s.\n\r", MSTR(ch, name), specific_item ? arg1 : get_forage_typename(type));
    send_to_monitor(ch, msg, MONITOR_CRAFTING);
    
    // Is there an artifact cache here to deal with?
    artifact_cache = find_artifact_cache(ch->in_room);

    // Just not possible to find anything like that here
    if (count_forage_matches(type, ch->in_room->sector_type) == 0 &&
	!(type == FORAGE_ARTIFACT && artifact_cache)) {
      cprintf(ch, "It seems that '%s' cannot be found here.\n\r",
              type == FORAGE_OTHER ?  arg1 : get_forage_typename(type));
      return;
    }

    chance = get_skill_percentage(ch, SKILL_FORAGE);
    chance += get_forage_bonus(type, ch->in_room->sector_type);

    int roll =  number(1, 100);
    if (roll > chance) {
      cprintf(ch, "You look around, but don't find any %s.\n\r", type == FORAGE_OTHER ? arg1 : get_forage_typename(type));
      gain_skill(ch, SKILL_FORAGE, 1);
      return;
    }
    
    if (roll == 1)  { // 1% chance for ultra-rare
      forage_flags = FORAGE_FLAG_ULTRA_RARE;
    } else if (roll <= MIN(5, chance/5)) {// 5% chance for rare
      forage_flags = FORAGE_FLAG_RARE;
    } else if (roll <= MIN(25, chance/3)) { // 25% chance for uncommon
      forage_flags = FORAGE_FLAG_UNCOMMON;
    } else {
      forage_flags = FORAGE_FLAG_COMMON;
    }
    
    roomlogf(ch->in_room, "forage_flags: %s, roll: %d", get_forage_flagname(forage_flags), roll);
    
    // Find foragable artifacts in an artifact cache
    if (type == FORAGE_ARTIFACT && artifact_cache && artifact_cache->contains) {
      found = artifact_cache->contains; // Pop from the top of the list
      obj_from_obj(found); // Take it out of the cache
      qgamelogf(QUIET_DEBUG, "skill_forage: extracting %s for %s in room %d.\n\r", OSTR(found, short_descr), MSTR(ch, name), ch->in_room->number);
      if (!artifact_cache->contains) { // Emptied the cache
        extract_obj(artifact_cache); // Housekeeping
        artifact_cache = NULL;
      }
    }
    
    // Find foragable item based on room sector
    vnum = get_forage_item(type, ch->in_room->sector_type, forage_flags, ch->in_room);
    
    if (!found && (!vnum || (found = read_object(vnum, VIRTUAL)) == NULL)) {
      if (specific_item) {
        sprintf(buf, "%s %s", arg1, get_forage_typename(type));
      } else {
        sprintf(buf, "%s", type == FORAGE_OTHER ? arg1 : get_forage_typename(type));
      }
      
      cprintf(ch, "You look around, but don't find any %s.\n\r", buf);
      
      if (vnum) {
        gamelogf("skill_forage: bad object vnum %d", vnum);
      }
      
      return;
    }
    // End of find block
    
    // Does what they found match the keyword they were looking for?
    if (specific_item && !(isname(arg1, real_sdesc_keywords_obj(found, ch)) || (isname(arg1, OSTR(found, name))))) {
      cprintf(ch, "You look around, but don't find any %s %s.\n\r", arg1,
              get_forage_typename(type));
      extract_obj(found);
      return;
    }
    
    // Check for weight and notake flag, put the object in the room
    if (!CAN_WEAR(found, ITEM_TAKE) ||
	(IS_CARRYING_W(ch) + GET_OBJ_WEIGHT(found)) > CAN_CARRY_W(ch)) {
      act("$n reveals $p on the ground.", FALSE, ch, found, NULL, TO_ROOM);
      act("You reveal $p on the ground.", FALSE, ch, found, NULL, TO_CHAR);
      obj_to_room(found, ch->in_room);
    } else {
      act("$n grabs $p off the ground.", FALSE, ch, found, NULL, TO_ROOM);
      act("You find $p and pick it up.", FALSE, ch, found, NULL, TO_CHAR);
      obj_to_char(found, ch);
    }
    
    sprintf(msg, "%s finds %s.\n\r", MSTR(ch, name), OSTR(found, short_descr));
    send_to_monitor(ch, msg, MONITOR_CRAFTING);

    return;
}

