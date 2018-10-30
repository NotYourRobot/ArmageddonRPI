/*
 * File: dictionary.h */
// Don't use these
typedef struct __hash_node
{
    char *key;
    void *value;
    struct __hash_node *next;
}
hash_node;

// Create these
typedef struct __dictionary
{
    hash_node **table;          // Array of nodes
    size_t table_size;
}
dictionary;


///////////////////////////////////////////////////////////
//  Forward Declarations and typedefs
///////////////////////////////////////////////////////////
typedef unsigned int dictionary_hash;

///////////////////////////////////////////////////////////
//
// dictionary_init  --  Allocation and initialize resources for a dictionary
//
//
void dictionary_init(dictionary * dict, size_t tablesize);
void dictionary_insert(dictionary * dict, const char *string, void *value);
void dictionary_update(dictionary * dict, const char *string, void *value);
void dictionary_remove(dictionary * dict, const char *string);
void *dictionary_get_value(dictionary * dict, const char *string);
int dictionary_entry_exists(dictionary * dict, const char *string);
void dictionary_foreach(dictionary * dict, void (*func) (const char *, void *));
void dictionary_destroy(dictionary * dict);
dictionary_hash get_hash_val(const char *string, size_t tablesize);

