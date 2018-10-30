/*
 * File: immortal.h */
void add_account_comment(CHAR_DATA * ch, PLAYER_INFO * pPInfo, char *comment);
bool has_extra_cmd(EXTRA_CMD * commands, Command cmd, int level, bool granted);
void update_karma_options(CHAR_DATA * ch, PLAYER_INFO * pPInfo, int prev);

