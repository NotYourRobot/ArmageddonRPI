/*
 * File: stringutils.c */
#include <string.h>

#include "core_structs.h"
#include "modify.h"
#include "utils.h"

/*
 * capitalize  - returns an initial-capped string.
 */

char *
capitalize(const char *str)
{
    static char strcap[16384 * 10];

    strncpy(strcap, str, sizeof(strcap));
    strcap[0] = UPPER(strcap[0]);
    return strcap;
}

char *
lowercase(const char *str)
{
    static char strcap[MAX_STRING_LENGTH];
    int i;

    for (i = 0; i < strlen(str); i++)
        strcap[i] = LOWER(str[i]);

    strcap[i] = '\0';
    return strcap;
}

/* find the closing parenthesis (or whatever) on the same "level"
 * as the opening one, such that "((a + b) + c)" will return...
 *                                ^           ^--the pos of this one */
int
close_parens(const char *exp, char type)
{
    int paren, pos, i;
    char buf[256];
    char p_type[] = { ']', '}', ')', '\'' };

    if (type == '[')
        i = 0;
    else if (type == '{')
        i = 1;
    else if (type == '(')
        i = 2;
    else if (type == '\'')
        i = 3;
    else {
        sprintf(buf, "Invalid %c check for close_parens()", type);
        //shhlog(buf);
        return (0);
    }

    for (paren = 1, pos = 1; paren > 0; pos++) {
        if (*(exp + pos) == '\0')       /* reach the end without find */
            return (0);         /* 0 is less than the min pos */
        if ((i == 3) && (*(exp + pos) == type))
            return (pos);
        if (*(exp + pos) == type)       /* another open */
            paren++;
        else if (*(exp + pos) == p_type[i])     /* another closed */
            paren--;
    }

    return (pos - 1);
}


/*
 * Grab 'a' 'an's and 'the's and smash the living hell out of them.
 * Good for use with manipulation of object names.  Only destroys first
 * occurance.
 */
char *
smash_article(const char *text)
{
    char buf[MAX_STRING_LENGTH];
    static char buf2[MAX_STRING_LENGTH];

    strcpy(buf2, "<NULL>");
    if (!text)
        return buf2;

    text = one_argument(text, buf, sizeof(buf));

    if (!strcmp(buf, "the") || !strcmp(buf, "The")
        || !strcmp(buf, "an") || !strcmp(buf, "An")
        || !strcmp(buf, "a") || !strcmp(buf, "A")) {
        sprintf(buf2, "%s", text);
    } else {
        sprintf(buf2, "%s%s%s", buf, *text != '\0' ? " " : "", text);
    }

    return buf2;
}

/*
 * Gets rid of the given word in the string
 * -Case insensitive search
 * -Matches multiple instances
 * -Need to strdup the result
 */
char *
smash_word(char *text, char *word)
{
    static char buf[MAX_STRING_LENGTH];
    char arg[MAX_STRING_LENGTH];
    char *orig;
    const char *tmp;

    strcpy(buf, "<NULL>");
    if (!text)
        return buf;

    buf[0] = '\0';
    orig = strdup(text);
    tmp = one_argument(orig, arg, sizeof(arg));
    while (arg[0] != '\0') {
        /* if it's not the word we're looking for (case insensitive) */
        if (strcasecmp(arg, word)) {
            strcat(buf, arg);
            strcat(buf, " ");
        }

        tmp = one_argument(tmp, arg, sizeof(arg));
    }

    /* get rid of the trailing space */
    buf[strlen(buf) - 1] = '\0';
    free((char *) orig);
    return buf;
}


/*
 * Compare strings, case insensitive, for prefix matching.
 * Return TRUE if astr not a prefix of bstr
 *   (compatibility with historical functions).
 */
bool
str_prefix(const char *astr, const char *bstr)
{
    if (astr == NULL) {
        fprintf(stderr, "Strprefix: null astr.");
        return TRUE;
    }

    if (bstr == NULL) {
        fprintf(stderr, "Strprefix: null bstr.");
        return TRUE;
    }

    for (; *astr; astr++, bstr++) {
        if (LOWER(*astr) != LOWER(*bstr))
            return TRUE;
    }

    return FALSE;
}


/*
 * Compare strings, case insensitive, for match anywhere.
 * Returns TRUE is astr not part of bstr.
 *   (compatibility with historical functions).
 */
bool
str_infix(const char *astr, const char *bstr)
{
    int sstr1;
    int sstr2;
    int ichar;
    char c0;

    if (astr == NULL) {
        fprintf(stderr, "Strinfix: null astr.");
        return TRUE;
    }

    if (bstr == NULL) {
        fprintf(stderr, "Strinfix: null bstr.");
        return TRUE;
    }

    if ((c0 = LOWER(astr[0])) == '\0')
        return FALSE;

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);

    for (ichar = 0; ichar <= sstr2 - sstr1; ichar++) {
        if (c0 == LOWER(bstr[ichar]) && !str_prefix(astr, bstr + ichar))
            return FALSE;
    }

    return TRUE;
}


/*
 * Compare strings, case insensitive, for suffix matching.
 * Return TRUE if astr not a suffix of bstr
 *   (compatibility with historical functions).
 */
bool
str_suffix(const char *astr, const char *bstr)
{
    int sstr1;
    int sstr2;

    if (astr == NULL) {
        fprintf(stderr, "Strsuffix: null astr.");
        return TRUE;
    }

    if (bstr == NULL) {
        fprintf(stderr, "Strsuffix: null bstr.");
        return TRUE;
    }

    sstr1 = strlen(astr);
    sstr2 = strlen(bstr);
    if (sstr1 <= sstr2 && !strcasecmp(astr, bstr + sstr2 - sstr1))
        return FALSE;
    else
        return TRUE;
}



/*
 * Removes the tildes from a string.
 * Used for player-entered strings that go into disk files.
 */
void
smash_tilde(char *str)
{
    for (; *str != '\0'; str++) {
        if (*str == '~')
            *str = '-';
    }

    return;
}

/* Removes trailing blanks from a string. */
void
smash_blanks(char *str)
{
    int i;

    for (i = strlen(str) -1; i >= 0 && (str[i] == '\t' || str[i] == ' '); i--)
	str[i] = str[i+1];

    return;
}

char *
smash_char(char *str, char toRem)
{
    static char buf[MAX_STRING_LENGTH];
    char *ptr = buf;
    char *orig = str;

    for (; *str != '\0'; str++) {
        if (*str != toRem) {
            *ptr = *str;
            ptr++;
        }
    }
    free(orig);
    *ptr = '\0';
    return (strdup(buf));
}

void
replace_char( const char *str, char from, char to, char *buf, int length ) {
    int count = 0;
    for( ; *str != '\0' && count < length; str++, count++) {
        if( *str == from )
            *buf = to;
        else
            *buf = *str;
        buf++;
    }
    *buf = '\0';
}

/* get one argument from a string */

const char *
one_argument(const char *source, char *target, size_t target_len)
{
    const char *begin;
    int len_s;                  /* length of non-spaces */
    const char *toksep = " \n\r\t";

    /*
     * Tenebrius replaced old, because
     * - no loops or if/thens
     * - no costly memory-alocation/deallocation.
     * - uses ansi standard functions
     */

    if (!source || !strlen(source)) {
        target[0] = '\0';
        return source;
    }

    begin = source + strspn(source, toksep);         /* move to first non-space */
    len_s = strcspn(begin, toksep);                  /* length until next space */
    len_s = MIN(len_s, target_len);                  
    memcpy(target, begin, len_s);                    /* copy it                 */
    target[len_s] = '\0';                            /* null terminate          */
    begin += len_s + strspn(begin + len_s, toksep);  /* move to first non-space */

    return begin;
}


char *
show_flags(struct flag_data *a, int b)
{

    int i;
    char buf[1024];

    strcpy(buf, "");

    for (i = 0; strlen(a[i].name); i++)
        if (IS_SET(b, a[i].bit)) {
            strcat(buf, a[i].name);
            if (b & ~(a[i].bit))
                strcat(buf, " | ");
        }

    return strdup(buf);
}


char *
show_attrib_flags(struct attribute_data *a, int b)
{

    int i;
    char buf[1024] = "";

    for (i = 0; strlen(a[i].name); i++)
        if (IS_SET(b, a[i].val)) {
            strcat(buf, a[i].name);
            strcat(buf, " ");
        }

    return strdup(buf);
}


char *
show_attrib(int val, struct attribute_data *att)
{
    int i;
    static char buf[1024];

    buf[0] = '\0';

    for (i = 0; strlen(att[i].name); i++) {
        if (att[i].val == val) {
            snprintf(buf, sizeof(buf), "%s", att[i].name);
            break;
        }
    }
    return (buf);
}

void
strip(char *line)
{
    int i;

    for (i = 0; *(line + i) != '\n' && *(line + i) != '\r' && *(line + i) != '\0'; i++);
    *(line + i) = '\0';
}


#define DISABLE_GARBAGE 1

