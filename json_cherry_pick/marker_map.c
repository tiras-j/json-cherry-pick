// Flat hash map for storing blob markers 
#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include "marker_map.h"

#define DEFAULT_ELEMENTS 256

static unsigned long djb2_hash(const char *s, size_t len)
{
    unsigned long h = 5381;
    int i = 0;
    for(; i < len; ++i) {
        h = ((h << 5) + h) + s[i];
    }

    return h;
}

static ssize_t locate_free_slot(struct marker_map *map, unsigned long hash)
{
    unsigned long pos, s_pos;
    pos = s_pos = hash % map->size;
    
    do {
        if(!map->pool[pos].used)
            return pos;
        if(++pos == map->size)
            pos = 0;
    } while(pos != s_pos);

    return -1;
}
    
static int realloc_map(struct marker_map *map)
{
    size_t i = 0, slot = 0, old_size = map->size;
    struct marker *new_pool = calloc(2*old_size, sizeof(struct marker));
    struct marker *old_pool = map->pool;
    
    if(new_pool == NULL)
        return -1;

    map->pool = new_pool;
    map->size = 2 * old_size;

    for(; i < old_size; ++i) {
        if(old_pool[i].used) {
            if((slot = locate_free_slot(map, old_pool[i].hash)) == -1)
                goto realloc_err;
            memcpy(&new_pool[slot], &old_pool[i], sizeof(struct marker));
        }
    }
    free(old_pool);
    return 0;

realloc_err:
    map->pool = old_pool;
    map->size = old_size;
    free(new_pool);
    return -1;
}

int initialize_map(struct marker_map *m)
{
    if(m == NULL) {
        return -1;
    }

    m->pool = calloc(DEFAULT_ELEMENTS, sizeof(struct marker));
    if(!m->pool) {
        return -1;
    }

    m->size = DEFAULT_ELEMENTS;
    m->nmemb = 0;
    return 0;
}

struct marker *insert_marker(struct marker_map *map, const char *data, size_t start, size_t end)
{
    unsigned long hash = djb2_hash(data + start, end - start);
    ssize_t pos;

    if(map->nmemb == map->size) {
        //printf("Realloc... ");
        if(realloc_map(map) == -1)
            return NULL;
        //printf("success!\n");
    }

    if((pos = locate_free_slot(map, hash)) == -1)
        return NULL;

    map->pool[pos].used = 1;
    map->pool[pos].hash = hash;
    map->nmemb++;
    return &map->pool[pos];
}

struct marker *fetch_marker(struct marker_map *map, const char *data, const char *key)
{

    unsigned long pos = djb2_hash(key, strlen(key)) % map->size;
    unsigned long s_pos = pos;

    do {
        if(!map->pool[pos].used) {
            return NULL;
        } else if(!strncmp(key, data + map->pool[pos].key_start, strlen(key))) {
            return &map->pool[pos];
        }
        if(++pos == map->size)
            pos = 0;
    } while(pos != s_pos);

    return NULL;
}

void dealloc_map(struct marker_map *m)
{
    size_t i = 0;
    if(m == NULL || m->pool == NULL)
        return;

    for(; i < m->size; ++i) {
        Py_XDECREF(m->pool[i].loaded_json);
    }

    free(m->pool);
    return;
}

