/*
 * File: MAIL.C
 * Usage: Special routine in character mail.
 *
 * Copyright (C) 1993, 1994 by Hal Roberts and the creators of DikuMUD.  
 * May Thil al'Sek bring them much warmth in all their manifestations!
 *
 *   /\    ____                    PART OF          __     ____   |\   |
 *  /__\   |___|   /\  /\    /\    __  ___  +--\   |  \   /    \  | \  |
 * /    \  |   \  /  \/  \  /--\  /    [_   |   \  |   |  |    |  |  \ |
 * \    /  |   /  \      /  \  /  \_]  [__  |___/  |__/   \____/  |   \|
 * ---------------------------------------------------------------------
 */

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <sys/dir.h>

#include "constants.h"
#include "structs.h"
#include "comm.h"
#include "modify.h"
#include "parser.h"
#include "utils.h"
#include "handler.h"
#include "db.h"
#include "utility.h"

/* start Hal's new mail routines */

/* start mail declarations */

/* How to Add a Post Office - 
   - bump NUM_POSTS up one 
   - add an entry to the post_offices according to the format given 
   below
   - *IMPORTANT* add a directory under the '../lib/post' hierarchy with
   name of your post office (trader, flint, etc.) 
   - underneath your newly created directory, create one directory
   for each post office, including the one you are creating but not
   transit directory
   - add a directory with the name of your post office under the 
   'post/transit/' hierarchy.
   - if the last three steps are not clear, look at the existing
   'lib/post/' hierarchy to get a feel for what I am doing.  if it
   is still not clear, ask me (Hal) for help
 */

struct mail_data                /* struct for reading/writing mail to file */
{
    int obj_num;
    int clevel;                 /* used to keep track of objects in objects */
    int day;                    /* day, month, and year of arrival */
    int month;
    int year;
    int to_post;
    int value[6];
    int note;
    struct mail_data *next;
};

struct post_data
{
    int mob_num;                /* vnum of post master */
    char *post;                 /* name of post office */
    char *city;                 /* name of city */
    int speed[NUM_POSTS];       /* hack alert: change to malloc with files */
    int reliability[NUM_POSTS]; /* x/100 that delivery will work */
    int cost[NUM_POSTS];        /* sid/stone to deliver mail */
};

struct post_data post_offices[] = {
    /* format - {mob num, post name, city name, list of delivery times, 
     * list of reliabilities, list of prices} */
    /* 0  */
    {0, "transit", "transit",
     {-1, -1, -1, -1},
     {-1, -1, -1, -1},
     {-1, -1, -1, -1}},

    /* 1  */
    {50033, "trader", "allanak",
     {-1, 0, 5, 4, 2},
     {-1, 100, 90, 90, 98},
     {-1, 1, 10, 8, 5}},

    /* 2  */
    {20115, "flint", "tuluk",
     {-1, 5, 0, 4, 6},
     {-1, 90, 100, 90, 88},
     {-1, 10, 1, 8, 12}},

    /* 3  */
    {64007, "luir", "luir",
     {-1, 4, 4, 0, 5},
     {-1, 92, 92, 100, 90},
     {-1, 8, 8, 1, 10}},

    /* 4  */
    {48001, "redstorm", "redstorm",
     {-1, 2, 8, 5, 0},
     {-1, 98, 88, 90, 100},
     {-1, 5, 12, 10, 1}}
};

/* end mail declarations */

/* begin mail functions */

/* find_post returns the index of a post office in the post_office struct
   given either the post office name, the city name, or the vnum of
   the post master.  Only one of the above need be given, and the function
   will return the first post office that it finds with the given
   city.  Returns -1 on failure */
int
find_post(char *arg, int mob_num)
{
    int i;
    int len;

    for (i = 0; i < NUM_POSTS; i++) {
        to_lower(arg);
        len = strlen(arg);
        if (!strncmp(arg, post_offices[i].post, len) || !strncmp(arg, post_offices[i].city, len)
            || (mob_num == post_offices[i].mob_num))
            break;
    }

    if (i == NUM_POSTS)
        return -1;
    else
        return i;
}

/* file_to_mail reads the next mail info in fd into a mail_data struct
   and returns the result. */
struct mail_data *
file_to_mail(FILE * fd)
{
    int n;
    struct mail_data *mail_obj = (struct mail_data *) malloc(sizeof *mail_obj);

    if (mail_obj == NULL) {
        gamelog("couldn't allocate memory in file_to_mail");
        exit(0);
    };

    if (fscanf(fd, "%d ", &mail_obj->obj_num) != EOF) {
        fscanf(fd, "%d ", &mail_obj->clevel);
        fscanf(fd, "%d ", &mail_obj->day);
        fscanf(fd, "%d ", &mail_obj->month);
        fscanf(fd, "%d ", &mail_obj->year);
        fscanf(fd, "%d ", &mail_obj->to_post);
        for (n = 0; n < 6; n++)
            fscanf(fd, "%d ", &mail_obj->value[n]);
        fscanf(fd, "%d\n", &mail_obj->note);
        return mail_obj;
    } else
        return 0;
}

/* mail_to_file appends the given piece of mail to the file named 
   filename */
void
mail_to_file(struct mail_data *mail, FILE * fd)
{
    int i;

    fprintf(fd, "%d ", mail->obj_num);
    fprintf(fd, "%d ", mail->clevel);
    fprintf(fd, "%d ", mail->day);
    fprintf(fd, "%d ", mail->month);
    fprintf(fd, "%d ", mail->year);
    fprintf(fd, "%d ", mail->to_post);
    for (i = 0; i < 6; i++)
        fprintf(fd, "%d ", mail->value[i]);
    fprintf(fd, "%d\n", mail->note);

}

/* obj_to_file converts an obj_data object to a mail_data object and sends 
   the mail_data object to a file */
void
obj_to_file(struct obj_data *obj, int clev, int dayp, int to_post, FILE * fd)
{
    int i, day, month, year;
    struct mail_data mail;

    day = time_info.day;
    month = time_info.month;
    year = time_info.year;

    /* set the time of arrival for the mail */
    day = day + dayp;

    if (day > 231) {
        day = day - 231;
        if (++month > 3) {
            month = 1;
            year++;
        }
    }

    mail.obj_num = obj_index[obj->nr].vnum;
    mail.clevel = clev;
    mail.day = day;
    mail.month = month;
    mail.year = year;
    mail.to_post = to_post;
    for (i = 0; i < 6; i++)
        mail.value[i] = obj->obj_flags.value[i];
    if (obj->obj_flags.type == ITEM_NOTE) {
        save_note(obj);
        mail.note = 1;
    } else
        mail.note = 0;

    mail_to_file(&mail, fd);

    return;
}

/* mail_expired returns true if mail has expired (mail expires 
   MAIL_EXPIRATION_TIME days after arrival).   Else, returns
   false.  */
int
mail_expired(struct mail_data *mail)
{
    int exday = mail->day;
    int exmonth = mail->month;
    int exyear = mail->year;

    exday += MAIL_EXPIRATION_TIME;

    if (exday > 231) {
        exday = exday - 231;
        if (++exmonth > 3) {
            exmonth = 1;
            exyear++;
        }
    }

    return ((time_info.year > exyear) || ((time_info.year == exyear) && (time_info.month > exmonth))
            || ((time_info.year == exyear) && (time_info.month == exmonth)
                && (time_info.day >= exday)));
}

/* purge_old_mail searches the given post_office for any mail that has
   expired and purges it.  If the mail is a note item with a notefile,
   purge_old_mail will remove the associated note file. */
void
purge_old_mail(int post)
{
    char dirname[256];
    char tempfile[256];
    char filename[256];
    struct direct *dp;
    struct mail_data *mail;
    FILE *fd, *mfd;
    DIR *dfd;

    /* mail can only expire if it is in the post office's receiving bin */
    sprintf(dirname, "post/%s/%s", post_offices[post].post, post_offices[post].post);

    if ((dfd = opendir(dirname)) == NULL) {
        gamelogf("Error opening mail directory %s: %s", dirname, strerror(errno));
        return;
    }

    /* loop through files in the [postoffice]/[postoffice] dir */
    while ((dp = readdir(dfd)) != NULL) {
        if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
            continue;
        if (strlen(dirname) + strlen(dp->d_name) + 2 > sizeof(filename))
            gamelog("Mail file name too long");
        else
            /* move mail to temp file and copy back into original file
             * those pieces that have not expired. */
        {
            sprintf(filename, "%s/%s", dirname, dp->d_name);
            sprintf(tempfile, "%s/~%s", dirname, dp->d_name);
            rename(filename, tempfile);
            if ((fd = fopen(tempfile, "r")) == NULL) {
                gamelog("Problem opening mail file in deliver_mail_post");
                continue;
            }

            while ((mail = file_to_mail(fd))) {
                if (!mail_expired(mail)) {
                    mfd = fopen(filename, "a");
                    mail_to_file(mail, mfd);
                    fclose(mfd);
                } else if ((mail->note) && (mail->value[1] > 0)) {
                    sprintf(filename, "notes/note%d", mail->value[1]);
                    remove(filename);
                }
            }
            fclose(fd);
            remove(tempfile);
        }
    }

    closedir(dfd);

    return;
}

/* mail_ready returns true if the given mail is ready to moved from 
   transit to the appropriate post office. */
int
mail_ready(struct mail_data *mail)
{
    return ((time_info.year > mail->year)
            || ((time_info.year == mail->year) && (time_info.month > mail->month))
            || ((time_info.year == mail->year) && (time_info.month == mail->month)
                && (time_info.day >= mail->day)));
}

/* deliver_mail_post delivers any mail that is due to to_post into
   to_post's receiving directory. */
void
deliver_mail_post(int to_post)
{
    char origdir[256];
    char destdir[256];
    char tempfile[256];
    char origfile[256];
    char destfile[256];
    struct direct *dp;
    struct mail_data *mail;
    FILE *fd, *mfd;
    DIR *dfd;


    sprintf(destdir, "post/%s/%s", post_offices[to_post].post, post_offices[to_post].post);
    sprintf(origdir, "post/transit/%s", post_offices[to_post].post);

    if ((dfd = opendir(origdir)) == NULL) {
        gamelogf("Error opening mail directory %s: %s", origdir, strerror(errno));
        return;
    }

    /* loop through files in the transit/[postoffice] dir */
    while ((dp = readdir(dfd)) != NULL) {
        if (*dp->d_name == '.')
            continue;
        if (strlen(destdir) + strlen(dp->d_name) + 2 > sizeof(origfile))
            gamelog("Mail file name too long");
        else
            /* move file to temp file and move each mail either back
             * into the transit dir or into the to_post receiving dir */
        {
            sprintf(destfile, "%s/%s", destdir, dp->d_name);
            sprintf(origfile, "%s/%s", origdir, dp->d_name);
            sprintf(tempfile, "%s/~%s", origdir, dp->d_name);
            rename(origfile, tempfile);
            if ((fd = fopen(tempfile, "r")) == NULL) {
                gamelog("Problem opening mail file in deliver_mail_post");
                continue;
            }

            while ((mail = file_to_mail(fd))) {
                if (mail_ready(mail))
                    mfd = fopen(destfile, "a");
                else
                    mfd = fopen(origfile, "a");
                mail_to_file(mail, mfd);
                fclose(mfd);
            }
            fclose(fd);
            remove(tempfile);
        }
    }

    closedir(dfd);

    return;
}

/* append concats origfile to destfile */
void
append(char *origfile, char *destfile)
{
    FILE *ofd, *dfd;
    char c;

    if ((ofd = fopen(origfile, "r")) == NULL) {
        gamelog("Problem opening mail file in append (mail)");
        return;
    }
    if ((dfd = fopen(destfile, "a")) == NULL) {
        gamelog("Problem opening mail file in append (mail)");
        return;
    }

    while ((c = getc(ofd)) != EOF)
        putc(c, dfd);

    fclose(ofd);
    fclose(dfd);

    return;
}

/* remove_mail_post removes all outgoing mail from to_post's 
   subdirectories */
void
remove_mail_post(int to_post, int from_post)
{
    char dirname[256];
    char origfile[256];
    char destfile[256];
    struct direct *dp;
    DIR *dfd;
    int delivery_success;

    delivery_success = number(0, 100) < post_offices[from_post].reliability[to_post];

    sprintf(dirname, "post/%s/%s", post_offices[from_post].post, post_offices[to_post].post);
    if ((dfd = opendir(dirname)) == NULL) {
        gamelogf("Error opening mail directory %s: %s", dirname, strerror(errno));
        return;
    }

    /* loop through [from_post]/[to_post] dir */
    while ((dp = readdir(dfd)) != NULL) {
        if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
            continue;
        if (strlen(dirname) + strlen(dp->d_name) + 2 > sizeof(origfile))
            gamelog("Mail file name too long");
        else {
            sprintf(origfile, "%s/%s", dirname, dp->d_name);
            sprintf(destfile, "post/transit/%s/%s", post_offices[to_post].post, dp->d_name);
            if (delivery_success)
                append(origfile, destfile);
            remove(origfile);
        }
    }

    closedir(dfd);

    return;
}

/* deliver_mail delivers virtual mail into the various post offices */
void
deliver_mail(void)
{
    static int from_post = 1;
    static int to_post = 1;
    struct timeval start;

    perf_enter(&start, "deliver_mail()");
    /* cycle the posts from/to which you are delivering */
    if ((from_post + 1) < NUM_POSTS)
        from_post++;
    else {
        from_post = 1;
        if ((to_post + 1) < NUM_POSTS)
            to_post++;
        else
            to_post = 1;
    }

    /* I wanna keep this in for a while to make sure that 
     * sprintf(buf, "Delivering mail to %s", post_offices[to_post].post);
     * gamelog(buf);
     * 
     * -- either purge old mail from the dir or move mail from the dir into
     * the transit dir */
    if (to_post == from_post)
        purge_old_mail(from_post);
    else
        remove_mail_post(to_post, from_post);

    /* deliver mail to the post from which the mail was picked up */
    deliver_mail_post(from_post);

    perf_exit("deliver_mail()", start);

    return;
}

/* drop_mail drops mail into the appropriate origin file */
int
drop_mail(struct char_data *ch, struct char_data *npc, struct obj_data *mail, char *recip,
          int to_post, int from_post, int clevel)
{
    int day;
    static int vary;
    char buf[256];
    char name[256];
    char filename[256];
    struct obj_data *obj;
    static FILE *fd;

    /* if the object is an inventory object, open a file for appending */
    if (!mail->in_obj) {
        to_lower(recip);
        sprintf(filename, "post/%s/%s/%s", post_offices[from_post].post, post_offices[to_post].post,
                recip);

        if (!(fd = fopen(filename, "a"))) {
            gamelog("Post master lost his box (problem opening mail file)");
            return 0;
        }

        vary = number(0, 2);
    }

    /* append mail to file */
    day = (post_offices[from_post].speed[to_post] + vary);
    obj_to_file(mail, clevel, day, to_post, fd);

    /* recursively store contained objects, keeping track of the
     * level of recursion in clevel */
    if (mail->contains) {
        for (obj = mail->contains; obj; obj = obj->next_content)
            drop_mail(ch, npc, obj, recip, to_post, from_post, clevel - 1);
    }

    if (!mail->in_obj) {
        first_name(MSTR(ch, name), name);
        if (to_post == from_post)
            sprintf(buf, "tell %s Very well, I'll hold %s for your friend.", name,
                    OSTR(mail, short_descr));
        else
            sprintf(buf, "tell %s I'll give %s to the next courier that comes by.", name,
                    OSTR(mail, short_descr));
        parse_command(npc, buf);

        obj_from_char(mail);
        extract_obj(mail);
        fclose(fd);
    }

    return 1;
}

/* char_pays_post returns true if the character successfully pays
   the post fee */
int
char_pays_post(struct char_data *ch, struct char_data *npc, struct obj_data *obj, int to_post,
               int from_post)
{
    char name[256];
    char buf[MAX_STRING_LENGTH];
    int cost;

    cost = post_offices[from_post].cost[to_post] * (obj->obj_flags.weight + 1);

    if (ch->points.obsidian < cost) {
        first_name(MSTR(ch, name), name);
        sprintf(buf,
                "tell %s I'd have to charge you %d coins for that.  "
                "You don't have enough money.", name, cost);
        parse_command(npc, buf);
        return 0;
    } else {
        ch->points.obsidian -= cost;
        npc->points.obsidian += cost;
        return 1;
    }
}


/* leave_mail parses the command line and calls functions to charge 
   the character for delivery and to drop the mail in the appropriate
   file */
int
leave_mail(struct char_data *ch, struct char_data *npc, const char *arg, int from_post)
{
    char name[256];
    char buf[256];
    char mail_name[256];
    char recip[256];
    char dest[256];
    int to_post;
    struct obj_data *mail;

    arg = one_argument(arg, buf, sizeof(buf));

    if (strcmp(buf, "mail"))
        return (0);

    arg = one_argument(arg, mail_name, sizeof(mail_name));

    if (!*mail_name) {
        first_name(MSTR(ch, name), name);
        sprintf(buf, "tell %s What do you want to mail ?", name);
        parse_command(npc, buf);
        return 1;
    }

    if (!(mail = get_obj_in_list_vis(ch, mail_name, ch->carrying))) {
        send_to_char("You can't mail what you don't have.\n\r", ch);
        return 1;
    }

    arg = two_arguments(arg, recip, sizeof(recip), dest, sizeof(dest));

    if (!strlen(recip)) {
        first_name(MSTR(ch, name), name);
        sprintf(buf, "tell %s Who do you want to leave the mail for ?", name);
        parse_command(npc, buf);
        return 1;
    }

    if (strlen(dest)) {
        if ((to_post = find_post(dest, -1)) < 0) {
            first_name(MSTR(ch, name), name);
            sprintf(buf, "tell %s I only send mail to places on the map.", name);
            parse_command(npc, buf);
            parse_command(npc, "emote points to the map on the wall.");
            return 1;
        }
    } else
        to_post = from_post;

    if (char_pays_post(ch, npc, mail, to_post, from_post))
        return (drop_mail(ch, npc, mail, recip, to_post, from_post, -1));
    else
        return 1;

}

/* assess_mail parses the command line and reports the cost and speed of
   delivering the piece of mail. */
int
assess_mail(struct char_data *ch, struct char_data *npc, const char *arg, int from_post)
{
    char name[256];
    char buf[256];
    char mail_name[256];
    char recip[256];
    char dest[256];
    int to_post;
    int cost;
    struct obj_data *mail;

    arg = one_argument(arg, buf, sizeof(buf));

    if (strcmp(buf, "mail"))
        return (0);

    arg = one_argument(arg, mail_name, sizeof(mail_name));

    if (!*mail_name) {
        first_name(MSTR(ch, name), name);
        sprintf(buf, "tell %s What do you want to mail ?", name);
        parse_command(npc, buf);
        return 1;
    }

    if (!(mail = get_obj_in_list_vis(ch, mail_name, ch->carrying))) {
        send_to_char("You can't mail what you don't have.\n\r", ch);
        return 1;
    }

    arg = two_arguments(arg, recip, sizeof(recip), dest, sizeof(dest));

    if (!strlen(recip)) {
        first_name(MSTR(ch, name), name);
        sprintf(buf, "tell %s Who do you want to leave the mail for ?", name);
        parse_command(npc, buf);
        return 1;
    }

    if (strlen(dest)) {
        if ((to_post = find_post(dest, -1)) < 0) {
            first_name(MSTR(ch, name), name);
            sprintf(buf, "tell %s I only send mail to places on the map.", name);
            parse_command(npc, buf);
            parse_command(npc, "emote points to the map on the wall.");
            return 1;
        }
    } else
        to_post = from_post;

    cost = post_offices[from_post].cost[to_post] * (mail->obj_flags.weight + 1);
    first_name(MSTR(ch, name), name);
    sprintf(buf, "tell %s It will cost %d sid and take %d days to send %s.", name, cost,
            post_offices[from_post].speed[to_post], OSTR(mail, short_descr));
    parse_command(npc, buf);

    return 1;
}

/* address_mail sticks a note giving the name, post office, and city
   of the recipient under the address_note edesc */
void
address_mail(struct mail_data mail_obj, char *recip, char *address)
{
    char buf[256];
    char post[256];
    char city[256];

    strcpy(post, post_offices[mail_obj.to_post].post);
    strcpy(city, post_offices[mail_obj.to_post].city);

    recip[0] = toupper(recip[0]);
    post[0] = toupper(post[0]);
    city[0] = toupper(city[0]);

    sprintf(buf, "This mail is for %s at %s in %s.", recip, post, city);

    strcpy(address, buf);

    return;
}

/* mail_to_player moves mail from a file to the player */
void
mail_to_player(FILE * fd, struct char_data *ch, char *recip, char *filename, char *address)
{
    int n;
    struct mail_data *mail_obj;
    struct obj_data *obj;
    struct obj_data *head_object;


    while ((mail_obj = file_to_mail(fd))) {
        if (!(obj = read_object(mail_obj->obj_num, VIRTUAL))) {
            gamelog("Unable to create mail object");
            free(mail_obj);
            continue;
        }

        for (n = 0; n < 6; n++)
            obj->obj_flags.value[n] = mail_obj->value[n];

        if ((obj->obj_flags.type == ITEM_NOTE) && mail_obj->value[1])
            read_note(obj);

        /* if item is inventory item, move it to character, else move
         * it to the last container of the appropriate level moved
         * to the character */
        if (mail_obj->clevel >= -1) {
            address_mail(*mail_obj, recip, address);
            obj_to_char(obj, ch);
        } else {
            head_object = ch->carrying;
            while ((mail_obj->clevel < -2) && head_object) {
                head_object = head_object->contains;
                mail_obj->clevel++;
            }
            if (head_object)
                obj_to_obj(obj, head_object);
            else {
                obj_to_char(obj, ch);
                shhlog("CRAP!");
            }
        }
        free(mail_obj);
    }

    remove(filename);
}

struct obj_list
{
    struct obj_data *obj;
    struct obj_list *next;
};

/* mail_weight returns the weight of the mail in the given file */
int
mail_weight(FILE * fd, char *recip, char *filename)
{
    int n;
    struct mail_data *mail_obj;
    struct obj_data *obj;
    struct obj_data *head_object;
    struct obj_list *new, *list;
    int weight = 0;


    list = (struct obj_list *) malloc(sizeof *list);
    list->next = NULL;

    while ((mail_obj = file_to_mail(fd))) {
        if (!(obj = read_object(mail_obj->obj_num, VIRTUAL))) {
            gamelog("Unable to create mail object");
            free(mail_obj);
            continue;
        }

        for (n = 0; n < 6; n++)
            obj->obj_flags.value[n] = mail_obj->value[n];

        if ((obj->obj_flags.type == ITEM_NOTE) && mail_obj->value[1])
            read_note(obj);

        /* if item is inventory item, move it to the list, else move
         * it to the last container of the appropriate level moved
         * to the list */
        if (mail_obj->clevel >= -1) {
            new = (struct obj_list *) malloc(sizeof *new);
            new->obj = obj;
            new->next = list->next;
            list->next = new;
        } else {
            head_object = list->next->obj;
            while ((mail_obj->clevel < -2) && head_object) {
                head_object = head_object->contains;
                mail_obj->clevel++;
            }
            if (head_object)
                obj_to_obj(obj, head_object);
            else
                shhlog("CRAP!");
        }
    }

    /* figure up the weight and free the obj list */
    while ((new = list->next)) {
        weight += calc_object_weight(new->obj);
        list->next = new->next;
        extract_obj(new->obj);
        free(new);
    }
    free(list);

    gamelogf("mail weight = %d", weight);
    return weight;
}

/* is_mail_courier returns true if the character is a courier at 
   at_post.  else, it returns false. For now it always returns false
   since manual delivery is not quite up to snuff.  */
int
is_mail_courier(struct char_data *ch, int at_post)
{
    if (!stricmp(MSTR(ch, name), "bram"))
        return 1;
    else if ((ch->player.tribe == 28) && (IS_SET(ch->specials.act, CFL_CLANLEADER)))
        return 1;
    else
        return 0;
}

/* get_mail_bundle gets all the mail intended for the given city 
   and moves it to the courier.  HACK - should change it to loading
   either by weight or one object at a time. */
int
get_courier_mail(struct char_data *ch, struct char_data *npc, int at_post, int to_post)
{
    int got_mail = 0, over_wt = 1;
    int weight, cost;
    char address[256];
    char name[256];
    char dirname[256];
    char buf[256];
    char filename[256];
    struct direct *dp;
    FILE *fd;
    DIR *dfd;

    if (!is_mail_courier(ch, at_post)) {
        first_name(MSTR(ch, name), name);
        sprintf(buf, "tell %s You are not one of my couriers.", name);
        parse_command(npc, buf);
        return 1;
    }

    sprintf(dirname, "post/%s/%s", post_offices[at_post].post, post_offices[to_post].post);
    if ((dfd = opendir(dirname)) == NULL) {
        gamelogf("Error opening mail directory %s: %s", dirname, strerror(errno));
        return (0);
    }

    while ((dp = readdir(dfd)) && !got_mail) {
        if (!strcmp(dp->d_name, ".") || !strcmp(dp->d_name, ".."))
            continue;
        if (strlen(dirname) + strlen(dp->d_name) + 2 > sizeof(filename))
            gamelog("Mail file name too long");
        else {
            sprintf(filename, "%s/%s", dirname, dp->d_name);
            if (!(fd = fopen(filename, "r"))) {
                gamelog("Problem opening existing mail file");
                continue;
            }

            weight = mail_weight(fd, dp->d_name, filename);
            if ((weight + IS_CARRYING_W(ch)) <= CAN_CARRY_W(ch)) {
                fclose(fd);
                fd = fopen(filename, "r");
                mail_to_player(fd, ch, dp->d_name, filename, address);
                cost = 3 * (post_offices[at_post].cost[to_post] * weight) / 4;
                ch->points.obsidian += cost;
                got_mail = 1;
            } else
                over_wt = 1;

            fclose(fd);
        }
    }

    closedir(dfd);

    if (got_mail) {
        first_name(MSTR(ch, name), name);
        sprintf(buf, "tell  %s %s", name, address);
        parse_command(npc, buf);
        sprintf(buf, "emote hands ~%s a bundle of mail and some coins.", name);
        parse_command(npc, buf);
    } else {
        first_name(MSTR(ch, name), name);
        if (!over_wt)
            sprintf(buf, "tell %s I don't have any mail headed there.", name);
        else
            sprintf(buf,
                    "tell %s I don't have any mail headed there light " "enough for you to carry.",
                    name);
        parse_command(npc, buf);
    }

    return 1;
}

/* get_personal_mail will move all the mail for the given player 
   (as evidenced by being in a file of his name) in the receiving dir
   of at_post */
int
get_personal_mail(struct char_data *ch, struct char_data *npc, int at_post)
{

    char keywords[512] = "";
    char name[256];
    char filename[256];
    char buf[256];
    char *hyphen;
    const char *tmp = keywords;
    FILE *fd;
    int got_mail = 0;

    strcat(keywords, MSTR(ch, name));
    strcat(keywords, " ");
    if (ch->player.extkwds)
        strcat(keywords, ch->player.extkwds);
    while ((hyphen = strstr(keywords, "-")))
        *hyphen = ' ';
    for ((tmp = one_argument(tmp, name, sizeof(name))); strlen(name) > 0;
         tmp = one_argument(tmp, name, sizeof(name))) {
        to_lower(name);
        sprintf(filename, "post/%s/%s/%s", post_offices[at_post].post, post_offices[at_post].post,
                name);

        if ((fd = fopen(filename, "r"))) {
            mail_to_player(fd, ch, name, filename, buf);
            fclose(fd);
            got_mail = 1;
        }
    }

    if (!got_mail) {
        first_name(MSTR(ch, name), name);
        sprintf(buf, "tell %s I don't have any mail for you.", name);
        parse_command(npc, buf);
        return 1;
    }

    first_name(MSTR(ch, name), name);
    sprintf(buf, "tell %s Here you go.", name);
    parse_command(npc, buf);
    sprintf(buf, "emote hands ~%s some mail.", name);
    parse_command(npc, buf);

    return 1;

}

/* get_mail parses the command line and calls functions either to get
   the personal mail of a character or to get a mail bundle for a 
   certain destination */
int
get_mail(struct char_data *ch, struct char_data *npc, const char *arg, int post)
{
    char name[256];
    char buf[256];
    char dest[256];
    int to_post = 0;

    arg = one_argument(arg, buf, sizeof(buf));

    if (strcmp(buf, "mail"))
        return (0);

    arg = one_argument(arg, dest, sizeof(dest));

    if (!strlen(dest))
        return (get_personal_mail(ch, npc, post));

    if ((to_post = find_post(dest, -1)) < 0) {
        first_name(MSTR(ch, name), name);
        sprintf(buf, "tell %s I only send mail to places on the map.", name);
        parse_command(npc, buf);
        parse_command(npc, "emote points to the map on the wall.");
        return 1;
    }
    return (get_courier_mail(ch, npc, post, to_post));
}

/* post_master is the mail special proper, which calls either get_mail
   leave_mail, or assess_mail after finding which post_master it was
   called by */
int
post_master(struct char_data *ch, int cmd, char *arg, struct char_data *npc_in,
            struct room_data *rm_in, struct obj_data *obj_in)
{
    int post;
    char buf[MAX_STRING_LENGTH];
    char *ignore = strdup("ignore_string");

    if ((post = find_post(ignore, npc_index[npc_in->nr].vnum)) < -1) {
        sprintf(buf, "Woops!  Post master #%d can't find himself !", npc_index[npc_in->nr].vnum);
        gamelog(buf);
        return (0);
    }

    switch (cmd) {
    case (CMD_LEAVE):
        return (leave_mail(ch, npc_in, arg, post));
    case (CMD_GET):
        return (get_mail(ch, npc_in, arg, post));
    case (CMD_ASSESS):
        return (assess_mail(ch, npc_in, arg, post));
    default:
        return (0);
    }
}

/* end mail system */

