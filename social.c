/*
 * File: SOCIAL.C
 * Usage: Commands involving social interaction.
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
#include <stdlib.h>

#include "structs.h"
#include "utils.h"
#include "comm.h"
#include "parser.h"
#include "handler.h"
#include "db.h"
#include "skills.h"
#include "guilds.h"
#include "modify.h"

void parse_string(char *input, char *output, struct char_data *ch1, struct char_data *ch2,
                  struct char_data *to);
int action(int cmd);
char *fread_action(FILE * fl);


struct social_messg
{
    int act_nr;
    int hide;
    int min_victim_position;    /* Position of victim */

    /* No argument was supplied */
    char *char_no_arg;
    char *others_no_arg;

    /* An argument was there, and a victim was found */
    char *char_found;           /* if NULL, read no further, ignore args */
    char *others_found;
    char *vict_found;

    /* An argument was there, but no victim was found */
    char *not_found;

    /* The victim turned out to be the character */
    char *char_auto;
    char *others_auto;

    /* The victim turned out to be an object */
    char *others_object;
    char *char_object;
}
 *soc_mess_list = 0;



struct pose_type
{
    int level;                  /* minimum level for poser */
    char *poser_msg[4];         /* message to poser        */
    char *room_msg[4];          /* message to room         */
}
pose_messages[MAX_MESSAGES];

static int list_top = -1;




char *
fread_action(FILE * fl)
{
    char buf[MAX_STRING_LENGTH], *rslt;

    fgets(buf, MAX_STRING_LENGTH, fl);
    if (feof(fl)) {
        gamelog("Fread_action - unexpected EOF.");
        exit(0);
    }

    if (*buf == '#')
        return (0);

    *(buf + strlen(buf) - 1) = '\0';
    CREATE(rslt, char, strlen(buf) + 1);
    strcpy(rslt, buf);
    return (rslt);
}

void
unboot_social_messages(void)
{
    int i;
    for (i = 0; i <= list_top; i++) {
        if (soc_mess_list[i].others_object)
            free(soc_mess_list[i].others_object);

        if (soc_mess_list[i].char_object)
            free(soc_mess_list[i].char_object);

        if (soc_mess_list[i].char_no_arg)
            free(soc_mess_list[i].char_no_arg);

        if (soc_mess_list[i].others_no_arg)
            free(soc_mess_list[i].others_no_arg);

        if (soc_mess_list[i].char_found)
            free(soc_mess_list[i].char_found);

        if (soc_mess_list[i].others_found)
            free(soc_mess_list[i].others_found);

        if (soc_mess_list[i].vict_found)
            free(soc_mess_list[i].vict_found);

        if (soc_mess_list[i].not_found)
            free(soc_mess_list[i].not_found);

        if (soc_mess_list[i].char_auto)
            free(soc_mess_list[i].char_auto);

        if (soc_mess_list[i].others_auto)
            free(soc_mess_list[i].others_auto);
    };

    free(soc_mess_list);
    soc_mess_list = NULL;
    list_top = -1;
};

void
boot_social_messages(void)
{
    FILE *fl;
    int tmp, hide, min_pos;

    if (!(fl = fopen(SOCMESS_FILE, "r"))) {
        gamelogf("Couldn't open %s, bailing!", SOCMESS_FILE);
        exit(0);
    }
    for (;;) {
        fscanf(fl, " %d ", &tmp);
        if (tmp < 0)
            break;
        fscanf(fl, " %d ", &hide);
        fscanf(fl, " %d \n", &min_pos);

        /* alloc a new cell */
        if (!soc_mess_list) {
            CREATE(soc_mess_list, struct social_messg, 1);
            list_top = 0;
        } else {
            if (!(soc_mess_list = (struct social_messg *)
                  realloc(soc_mess_list, sizeof(struct social_messg) * (++list_top + 1)))) {
                perror("boot_social_messages. realloc");
                exit(1);
            }
        }

        /* read the stuff */
        soc_mess_list[list_top].act_nr = tmp;
        soc_mess_list[list_top].hide = hide;
        soc_mess_list[list_top].min_victim_position = min_pos;

        /* object to room */
        soc_mess_list[list_top].others_object = fread_action(fl);
        /* object to char */
        soc_mess_list[list_top].char_object = fread_action(fl);

        soc_mess_list[list_top].char_no_arg = fread_action(fl);
        soc_mess_list[list_top].others_no_arg = fread_action(fl);

        soc_mess_list[list_top].char_found = fread_action(fl);

        /* if no char_found, the rest is to be ignored */
        if (!soc_mess_list[list_top].char_found) {
            soc_mess_list[list_top].others_found = 0;
            soc_mess_list[list_top].vict_found = 0;
            soc_mess_list[list_top].not_found = 0;
            soc_mess_list[list_top].char_auto = 0;
            soc_mess_list[list_top].others_auto = 0;
            continue;
        };

        soc_mess_list[list_top].others_found = fread_action(fl);
        soc_mess_list[list_top].vict_found = fread_action(fl);

        soc_mess_list[list_top].not_found = fread_action(fl);

        soc_mess_list[list_top].char_auto = fread_action(fl);

        soc_mess_list[list_top].others_auto = fread_action(fl);

    }
    fclose(fl);
}




int
find_action(int cmd)
{
    int bot, top, mid;

    bot = 0;
    top = list_top;

    if (top < 0)
        return (-1);

    for (;;) {
        mid = (bot + top) / 2;

        if (soc_mess_list[mid].act_nr == cmd)
            return (mid);
        if (bot == top)
            return (-1);

        if (soc_mess_list[mid].act_nr > cmd)
            top = --mid;
        else
            bot = ++mid;
    }
}





void
cmd_action(struct char_data *ch, const char *argument, Command cmd, int count)
{
    int act_nr;
    char buf[MAX_INPUT_LENGTH];
    struct social_messg *action;
    struct char_data *vict, *temp_char;
    struct obj_data *obj = 0;

    if ((act_nr = find_action(cmd)) < 0) {
        send_to_char("That action is not supported.\n\r", ch);
        return;
    }

    action = &soc_mess_list[act_nr];

    if (action->char_found)
        argument = one_argument(argument, buf, sizeof(buf));
    else
        *buf = '\0';

    if (!*buf) {
        if (action->char_no_arg)
            send_to_char(action->char_no_arg, ch);
        send_to_char("\n\r", ch);
        if (action->others_no_arg)
            act(action->others_no_arg, action->hide, ch, 0, 0, TO_ROOM);

        return;
    }

    if (!(vict = get_char_room_vis(ch, buf))) {

        generic_find(buf, FIND_OBJ_ROOM | FIND_OBJ_EQUIP | FIND_OBJ_INV, ch, &temp_char, &obj);

        if (obj) {
            if (action->char_object)
                act(action->char_object, 0, ch, obj, 0, TO_CHAR);
            if (action->others_object)
                act(action->others_object, action->hide, ch, obj, 0, TO_ROOM);
        } else {
            if (action->not_found)
                send_to_char(action->not_found, ch);
            send_to_char("\n\r", ch);
        }
    } else if (vict == ch) {
        if (action->char_auto)
            send_to_char(action->char_auto, ch);
        send_to_char("\n\r", ch);
        if (action->others_auto)
            act(action->others_auto, action->hide, ch, 0, 0, TO_ROOM);
    } else {
        if (GET_POS(vict) < action->min_victim_position) {
            act("$N is not in a proper position for that.", FALSE, ch, 0, vict, TO_CHAR);
        } else {
            if (action->char_found)
                act(action->char_found, 0, ch, 0, vict, TO_CHAR);
            if (action->others_found)
                act(action->others_found, action->hide, ch, 0, vict, TO_NOTVICT);
            if (action->vict_found)
                act(action->vict_found, action->hide, ch, 0, vict, TO_VICT);
        }
    }
}



void
cmd_insult(struct char_data *ch, const char *argument, Command cmd, int count)
{
    static char buf[100];
    static char arg[MAX_STRING_LENGTH];
    struct char_data *victim;

    argument = one_argument(argument, arg, sizeof(arg));

    if (*arg) {
        if (!(victim = get_char_room_vis(ch, arg))) {
            send_to_char("Can't hear you!\n\r", ch);
        } else {
            if (victim != ch) {
                sprintf(buf, "You insult them.\n\r");
                send_to_char(buf, ch);

                switch (random() % 3) {
                case 0:{
                        if (GET_SEX(ch) == SEX_MALE) {
                            if (GET_SEX(victim) == SEX_MALE)
                                act("$n accuses you of fighting like a woman!", FALSE, ch, 0,
                                    victim, TO_VICT);
                            else
                                act("$n says that women can't fight.", FALSE, ch, 0, victim,
                                    TO_VICT);
                        } else {        /* Ch == Woman */
                            if (GET_SEX(victim) == SEX_MALE)
                                act("$n accuses you of having the smallest.... (brain?)", FALSE, ch,
                                    0, victim, TO_VICT);
                            else
                                act("$n tells you that you'd lose a beautycontest against a troll.",
                                    FALSE, ch, 0, victim, TO_VICT);
                        }
                    }
                    break;
                case 1:{
                        act("$n calls your mother a bitch!", FALSE, ch, 0, victim, TO_VICT);
                    }
                    break;
                default:{
                        act("$n tells you to get lost!", FALSE, ch, 0, victim, TO_VICT);
                    }
                    break;
                }               /* end switch */

                act("$n insults $N.", TRUE, ch, 0, victim, TO_NOTVICT);
            } else {            /* ch == victim */
                send_to_char("You feel insulted.\n\r", ch);
            }
        }
    } else
        send_to_char("Sure you don't want to insult everybody.\n\r", ch);
}

void
cmd_pose(struct char_data *ch, const char *argument, Command cmd, int count)
{
    send_to_char("Sorry Buggy command.\n\r", ch);
    return;
}

