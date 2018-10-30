/*
 * File: XML.C
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

#include <ctype.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "character.h"
#include "comm.h"
#include "db.h"
#include "gamelog.h"
#include "handler.h"
#include "modify.h"
#include "utils.h"
#include "xml.h"

/* parse_xml_file()
 *
 * Parses an xml file
 */
int
parse_xml_file(FILE * fp, void *parse_data, void *start_handler, void *end_handler)
{
    char buf[BUFSIZ + 3];
    XML_Parser parser = 0;
    int done;

    parser = XML_ParserCreate(NULL);
    XML_SetUserData(parser, parse_data);

    XML_SetElementHandler(parser, start_handler, end_handler);
    XML_SetCharacterDataHandler(parser, content_handler);

    while (fgets(buf, sizeof(buf), fp)) {
        done = feof(fp);
        if (!XML_Parse(parser, buf, strlen(buf), done)) {
            gamelogf("%s at line %d", XML_ErrorString(XML_GetErrorCode(parser)),
                     XML_GetCurrentLineNumber(parser));
            fclose(fp);
            return FALSE;
        }
    }

    XML_ParserFree(parser);
    return TRUE;
};

/* content_handler()
 * 
 * This function is called by the parser, sending the datavalue of the element
 * as a char buffer and the length of the buffer.
 */
void
content_handler(void *userData, const XML_Char * s, int len)
{
    PARSE_DATA *parse_data = (PARSE_DATA *) userData;
    char *tmp = NULL;

    CREATE(tmp, char, len + 1);
    memcpy(tmp, s, len);
    tmp[len] = '\0';

    if (parse_data->tmp) {
        RECREATE(parse_data->tmp, char, strlen(parse_data->tmp) + len + 1);
        strcat(parse_data->tmp, tmp);
    } else {
        parse_data->tmp = strdup(tmp);
    }

    DESTROY(tmp);
}

/* write_start_element_raw()
 *
 * Writes out a start tag w/o appending a newline.
 *
 * e.g.  <name att1="val1" att2="val2">
 */
void
write_start_element_raw(FILE * fp, const char *name, const char **atts)
{
    int i;

    fprintf(fp, "<%s", name);
    for (i = 0; atts && atts[i]; i++) {
        if (!(i % 2) && atts[i + 1]) {  // even number, process the lval & rval pair
            fprintf(fp, " %s=\"%s\"", atts[i], atts[i + 1]);
        }
    }
    fprintf(fp, ">");
}

/* write_start_element()
 *
 * Writes out a start tag and appends a newline.
 *
 * e.g.  <name att1="val1" att2="val2">\n
 */
void
write_start_element(FILE * fp, const char *name, const char **atts)
{
    write_start_element_raw(fp, name, atts);
    fprintf(fp, "\n");
}

/* write_end_element_raw()
 *
 * Writes out an end tag w/o appending a newline.
 *
 * e.g. </name>
 */
void
write_end_element_raw(FILE * fp, const char *name)
{
    char *tagname = NULL;

    CREATE(tagname, char, strlen(name) + 2);
    strcpy(tagname, "/");
    strcat(tagname, name);
    write_start_element_raw(fp, tagname, NULL);
    DESTROY(tagname);
}

/* write_end_element()
 *
 * Writes out an end tag and appends a newline.
 *
 * e.g. </name>\n
 */
void
write_end_element(FILE * fp, const char *name)
{
    write_end_element_raw(fp, name);
    fprintf(fp, "\n");
}

/* write_full_element
 *
 * Writes out an entire element including start and end tags and appends
 * a newline.
 *
 * e.g. <name>data data data</name>\n
 */
void
write_full_element(FILE * fp, const char *name, const char **atts, const char *fmt, ...)
{
    va_list argp;
    char *outstr = NULL;
    char buf[MAX_STRING_LENGTH * 2];

    write_start_element_raw(fp, name, atts);
    va_start(argp, fmt);
    //vfprintf(fp, fmt, argp);
    vsnprintf(buf, sizeof(buf), fmt, argp);
    outstr = encode_str_to_xml(buf);
    fwrite(outstr, strlen(outstr), 1, fp);
    va_end(argp);
    write_end_element(fp, name);

    DESTROY(outstr);
}

/* encode_str_to_xml()
 *
 * Encodes a character string into one that is acceptable to XML parsing.
 */
char *
encode_str_to_xml(const char *unencodedString)
{
    char *encodedString = NULL;
    char tmpbuf[MAX_STRING_LENGTH];
    int i, len = MAX_STRING_LENGTH;

    CREATE(encodedString, char, len);
    strcpy(encodedString, "");

    // Scan the string for special characters that need translation
    for (i = 0; i < strlen(unencodedString); i++) {
        switch (unencodedString[i]) {
        case '&':
            strcpy(tmpbuf, XML_AMP);
            break;
        case '<':
            strcpy(tmpbuf, XML_LT);
            break;
        case '>':
            strcpy(tmpbuf, XML_GT);
            break;
        case '"':
            strcpy(tmpbuf, XML_QUOT);
            break;
        case '\'':
            strcpy(tmpbuf, XML_APOS);
            break;
        default:
            if (isprint(unencodedString[i]) || isspace(unencodedString[i])) {
                tmpbuf[0] = unencodedString[i];
                tmpbuf[1] = '\0';
            } else
                continue;
            break;
        }
        if (strlen(encodedString) + strlen(tmpbuf) + 1 > len) {
            len = strlen(encodedString) + strlen(tmpbuf) + 1;
            RECREATE(encodedString, char, len);
        }
        strcat(encodedString, tmpbuf);
    }
    return encodedString;
}

