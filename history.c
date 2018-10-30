/*
 * File: HISTORY.C
 * Usage: History / Accounting functions
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
#include <assert.h>
#include <time.h>
#include <stdlib.h>
#include <stdarg.h>
#include <signal.h>
#include <sys/dir.h>

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

/* Reads in a history file for an account */
HISTORY_INFO *
read_history(PLAYER_INFO * pPInfo)
{

    HISTORY_INFO *pHistory = NULL;
    HISTORY_INFO *pEntry = NULL;
    char strsave[MAX_INPUT_LENGTH];
    FILE *fp;

    char buf[MAX_STRING_LENGTH];
    int val = 0;

    sprintf(strsave, "%s/%s/.history", ACCOUNT_DIR, pPInfo->name);

    /* try and open player_info file */
    if ((fp = fopen(strsave, "r")) != NULL) {
        while (!feof(fp)) {
            CREATE(pEntry, HISTORY_INFO, 1);
            fscanf(fp, "%s %d", buf, &val);     /* Not elegant, but it works */
            pEntry->site = strdup(buf);
            pEntry->logins = val;
            fread_to_eol(fp);   /* Only one entry per line, ensure the next line */
            pEntry->next = pHistory;
            pHistory = pEntry;
            pEntry = NULL;
        }

        fclose(fp);
    }
    return pHistory;
}

/* Write out a history file for an account */
void
write_history(PLAYER_INFO * pPInfo, HISTORY_INFO * pHistory)
{
    HISTORY_INFO *pEntry = NULL;
    char strsave[MAX_INPUT_LENGTH];
    FILE *fp;

    sprintf(strsave, "%s/%s/.history", ACCOUNT_DIR, pPInfo->name);
    if ((fp = fopen(strsave, "w")) == NULL) {
        fprintf(stderr, "write_history: fopen");
        perror(strsave);
    } else {
        for (pEntry = pHistory; pEntry; pEntry = pEntry->next) {
            fprintf(fp, "%s %d\n", pEntry->site, pEntry->logins);
        }
    }
    fclose(fp);

    chmod(strsave,
          S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | S_IWGRP | S_IXGRP | S_IROTH | S_IWOTH | S_IXOTH);

    return;
}

/* Updates an account's history */
void
update_history(PLAYER_INFO * pPInfo, char *site)
{
    HISTORY_INFO *pHistory = NULL;
    HISTORY_INFO *pEntry = NULL;

    pHistory = read_history(pPInfo);

    /* Search list for site */
    for (pEntry = pHistory; pEntry; pEntry = pEntry->next) {
        if (!strncmp(pEntry->site, site, strlen(site)))
            break;
    }

    /* Add site if not found */
    if (!pEntry) {
        CREATE(pEntry, HISTORY_INFO, 1);
        pEntry->site = strdup(site);
        pEntry->logins = 0;
        pEntry->next = pHistory;
        pHistory = pEntry;
    }

    /* Update login count for site */
    pEntry->logins++;

    write_history(pPInfo, pHistory);

    /* Clean up memory */
    while (pHistory) {
        pEntry = pHistory;
        pHistory = pHistory->next;
        free(pEntry->site);
        free(pEntry);
    }
}

