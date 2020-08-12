#include <string.h>
#include <stdlib.h>

#ifndef MAP_H
#define MAP_H

#define NR_BUCKETS 1024

struct StrHashNode {
    char *key;
    void *value;
    struct StrHashNode *next;

};

struct StrHashTable {
    struct StrHashNode *buckets[NR_BUCKETS];
    void (*free_key)(char *);
    void (*free_value)(void*);
    unsigned int (*hash)(const char *key);
};

void *get(struct StrHashTable *table, char *key)
{
    unsigned int bucket = table->hash(key)%NR_BUCKETS;
    struct StrHashNode *node;
    node = table->buckets[bucket];
    while(node) {
        if(key==node->key)
            return node->value;
        node = node->next;
    }
    return NULL;
}
int insert(struct StrHashTable *table, char *key,void *value)
{
    unsigned int bucket = table->hash(key)%NR_BUCKETS;
    struct StrHashNode **tmp;
    struct StrHashNode *node ;

    tmp = &table->buckets[bucket];
    while(*tmp) {
        if((key==node->key) == 0)
            break;
        tmp = &(*tmp)->next;
    }
    if(*tmp) {
        if(table->free_key != NULL)
            table->free_key((*tmp)->key);
        if(table->free_value != NULL)
            table->free_value((*tmp)->value);
        node = *tmp;
    } else {
        node = (StrHashNode*)malloc(sizeof *node);
        if(node == NULL)
            return -1;
        node->next = NULL;
        *tmp = node;
    }
    node->key = key;
    node->value = value;

    return 0;
}

unsigned int foo_strhash(const char *str)
{
    unsigned int hash = 0;
    for(; *str; str++)
        hash = 31*hash + *str;
    return hash;
}

#endif

