/*
 *  File:	show_menu.h
 *  Author:	Morgenes - Neal Haggard
 *  Purpose:	defines for the menu system for Armageddon
 */

#ifndef __SHOW_MENU_INCLUDED
#define __SHOW_MENU_INCLUDED


typedef struct menu_key MENU_KEY;

struct menu_key
{
    char key;
    char *text;
    int show;
};

#define MENU_KEY_SHOW_ALWAYS       0
#define MENU_KEY_NO_ACCOUNT        1
#define MENU_KEY_HAS_CHARS         2
#define MENU_KEY_CAN_CREATE_NEW    3
#define MENU_KEY_CONNECTED_TO_CHAR 4
#define MENU_KEY_NOT_CONNECTED_TO_CHAR 5
#define MENU_KEY_CAN_REVISE_CHAR   6
#define MENU_KEY_CONFIRMED_EMAIL   7
#define MENU_KEY_HAS_ACCOUNT       8
#define MENU_KEY_HAS_CHARS_NOT_CONNECTED 9

//#define MAX_MENU_KEYS 20
#define MAX_MENU_KEYS 18

#define NUM_MENU_MANTIS 4
#define MANTIS_NORMAL 0
#define MANTIS_SANDY_CLAWS 1
#define MANTIS_PIKACHU 2
#define MANTIS_CYBERMAN 3


#endif

