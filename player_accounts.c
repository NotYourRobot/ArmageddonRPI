/*
 * File: PLAYER_ACCOUNTS.C
 * Usage: Player Account functions.
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

#define _GNU_SOURCE

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/dir.h>
#include <errno.h>

#include <sys/stat.h>

#include "structs.h"
#include "handler.h"
#include "modify.h"
#include "comm.h"
#include "creation.h"
#include "cities.h"
#include "utility.h"
#include "utils.h"
#include "guilds.h"
#include "skills.h"
#include "db_file.h"
#include "history.h"
#include "gamelog.h"

//char *crypt(const char *key, const char *salt);

/* allocate space and set defaults for a player_info (account) structure */
PLAYER_INFO *
alloc_player_info(void)
{
    PLAYER_INFO *pPInfo;
    int i;

    CREATE(pPInfo, PLAYER_INFO, 1);

    pPInfo->next = NULL;
    pPInfo->name = NULL;
    pPInfo->email = NULL;
    pPInfo->password = NULL;
    pPInfo->characters = NULL;
    pPInfo->num_chars = 0;
    pPInfo->num_chars_alive = 0;
    pPInfo->num_chars_applying = 0;
    pPInfo->num_chars_allowed = 1;
    pPInfo->karma = 0;
    pPInfo->karmaRaces = KINFO_RACE_DEFAULT;
    pPInfo->karmaGuilds = KINFO_GUILD_DEFAULT;
    pPInfo->tempKarmaRaces = 0;
    pPInfo->tempKarmaGuilds = 0;

    for (i = 0; i < MAX_LOGINS_SAVED; i++)
        pPInfo->last_on[i] = NULL;

    for (i = 0; i < MAX_LOGINS_SAVED; i++)
        pPInfo->last_failed[i] = NULL;

    pPInfo->karmaLog = NULL;
    pPInfo->flags = PINFO_VERBOSE_HELP;
    pPInfo->edit_end = '~';

    return pPInfo;
}


/* find a character currently (somewhere in the game) that has the requested
 * email or account name
 */
PLAYER_INFO *
find_player_info(char *name)
{
    DESCRIPTOR_DATA *d;

    for (d = descriptor_list; d; d = d->next) {
        if (d->player_info && ((d->player_info->email && !strcasecmp(d->player_info->email, name))
                               || (d->player_info->name
                                   && !strcasecmp(d->player_info->name, name)))) {
            return d->player_info;
        }
    }

    return NULL;
}


/* allocate and initialize a char_info
 * (char_info is a character stub for player account info)
 */
CHAR_INFO *
alloc_char_info(void)
{
    CHAR_INFO *pChInfo;

    CREATE(pChInfo, CHAR_INFO, 1);
    pChInfo->next = NULL;
    pChInfo->name = NULL;
    pChInfo->status = APPLICATION_NONE;

    return pChInfo;
}

LOGIN_DATA *
new_login_data(void)
{
    LOGIN_DATA *pLData;

    CREATE(pLData, LOGIN_DATA, 1);

    pLData->site = NULL;
    pLData->in = NULL;
    pLData->out = NULL;
    pLData->descriptor = -1;
    return pLData;
}

void
free_login_data(LOGIN_DATA * pLData)
{
    if (!pLData)
        return;

    if (pLData->site)
        free(pLData->site);

    if (pLData->in)
        free(pLData->in);

    if (pLData->out)
        free(pLData->out);

    free(pLData);
}


void
add_in_login_data(PLAYER_INFO * pPInfo, int descriptor, char *site)
{
    long ct;
    int i;
    char *tmstr;

    if (!pPInfo)
        return;

    /* clear the last if it exists */
    if (pPInfo->last_on[MAX_LOGINS_SAVED - 1]) {
        free_login_data(pPInfo->last_on[MAX_LOGINS_SAVED - 1]);
        pPInfo->last_on[MAX_LOGINS_SAVED - 1] = NULL;
    }

    /* shift down */
    for (i = MAX_LOGINS_SAVED - 1; i > 0; i--)
        pPInfo->last_on[i] = pPInfo->last_on[i - 1];

    /* create a new login data */
    pPInfo->last_on[0] = new_login_data();
    pPInfo->last_on[0]->site = strdup(site);
    pPInfo->last_on[0]->descriptor = descriptor;

    /* find the current time */
    ct = time(0);
    tmstr = asctime(localtime(&ct));
    tmstr[strlen(tmstr) - 1] = '\0';
    //    tmstr[strlen(tmstr) - 6] = '\0';

    pPInfo->last_on[0]->in = strdup(tmstr);
    pPInfo->last_on[0]->out = strdup("now");

#ifdef RECORD_HISTORY
    update_history(pPInfo, site);
#endif
    return;
}


void
add_out_login_data(PLAYER_INFO * pPInfo, int descriptor)
{
    long ct;
    char *tmstr;
    int i;

    if (!pPInfo)
        return;

    for (i = 0; i < MAX_LOGINS_SAVED; i++)
        if (pPInfo->last_on[i] && descriptor == pPInfo->last_on[i]->descriptor)
            break;

    if (i == MAX_LOGINS_SAVED)
        return;

    if (!pPInfo->last_on[i])
        return;

    if (pPInfo->last_on[i]->out)
        free(pPInfo->last_on[i]->out);

    ct = time(0);
    tmstr = asctime(localtime(&ct));
    tmstr[strlen(tmstr) - 1] = '\0';

    pPInfo->last_on[i]->out = strdup(tmstr);
    return;
}

void
add_failed_login_data(PLAYER_INFO * pPInfo, char *site)
{
    long ct;
    int i;
    char *tmstr;

    if (!pPInfo)
        return;

    /* clear the last if it exists */
    if (pPInfo->last_failed[MAX_LOGINS_SAVED - 1]) {
        free_login_data(pPInfo->last_failed[MAX_LOGINS_SAVED - 1]);
        pPInfo->last_failed[MAX_LOGINS_SAVED - 1] = NULL;
    }

    /* shift down */
    for (i = MAX_LOGINS_SAVED - 1; i > 0; i--)
        pPInfo->last_failed[i] = pPInfo->last_failed[i - 1];

    /* create a new login data */
    pPInfo->last_failed[0] = new_login_data();
    pPInfo->last_failed[0]->site = strdup(site);

    /* find the current time */
    ct = time(0);
    tmstr = asctime(localtime(&ct));
    tmstr[strlen(tmstr) - 1] = '\0';

    pPInfo->last_failed[0]->in = strdup(tmstr);

#ifdef RECORD_HISTORY
    update_history(pPInfo, site);
#endif
    return;
}


void
free_player_info(PLAYER_INFO * pPInfo)
{
    CHAR_INFO *chinfo;
    CHAR_INFO *chinfo_next;
    char fn[MAX_STRING_LENGTH];
    int i;
    DESCRIPTOR_DATA *d;

    for (d = descriptor_list; d; d = d->next)
        if (d->player_info == pPInfo)
            return;

    for (chinfo = pPInfo->characters; chinfo; chinfo = chinfo_next) {
        chinfo_next = chinfo->next;
        free_char_info(chinfo);
    }

    /* remove the lock file */
    if (pPInfo->name && descriptor_list) {
        sprintf(fn, "%s/%s", ACCOUNTLOCK_DIR, pPInfo->name);
        remfile(fn);
    }

    /* free last logged in info */
    for (i = 0; i < MAX_LOGINS_SAVED; i++) {
        if (pPInfo->last_on[i]) {
            free_login_data(pPInfo->last_on[i]);
        }
    }

    /* free last failed info */
    for (i = 0; i < MAX_LOGINS_SAVED; i++) {
        if (pPInfo->last_failed[i]) {
            free_login_data(pPInfo->last_failed[i]);
        }
    }

    /* free strings */
    free(pPInfo->name);
    free(pPInfo->created);
    free(pPInfo->email);
    free(pPInfo->password);
    free(pPInfo->comments);
    free(pPInfo->kudos);
    free(pPInfo->karmaLog);
    free(pPInfo);
    return;
}


/* free memory associated with a char_info && the char_info itself */
void
free_char_info(CHAR_INFO * pChInfo)
{
    if (pChInfo->name)
        free(pChInfo->name);
    pChInfo->name = NULL;

    pChInfo->next = NULL;
    free(pChInfo);
    return;
}


/* adds a new character & status to a player account info */
void
add_new_char_info(PLAYER_INFO * pPInfo, const char *name, int status)
{
    CHAR_INFO *pChInfo, *lastChInfo;

    if (name == NULL) {
        return;
    }

    if (lookup_char_info(pPInfo, name) != NULL) {
        update_char_info(pPInfo, name, status);
        return;
    }

    /* create a new char_info and fill it in with supplied info */
    pChInfo = alloc_char_info();

    pChInfo->name = strdup(lowercase(name));
    pChInfo->status = status;

    if (status == APPLICATION_NONE)
        pPInfo->num_chars_alive++;
    else if (status == APPLICATION_APPLYING || status == APPLICATION_ACCEPT
             || status == APPLICATION_DENY || status == APPLICATION_REVISING
             || status == APPLICATION_REVIEW)
        pPInfo->num_chars_applying++;

    pPInfo->num_chars++;

    /*
     * Find the last char so we can tack this new one on the end
     */
    for (lastChInfo = pPInfo->characters; lastChInfo; lastChInfo = lastChInfo->next) {
        if (lastChInfo->next == NULL)
            break;
    }

    /* attach the ch_info to the player_info */
    if (lastChInfo) {
        /*
         * this isn't their first character
         */
        lastChInfo->next = pChInfo;
        pChInfo->next = NULL;
    } else {
        pPInfo->characters = pChInfo;
        pChInfo->next = NULL;
    }
    return;
}


/* find the char_info for a character in an player account */
CHAR_INFO *
lookup_char_info(PLAYER_INFO * pPInfo, const char *name)
{
    CHAR_INFO *pChInfo;
    char name_buf[MAX_STRING_LENGTH];

    if (name == NULL) {
        return NULL;
    }

    // Hack to make sure that we don't have any trailing spaces
    first_name(name, name_buf);

    if (name_buf[0] == '\0') {
        return NULL;
    }

    strcpy( name_buf, lowercase(name_buf));

    for (pChInfo = pPInfo->characters; pChInfo; pChInfo = pChInfo->next)
        if (!strcmp(pChInfo->name, name_buf))
            break;

    return pChInfo;
}


/* update the player information for a specified character */
void
update_char_info(PLAYER_INFO * pPInfo, const char *name, int status)
{
    CHAR_INFO *pChInfo;

    if (pPInfo == NULL)
        return;

    if ((pChInfo = lookup_char_info(pPInfo, name)) == NULL)
        return;

    switch (pChInfo->status) {
    case APPLICATION_NONE:
        pPInfo->num_chars_alive--;
        break;
    case APPLICATION_APPLYING:
    case APPLICATION_REVISING:
    case APPLICATION_REVIEW:
    case APPLICATION_ACCEPT:
    case APPLICATION_DENY:
        pPInfo->num_chars_applying--;
        break;
    default:
    case APPLICATION_STORED:
    case APPLICATION_DEATH:
        break;
    }

    pChInfo->status = status;

    switch (pChInfo->status) {
    case APPLICATION_NONE:
        pPInfo->num_chars_alive++;
        break;
    case APPLICATION_APPLYING:
    case APPLICATION_REVISING:
    case APPLICATION_REVIEW:
    case APPLICATION_ACCEPT:
    case APPLICATION_DENY:
        pPInfo->num_chars_applying++;
        break;
    default:
    case APPLICATION_STORED:
    case APPLICATION_DEATH:
        break;
    }
}

/* remove char_info for specified name from a player account */
void
remove_char_info(PLAYER_INFO * pPInfo, const char *name)
{
    CHAR_INFO *pChInfo;

    if ((pChInfo = pPInfo->characters) == NULL)
        return;

    /*
     * First in the list
     */
    if (!strcasecmp(pChInfo->name, name)) {
        pPInfo->num_chars--;

        switch (pChInfo->status) {
        case APPLICATION_NONE:
            pPInfo->num_chars_alive--;
            break;
        case APPLICATION_APPLYING:
        case APPLICATION_REVISING:
        case APPLICATION_REVIEW:
        case APPLICATION_ACCEPT:
        case APPLICATION_DENY:
            pPInfo->num_chars_applying--;
            break;
        default:
        case APPLICATION_STORED:
        case APPLICATION_DEATH:
            break;
        }

        pPInfo->characters = pChInfo->next;
        free_char_info(pChInfo);
    } else {
        CHAR_INFO *tmp;

        for (; pChInfo->next; pChInfo = pChInfo->next)
            if (!strcmp(pChInfo->next->name, name))
                break;

        if (pChInfo->next) {
            pPInfo->num_chars--;

            switch (pChInfo->next->status) {
            case APPLICATION_NONE:
                pPInfo->num_chars_alive--;
                break;
            case APPLICATION_APPLYING:
            case APPLICATION_REVISING:
            case APPLICATION_REVIEW:
            case APPLICATION_ACCEPT:
            case APPLICATION_DENY:
                pPInfo->num_chars_applying--;
                break;
            default:
            case APPLICATION_STORED:
            case APPLICATION_DEATH:
                break;
            }

            tmp = pChInfo->next->next;

            free_char_info(pChInfo->next);
            pChInfo->next = tmp;
        }
    }

    pPInfo->num_chars = MAX(pPInfo->num_chars, 0);
    pPInfo->num_chars_alive = MAX(pPInfo->num_chars_alive, 0);
    pPInfo->num_chars_applying = MAX(pPInfo->num_chars_applying, 0);
    return;
}


/* physically write out the player_info data */
void
fwrite_player_info(PLAYER_INFO * pPInfo, FILE * fp)
{
    int i;

    fprintf(fp, "Name    %s~\n", pPInfo->name);
    fprintf(fp, "Email   %s~\n", pPInfo->email);

    for (i = 0; i < MAX_LOGINS_SAVED; i++)
        if (pPInfo->last_on[i])
            fprintf(fp, "LastOn %s~ %s~ %s~\n", pPInfo->last_on[i]->site, pPInfo->last_on[i]->in,
                    pPInfo->last_on[i]->out);

    for (i = 0; i < MAX_LOGINS_SAVED; i++)
        if (pPInfo->last_failed[i])
            fprintf(fp, "LastFailed %s~ %s~\n", pPInfo->last_failed[i]->site,
                    pPInfo->last_failed[i]->in);

    fprintf(fp, "Password   %s~\n", pPInfo->password);
    fprintf(fp, "Comments\n%s~\n", pPInfo->comments);
    fprintf(fp, "Kudos\n%s~\n", pPInfo->kudos);
    fprintf(fp, "NumChars   %d\n", pPInfo->num_chars);
    fprintf(fp, "NumCharsAlive   %d\n", pPInfo->num_chars_alive);
    fprintf(fp, "NumCharsApplying   %d\n", pPInfo->num_chars_applying);
    fprintf(fp, "NumCharsAllowed   %d\n", pPInfo->num_chars_allowed);
    fprintf(fp, "Flags   %d\n", pPInfo->flags);
    fprintf(fp, "Karma   %d\n", pPInfo->karma);
    fprintf(fp, "KarmaLog   %s~\n", pPInfo->karmaLog);
    fprintf(fp, "KarmaRaces   %ld\n", pPInfo->karmaRaces);
    fprintf(fp, "KarmaGuilds   %ld\n", pPInfo->karmaGuilds);
    fprintf(fp, "TempKarmaRaces   %ld\n", pPInfo->tempKarmaRaces);
    fprintf(fp, "TempKarmaGuilds   %ld\n", pPInfo->tempKarmaGuilds);
    fprintf(fp, "EditEnd %c\n", pPInfo->edit_end);

    if (pPInfo->created && pPInfo->created[0] != '\0') {
        fprintf(fp, "Created %s~\n", pPInfo->created);
    }

    return;
}



/* write out the char_info data */
void
fwrite_char_info(CHAR_INFO * chinfo, FILE * fp)
{
    /* recursive calls to maintain order when reading in file */
    if (chinfo->next) {
        fwrite_char_info(chinfo->next, fp);
    }

    fprintf(fp, "Character  %s~ %d\n", chinfo->name, chinfo->status);
    return;
}


/* Save a player account file (char_info data & all ) */
void
save_player_info(PLAYER_INFO * pPInfo)
{
    char strsave[MAX_INPUT_LENGTH];
    struct stat stbuf;
    FILE *fp;
    char name[MAX_STRING_LENGTH];
    char email[MAX_STRING_LENGTH];

    sprintf(strsave, "%s/%s", ACCOUNT_DIR, pPInfo->name);

    // TODO: When switch over to the DB look for existence of ID
    /* does the account directory exist? */
    if (stat(strsave, &stbuf) == -1) {
        //shhlogf("New account: %s", pPInfo->name);
        /* if not, make it */
        sprintf(strsave, "mkdir --mode 777 %s/%s", ACCOUNT_DIR, pPInfo->name);
        system(strsave);

        sprintf(strsave, "%s/%s", ACCOUNT_DIR, pPInfo->name);
        chmod(strsave,
              S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH |
              S_IXOTH);

        /* link the soft email index */
        sprintf(strsave, "ln -s %s/%s/%s %s/%s/%s", LIB_DIR, ACCOUNT_DIR, pPInfo->name, LIB_DIR,
                ALT_ACCOUNT_DIR, pPInfo->email);
        system(strsave);

        /* account mail */
        sprintf(strsave, "mkdir accountmail/%s", pPInfo->name);
        system(strsave);

        sprintf(strsave, "accountmail/%s", pPInfo->name);
        chmod(strsave,
              S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH |
              S_IXOTH);
 
    }

    sprintf(strsave, "%s/%s/.player_info", ACCOUNT_DIR, pPInfo->name);
    if ((fp = fopen(strsave, "w")) == NULL) {
        fprintf(stderr, "Save_player_info: fopen");
        perror(strsave);
    } else {
        fwrite_player_info(pPInfo, fp);
        if (pPInfo->characters != NULL)
            fwrite_char_info(pPInfo->characters, fp);
        fprintf(fp, "End\n");
    }

    if( fp != NULL )
       fclose(fp);

    chmod(strsave,
          S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);

    return;
}




#if defined(KEY)
#undef KEY
#endif

#define KEY( literal, field, value )                    \
                    if ( !strcasecmp( word, literal ) )    \
                    {                                   \
                       field  = value;                  \
                       fMatch = TRUE;                   \
                       break;                           \
                    }

#define SKEY( literal, field )                    \
                    if ( !strcasecmp( word, literal ) )    \
                    {                                   \
                       free( field );                   \
                       field  = fread_string_no_whitespace(fp); \
                       fMatch = TRUE;                   \
                       break;                           \
                    }

void
fread_player_info(PLAYER_INFO * pPInfo, FILE * fp)
{
    char *word;
    bool fMatch;
    int i;
    char *t;

    pPInfo->email = strdup("");
    pPInfo->password = strdup("");
    pPInfo->comments = strdup("");
    pPInfo->kudos = strdup("");
    pPInfo->num_chars = 0;
    pPInfo->num_chars_alive = 0;
    pPInfo->num_chars_allowed = 0;
    pPInfo->karma = 0;
    pPInfo->karmaLog = NULL;
    pPInfo->flags = 0;
    pPInfo->characters = NULL;
    pPInfo->next = NULL;
    pPInfo->edit_end = '~';
    pPInfo->karmaRaces = KINFO_RACE_DEFAULT;
    pPInfo->karmaGuilds = KINFO_GUILD_DEFAULT;
    pPInfo->created = NULL;

    for (i = 0; i < MAX_LOGINS_SAVED; i++)
        pPInfo->last_on[i] = NULL;

    for (;;) {
        word = feof(fp) ? "End" : ((t = fread_word(fp)) ? t : "End");
        fMatch = FALSE;

        switch (UPPER(word[0])) {
        case '*':
            fMatch = TRUE;
            fread_to_eol(fp);
            break;

        case 'C':
            if (!strcasecmp(word, "Character")) {
                CHAR_INFO *chinfo;

                fMatch = TRUE;
                chinfo = alloc_char_info();

                chinfo->name = fread_string_no_whitespace(fp);
                chinfo->status = fread_number(fp);
                chinfo->next = pPInfo->characters;
                pPInfo->characters = chinfo;
            }
            SKEY("Comments", pPInfo->comments);
            SKEY("Created", pPInfo->created);
            break;

        case 'E':
            SKEY("Email", pPInfo->email);

            if (!strcasecmp(word, "EditEnd")) {
                char *tmp = fread_word(fp);
                if (!tmp) {
                    fprintf(stderr, "Tried to enter something too long in email.");
                    fMatch = TRUE;
                    pPInfo->edit_end = 'a';
                } else {
                    fMatch = TRUE;
                    pPInfo->edit_end = tmp[0];
                };
            }

            if (!strcasecmp(word, "End")) {
                /* To fix anyone who inadvertantly got set to "none" */
                if (pPInfo->karmaRaces == 0)
                    pPInfo->karmaRaces = KINFO_RACE_DEFAULT;
                if (pPInfo->karmaGuilds == 0)
                    pPInfo->karmaGuilds = KINFO_GUILD_DEFAULT;
            
                if (pPInfo->kudos[0] == '\0' && pPInfo->comments != NULL ) {
                   int index = 0;
                   int lastStart = 0;
                   int len = strlen(pPInfo->comments);
                   char temp_buf[MAX_STRING_LENGTH];
                   char temp_comments[MAX_STRING_LENGTH];
                   char temp_kudos[MAX_STRING_LENGTH];
            
                   temp_buf[0] = '\0';
                   temp_comments[0] = '\0';
                   temp_kudos[0] = '\0';
            
                   // look at their comments
                   for( index = 0; index < len; index++ ) {
                      if( pPInfo->comments[index] == '\r' ) {
                          lastStart = index + 1;
                          continue;
                      }
            
                      // move the current char into the buffer
                      temp_buf[index - lastStart] = pPInfo->comments[index];
                      temp_buf[index + 1 - lastStart] = '\0';
            
                      // did we find end of line?
                      if( pPInfo->comments[index] == '\n' ) {
                         // cap the end of the line
                         temp_buf[index + 1 - lastStart] = '\r';
                         temp_buf[index + 2 - lastStart] = '\0';
            
                         // look and see if it contains kudos
                         if( strcasestr(temp_buf, "kudo") != NULL ) {
                            // add it to the kudos buffer
                            strcat(temp_kudos, temp_buf);
                         } else { // must be a comment
                            strcat(temp_comments, temp_buf);
                         }
            
                         temp_buf[0] = '\0';
                         lastStart = index + 1;
                      }
                   }
            
                   // if there are kudos
                   if( temp_kudos[0] != '\0' ) {
                       // clean-up the old
                       free(pPInfo->kudos);
                       pPInfo->kudos = strdup(temp_kudos);
                       free(pPInfo->comments);
                       pPInfo->comments = strdup(temp_comments);
                   }
                }
                return;
            }
            break;

        case 'F':
            KEY("Flags", pPInfo->flags, fread_number(fp));
            break;

        case 'K':
            KEY("Karma", pPInfo->karma, fread_number(fp));
            SKEY("KarmaLog", pPInfo->karmaLog);
            KEY("KarmaRaces", pPInfo->karmaRaces, fread_number(fp));
            KEY("KarmaGuilds", pPInfo->karmaGuilds, fread_number(fp));
            SKEY("Kudos", pPInfo->kudos);
            break;

        case 'L':
            if (!strcasecmp(word, "LastFailed")) {
                for (i = 0; i < MAX_LOGINS_SAVED; i++) {
                    if (!pPInfo->last_failed[i]) {
                        pPInfo->last_failed[i] = new_login_data();

                        pPInfo->last_failed[i]->site = fread_string_no_whitespace(fp);
                        pPInfo->last_failed[i]->in = fread_string_no_whitespace(fp);
                        pPInfo->last_failed[i]->out = NULL;
                        break;
                    }
                }

                /* if we didn't find an empty spot, just trash it */
                if (i == MAX_LOGINS_SAVED) {
#define FREAD_PLAYER_INFO_MEMORY1
#ifdef FREAD_PLAYER_INFO_MEMORY1
                    /* small memory leak   2002.12.27  -- Tenebrius */
                    /* remove else bits and comments if still no    */
                    /* problems by 2003.5.27                        */

                    char *tmp;

                    tmp = fread_string_no_whitespace(fp);
                    if (tmp)
                        free(tmp);

                    tmp = fread_string_no_whitespace(fp);
                    if (tmp)
                        free(tmp);

#else
                    fread_string_no_whitespace(fp);
                    fread_string_no_whitespace(fp);

#endif

                }

                fMatch = TRUE;
                break;
            }

            if (!strcasecmp(word, "LastOn")) {
                for (i = 0; i < MAX_LOGINS_SAVED; i++) {
                    if (!pPInfo->last_on[i]) {
                        pPInfo->last_on[i] = new_login_data();

                        pPInfo->last_on[i]->site = fread_string_no_whitespace(fp);
                        pPInfo->last_on[i]->in = fread_string_no_whitespace(fp);
                        pPInfo->last_on[i]->out = fread_string_no_whitespace(fp);
                        if (!strcmp(pPInfo->last_on[i]->out, "now")) {
                            free(pPInfo->last_on[i]->out);
                            pPInfo->last_on[i]->out = strdup("crash");
                        }
                        break;
                    }
                }

                /* if we didn't find an empty spot, just trash it */
                if (i == MAX_LOGINS_SAVED) {

#define FREAD_PLAYER_INFO_MEMORY2
#ifdef FREAD_PLAYER_INFO_MEMORY2
                    /* small memory leak   2002.12.27  -- Tenebrius */
                    /* remove else bits and comments if still no    */
                    /* problems by 2003.5.27                        */
                    char *tmp;

                    tmp = fread_string_no_whitespace(fp);
                    if (tmp)
                        free(tmp);
                    tmp = fread_string_no_whitespace(fp);
                    if (tmp)
                        free(tmp);
                    tmp = fread_string_no_whitespace(fp);
                    if (tmp)
                        free(tmp);
#else
                    fread_string_no_whitespace(fp);
                    fread_string_no_whitespace(fp);
                    fread_string_no_whitespace(fp);

#endif


                }

                fMatch = TRUE;
                break;
            }
            break;

        case 'N':
            SKEY("Name", pPInfo->name);
            KEY("NumCharsAlive", pPInfo->num_chars_alive, fread_number(fp));
            KEY("NumCharsApplying", pPInfo->num_chars_applying, fread_number(fp));
            KEY("NumCharsAllowed", pPInfo->num_chars_allowed, fread_number(fp));
            KEY("NumChars", pPInfo->num_chars, fread_number(fp));
            break;

        case 'P':
            SKEY("Password", pPInfo->password);
            break;

        case 'T':
            KEY("TempKarmaRaces", pPInfo->tempKarmaRaces, fread_number(fp));
            KEY("TempKarmaGuilds", pPInfo->tempKarmaGuilds, fread_number(fp));
            break;
        }

        if (!fMatch) {
            fprintf(stderr, "Fread_player_info: no match.");
            fread_to_eol(fp);
        }
    }
}



/* load a player info from file */
bool
load_player_info(DESCRIPTOR_DATA * d, char *name)
{
    char strsave[MAX_INPUT_LENGTH];
    FILE *fp;
    bool found;

    found = FALSE;

    snprintf(strsave, sizeof(strsave), "%s/%s/.player_info", ACCOUNT_DIR, name);

    /* try and open player_info file */
    if ((fp = fopen(strsave, "r")) != NULL) {
        found = TRUE;

        d->player_info = alloc_player_info();
        if (d->player_info->name)
            free(d->player_info->name);
        d->player_info->name = strdup(name);

        fread_player_info(d->player_info, fp);
        // read from the db, overwriting anything in the file
        // db_read_player_info(d->player_info);

        fclose(fp);

        /* write a lock file for while they're ingame or being reviewed */
        if (descriptor_list) {
            snprintf(strsave, sizeof(strsave), "%s/%s", ACCOUNTLOCK_DIR, name);
            touchfile(strsave, "lock\n");
        }
    } else {                    /* no account information found, try alternate */

        snprintf(strsave, sizeof(strsave), "%s/%s/.player_info", ALT_ACCOUNT_DIR, name);

        /* try the email directory */
        if ((fp = fopen(strsave, "r")) != NULL) {
            found = TRUE;

            d->player_info = alloc_player_info();
            d->player_info->email = strdup(name);
            fread_player_info(d->player_info, fp);
            // read from the db, overwriting anything in the file
            // db_read_player_info(d->player_info);

            fclose(fp);

            /* write a lock file for while they're ingame or being reviewed */
            if (descriptor_list) {
                snprintf(strsave, sizeof(strsave), "%s/%s", ACCOUNTLOCK_DIR, name);
                touchfile(strsave, "lock\n");
            }
        }
    }

    return found;
}

const char *
application_status(int app)
{
    switch (app) {
    default:
        return "Alive";
    case APPLICATION_APPLYING:
        return "Currently Applying";
    case APPLICATION_REVISING:
        return "Currently Revising";
    case APPLICATION_REVIEW:
        return "Being Reviewed";
    case APPLICATION_ACCEPT:
        return "Accepted";
    case APPLICATION_DENY:
        return "Denied";
    case APPLICATION_DEATH:
        return "Dead";
    case APPLICATION_STORED:
        return "Stored";
    }
}


char *pwdreset_text =
    "The password for your account, %s, has been reset.  When you log\nin with "
    "this new password, you will be asked for a new password.  For\nsecurity "
    "reasons, please do not use the same password you were using before\nthis " "reset."
    "\n\nThe password for your account is:   %s\n";

/*
 * the optional text needs to have two %s's, one for the account name,
 * the other for the password.
 */
void
scramble_and_mail_account_password(PLAYER_INFO * pinfo, char *text)
{
    char arg[MAX_STRING_LENGTH], command[MAX_STRING_LENGTH];
    char *pwdnew, *p;
    bool fTilde = FALSE;
    FILE *fp;
    int i;

    srand(time(0));

    do {
        /*
         * password set to abcdef
         */
        arg[0] = 'a';
        arg[1] = 'b';
        arg[2] = 'c';
        arg[3] = 'd';
        arg[4] = 'e';
        arg[5] = 'f';
        arg[6] = '\0';

        /*
         * No tilde allowed because of player file format.
         */
        pwdnew = (char *) arg;
        for (p = pwdnew; *p != '\0'; p++) {
            if (*p == '~') {
                fTilde = TRUE;
            }
        }
    } while (fTilde == TRUE);

    free(pinfo->password);
    pinfo->password = strdup(pwdnew);


    save_player_info(pinfo);
    return;
}

