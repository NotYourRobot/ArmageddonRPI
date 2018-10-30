/*
 * File: FILES.C
 * Usage: Routines and commands for in-game file manipulation.
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
#include <ctype.h>
#include <string.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/dir.h>

#include "structs.h"
#include "modify.h"
#include "utils.h"
#include "parser.h"
#include "handler.h"
#include "db.h"
#include "db_file.h"
#include "comm.h"
#include "utility.h"

bool
file_exists(char *fname)
{
    FILE *fp;

    if (!(fp = fopen(fname, "r")))
        return FALSE;
    fclose(fp);

    return TRUE;
}

bool
dir_exists(char *dname)
{
    DIR *dp;

    if (!(dp = opendir(dname)))
        return FALSE;
    closedir(dp);

    return TRUE;
}


bool
is_valid_dir(struct char_data * ch, char *dname)
{
    struct stat buf;
    char msg[1000];


    if (!dir_exists(dname)) {
        sprintf(msg, "Directory %s does not exists\n\r", dname);
        send_to_char(msg, ch);
        return FALSE;
    };

    /* and lastely check if it is other-read & other-write */
    if (-1 == stat(ch->desc->path, &buf)) {
        sprintf(msg, "Directory %s is not statable\n\r", dname);
        send_to_char(msg, ch);
        return FALSE;
    }

    if ((buf.st_mode & (S_IROTH)) == 0) {
        sprintf(msg, "Directory %s is not other readable\n\r", dname);
        send_to_char(msg, ch);
        return FALSE;
    }

    if ((buf.st_mode & (S_IWOTH)) == 0) {
        sprintf(msg, "Directory %s is not other writeable\n\r", dname);
        send_to_char(msg, ch);
        return FALSE;
    }

    return TRUE;
}

bool
is_valid_file(struct char_data * ch, char *fname)
{
    struct stat buf;
    char msg[1000];

    if (!file_exists(fname)) {
        sprintf(msg, "File %s does not exist.\n\r", fname);
        send_to_char(msg, ch);
    }

    /* and lastely cHeck if it is other-read & other-write */
    if (-1 == stat(fname, &buf)) {
        sprintf(msg, "File %s is not statable.\n\r", fname);
        send_to_char(msg, ch);
        return FALSE;
    }

    if ((buf.st_mode & (S_IROTH)) == 0) {
        sprintf(msg, "File %s is not other readable.\n\r", fname);
        send_to_char(msg, ch);
        return FALSE;
    }

    if ((buf.st_mode & (S_IWOTH)) == 0) {
        sprintf(msg, "File %s is not other writeable.\n\r", fname);
        send_to_char(msg, ch);
        return FALSE;
    }

    return TRUE;
}

char *
default_path(void)
{

    char cwd[256];
    static char path[256];

    getcwd(cwd, 255);

    sprintf(path, "%s/%s", cwd, IMMORTAL_DIR);

    return path;
}

int
get_file_owner_name(char *owner, char *path, char *file)
{

    char fn[512];
    FILE *fd;

    sprintf(fn, "%s/%s", path, file);

    if ((fd = fopen(fn, "r"))) {
        fscanf(fd, "\"%s", owner);
        *owner = tolower(*owner);
        fclose(fd);
        return 1;
    }

    return 0;
}

int
is_file_owner(struct char_data *ch, char *file)
{

    char owner[256];

    if (ch->player.level == OVERLORD)
        return 1;

    if (get_file_owner_name(owner, ch->desc->path, file))
        return (!strncasecmp(owner, MSTR(ch, name), strlen(MSTR(ch, name))));
    else
        return 1;
}

int
is_safe_filename(char *name)
{

    int i;

    for (i = 0; i < strlen(name); i++)
        if (!isdigit(name[i]) && !isalpha(name[i]) && (name[i] != '-') && (name[i] != '_'))
            return 0;

    return 1;
}

/* returns true if ch->desc->path is under 
   <working directory>/IMMORTAL_DIR  or 
   <working directory>/ASL_DIR */
int
is_safe_path(char *path)
{

    char old_path[256];         /* working directory of the game */
    char new_path[256];         /* resolved ch->desc->path */

    char imm_path[256];         /* <working directory>/IMMORTAL_DIR */

    /* chdir to path, get the resolved path name, and chdir
     * back to original dir */
    getcwd(old_path, 256);
    if (chdir(path) < -1)
        return 0;
    getcwd(new_path, 256);
    chdir(old_path);

    /* is the resolved path under imm_path? */
    sprintf(imm_path, "%s/%s", old_path, IMMORTAL_DIR);
    if (strncmp(new_path, imm_path, strlen(imm_path))) {
        return 0;
    }

    return 1;
}

/* modes: 'x' remove, 'e' edit, 'p' page, 'l' list, 'c' cd */
int
is_safe_file(struct char_data *ch, char *file, char mode)
{

    char action[80];
    char old_path[256];
    char new_path[256];

    char fn[512];

    if (!ch->desc->path) {
        strcpy(ch->desc->path, default_path());
        cprintf(ch, "Restting path.\n\r");
        return 0;
    }

    switch (mode) {
    case ('x'):
        sprintf(action, "remove");
        break;
    case ('e'):
        sprintf(action, "edit");
        break;
    case ('p'):
        sprintf(action, "page");
        break;
    case ('l'):
        sprintf(action, "list");
        break;
    case ('c'):
        sprintf(action, "cd");
        break;
    default:
        warning_logf("is_safe_file: unknown mode %c", mode);
        return 0;
    }

    if (!is_safe_path(ch->desc->path)) {
        warning_logf("%s tried to %s unsafe path %s", MSTR(ch, name), ch->desc->path,
                     file ? file : "");
        return 0;
    }

    if (mode == 'c') {
        strcpy(old_path, ch->desc->path);
        sprintf(new_path, "%s/%s", ch->desc->path, file);

        if (!is_safe_path(new_path)) {
#ifdef SPAM
            warning_logf("%s tried to %s to unsafe directory %s", MSTR(ch, name), action, new_path);
#endif
            return 0;
        }

    }


    if ((mode == 'x') || (mode == 'e') || (mode == 'p'))
        if (!is_safe_filename(file)) {
#ifdef SPAM
            warning_logf("%s tried to %s with unsafe filename %s", MSTR(ch, name), action, file);
#endif
            return 0;
        }

    sprintf(fn, "%s", file);

    if ((mode == 'x') || (mode == 'e'))
        if (!is_file_owner(ch, fn)) {
#ifdef SPAM
            warning_logf("%s tried to %s unowned file %s/%s", MSTR(ch, name), action,
                         ch->desc->path, fn);
#endif
            return 0;
        }

    return 1;
}

void
cmd_edit(struct char_data *ch, const char *argument, Command cmd, int count)
{
    char fn[256], buf[256];
    int old_mask;

    if (!ch->desc)
        return;

    /* I'm putting this back in for editting asl programs.
     * Notice the check of the file name below -Hal 
     * send_to_char ("WHOOOP WHOOOOP, security breach. Sorry no more editing"
     * " files.\n\r",ch );
     */

    argument = one_argument(argument, buf, sizeof(buf));
    if (!*buf) {
        send_to_char("No filename given.\n\r", ch);
        return;
    }

    if (!is_safe_file(ch, buf, 'e')) {
        cprintf(ch, "Illegal path or filename.\n\r");
        return;
    }


    sprintf(fn, "%s/%s", ch->desc->path, buf);

    old_mask = umask((int) 000);
    if (!(ch->desc->editing = fopen(fn, "w"))) {
        send_to_char("Cannot open that file for writing.\n\r", ch);
        return;
    }

    fprintf(ch->desc->editing, "\"%s %d %d %d\"\n\r\n\r", MSTR(ch, name), ch->player.level,
            ch->specials.uid, ch->specials.group);
    umask(old_mask);

    send_to_char("Terminate with a '~'\n\r", ch);
}

void
cmd_rm(struct char_data *ch, const char *argument, Command cmd, int count)
{
    char fn[256], buf[256];

    if (!ch->desc)
        return;

    /*
     * send_to_char ("WHOOOP WHOOOOP, security breach. Sorry no more editing"
     * " files.\n\r",ch );
     */

    argument = one_argument(argument, buf, sizeof(buf));
    if (!*buf) {
        send_to_char("No such file.\n\r", ch);
        return;
    }

    sprintf(fn, "%s/%s", ch->desc->path, buf);


    if (!is_safe_file(ch, buf, 'x')) {
        cprintf(ch, "Illegal path or filename.\n\r");
        return;
    }

    if (!file_exists(fn)) {
        cprintf(ch, "There is no %s file.\n\r", fn);
        return;
    }

    if (!remove(fn)) {
        perror("cannot remove file with cmd_remove");
    }

    send_to_char("File removed.\n\r", ch);

    return;
}


void
cmd_ls(struct char_data *ch, const char *argument, Command cmd, int count)
{

    char list[MAX_STRING_LENGTH];
    char buf[256];
    DIR *dp;
    struct direct *entry;

    if (!ch->desc)
        return;

    if (!is_safe_file(ch, "", 'l')) {
        cprintf(ch, "Illegal directory.\n\r");
        return;
    }

    sprintf(buf, "Directory: %s\n\r", ch->desc->path);
    send_to_char(buf, ch);

    if (!(dp = opendir(ch->desc->path))) {
        send_to_char("No such directory.\n\r", ch);
        return;
    }

    *list = '\0';
    while ((entry = readdir(dp))) {
        if ((*entry->d_name != '\0') && (*entry->d_name != '.')) {
            sprintf(buf, "%s\n\r", entry->d_name);
            strcat(list, buf);
        }
    }

    if (*list == '\0') {
        send_to_char("Your directory is empty.\n\r", ch);
        return;
    }
    closedir(dp);
    send_to_char(list, ch);
}


void
cmd_page(struct char_data *ch, const char *argument, Command cmd, int count)
{
    char fn[256], buf[256];
    char *list;

    if (!ch->desc)
        return;

    argument = one_argument(argument, buf, sizeof(buf));
    if (!*buf) {
        send_to_char("No filename given.\n\r", ch);
        return;
    }

    if (!is_safe_file(ch, buf, 'p')) {
        cprintf(ch, "Illegal path or filename.\n\r");
        return;
    }

    sprintf(fn, "%s/%s", ch->desc->path, buf);

    list = 0;
    file_to_string(fn, &list);
    if (!list) {
        gamelogf("Couldn't page file, file_to_string failed: %s", fn);
        return;
    }
    page_string(ch->desc, list, 1);
    free(list);
}

void
cmd_cd(struct char_data *ch, const char *argument, Command cmd, int count)
{
    char buf[256];
    char dir[256];
    char cwd[512];

    getcwd(cwd, 512);

    argument = one_argument(argument, buf, sizeof(buf));

    if (strlen(buf) == 0) {
        strcpy(ch->desc->path, default_path());
        cprintf(ch, "Changing to users directory.\n\rpath = %s\n\r", ch->desc->path);
    }

    if (!is_safe_file(ch, buf, 'c')) {
        cprintf(ch, "Illegal pathname.\n\r");
        return;
    }

    sprintf(dir, "%s/%s", ch->desc->path, buf);

    if (strlen(dir) > 256) {
        send_to_char("Directory name is getting to long putting you back in" " home\n\r", ch);
        sprintf(ch->desc->path, "%s", default_path());
        return;
    }

    chdir(dir);
    getcwd(dir, 256);
    strcpy(ch->desc->path, dir);
    chdir(cwd);

    send_to_char("Directory changed.\n\r", ch);

    return;
}

void
add_line_to_file(struct descriptor_data *d, char *line)
{
    if (*line == '~') {
        fclose(d->editing);

        d->editing = 0;
        return;
    }

    fputs(line, d->editing);
    fputs("\n", d->editing);
}

