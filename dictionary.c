/*
 * File: dictionary.c */
#include <string.h>
#include <stdlib.h>

#include "dictionary.h"

///////////////////////////////////////////////////////////
//  Forward Declarations and typedefs
///////////////////////////////////////////////////////////
hash_node *dictionary_get_node(dictionary * dict, const char *string);
void destroy_node(hash_node * node);




void
dictionary_init(dictionary * dict, size_t tablesize)
{
    dict->table_size = tablesize;
    dict->table = (hash_node **) calloc(sizeof(hash_node), tablesize);
}

void
dictionary_insert(dictionary * dict, const char *string, void *value)
{
    dictionary_hash hashval;

    hash_node *newnode = (hash_node *) calloc(sizeof(hash_node), 1);
    newnode->value = value;
    newnode->key = strdup(string);      // God forgive me

    hashval = get_hash_val(string, dict->table_size);
    newnode->next = dict->table[hashval];
    dict->table[hashval] = newnode;
}

void
dictionary_update(dictionary * dict, const char *string, void *value)
{
    hash_node *tmp = dictionary_get_node(dict, string);
    if (!tmp)
        return;

    tmp->value = value;
}

void
dictionary_remove(dictionary * dict, const char *string)
{
    dictionary_hash hashval = get_hash_val(string, dict->table_size);
    hash_node *tmp, *last = NULL;

    if (!dict->table[hashval])
        return;                 // Nothing to remove

    for (tmp = dict->table[hashval]; tmp && strcmp(tmp->key, string); tmp = tmp->next)
        last = tmp;             //Seek

    if (tmp == dict->table[hashval]) {  // Found at front of the bin-list
        dict->table[hashval] = tmp->next;
    } else {
        last->next = tmp->next;
    }

    destroy_node(tmp);
}

void *
dictionary_get_value(dictionary * dict, const char *string)
{
    hash_node *tmp;
    tmp = dictionary_get_node(dict, string);
    if (tmp)
        return tmp->value;

    return NULL;
}

int
dictionary_entry_exists(dictionary * dict, const char *string)
{
    hash_node *tmp = dictionary_get_node(dict, string);

    return (tmp != NULL);
}

void
dictionary_foreach(dictionary * dict, void (*func) (const char *, void *))
{
    size_t i;
    for (i = 0; i < dict->table_size; i++) {
        hash_node *tmp;
        for (tmp = dict->table[i]; tmp; tmp = tmp->next) {
            func(tmp->key, tmp->value);
        }
    }
}

void
dictionary_destroy(dictionary * dict)
{
    size_t i;
    for (i = 0; i < dict->table_size; i++) {
        hash_node *tmp, *next;
        if (dict->table[i] == NULL)
            continue;

        for (tmp = dict->table[i]; tmp != NULL; tmp = next) {
            next = tmp->next;
            destroy_node(tmp);
        }
    }
    free(dict->table);
}

dictionary_hash
get_hash_val(const char *string, size_t tablesize)
{
    unsigned int i;
    unsigned int slen = strlen(string);
    unsigned long val = 0;
    memcpy(&val, string, (slen > 4 ? 4 : slen));
    for (i = 4; i < slen; i += 4) {
        unsigned long tmp = 0;
        memcpy(&tmp, string + i, (slen - i > 4 ? 4 : slen - i));
        val *= 1.4142135623730950488016887242096980785696718f;
        val ^= tmp;
    }

    return (unsigned long) (val * 1.4142135623730950488016887242096980785696718f) % tablesize;
}


/////////////////////////////////////////////////////////////////////
//  To be considered private, don't expose these in the header
/////////////////////////////////////////////////////////////////////
hash_node *
dictionary_get_node(dictionary * dict, const char *string)
{
    dictionary_hash hashval = get_hash_val(string, dict->table_size);
    hash_node *tmp;
    for (tmp = dict->table[hashval]; tmp != NULL; tmp = tmp->next) {
        if (strcmp(tmp->key, string) == 0)
            return tmp;
    }

    return NULL;
}

void
destroy_node(hash_node * node)
{
    free(node->key);
    node->key = NULL;
    node->next = NULL;
    free(node);
}


/////////////////////////////////////////////////////////////////////
//  Unit testing only
/////////////////////////////////////////////////////////////////////
#ifdef UNIT_TEST

void
print_entry(const char *key, void *val)
{
    printf(" %-20s %-5d\n", key, (int) val);
}


int
main(int argc, char **argv)
{
    int i;
    dictionary maindict;
    dictionary_init(&maindict, 8 * 1024);

    for (i = 1; i < argc; i++) {
//    printf("%-4ld\t%s\n", get_hash_val( argv[i], 8 * 1024 ), argv[i]);

        if (dictionary_entry_exists(&maindict, argv[i])) {
            dictionary_update(&maindict, argv[i], dictionary_get_value(&maindict, argv[i]) + 1);
        } else
            dictionary_insert(&maindict, argv[i], (void *) 1);
    }

    dictionary_remove(&maindict, "bundle");
    printf("Displaying Table\n");
    dictionary_foreach(&maindict, print_entry);


    dictionary_destroy(&maindict);
}

#endif // UNIT_TEST

