/*
 * File: XML.H
 * Usage: Routines and commands for handling XML files.
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
/* Revision history:
 * 1/6/2006 -- File created.  (Tiernan)
 */

#ifndef  __XML_INCLUDED
#define  __XML_INCLUDED

#include "expat.h"
#include "character.h"

#define XML_AMP		"&amp;"
#define XML_LT		"&lt;"
#define XML_GT		"&gt;"
#define XML_QUOT	"&quot;"
#define XML_APOS	"&apos;"

/* I'm not particularly fond of having to include all the different types
 * of state-managing information for all the start & end handlers in one
 * very large parse structure.  However, since we don't have a way to 
 * truly subclass objects or anything, this is the best compromise.
 *
 * If you add a new set of handlers and need to manage state with a new
 * set of parse data, add it into this one and then use the one you need.
 */
typedef struct parse_xml_data
{
    char *tmp;                  // char pointer for data in between tags
    CHARACTER_PARSE_DATA char_parse_data;       // Character parsing info
}
PARSE_DATA;

void content_handler(void *userData, const XML_Char * s, int len);
int parse_xml_file(FILE * fp, void *parse_data, void *start_handler, void *end_handler);
void write_start_element_raw(FILE * fp, const char *name, const char **atts);
void write_start_element(FILE * fp, const char *name, const char **atts);
void write_end_element_raw(FILE * fp, const char *name);
void write_end_element(FILE * fp, const char *name);
void write_full_element(FILE * fp, const char *name, const char **atts, const char *fmt, ...);

char *encode_str_to_xml(const char *unencodedString);
#endif
