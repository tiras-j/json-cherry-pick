// Flat hash map for storing blob markers 
//#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define DEFAULT_ELEMENTS 256

struct marker {
//    PyObject *loaded_json;
    struct marker *parent;
    size_t key_start;
    size_t key_end;
    size_t val_start;
    size_t val_end;
    int used;
};

struct marker_map {
    struct marker *pool;
    size_t size;
};

static unsigned long djb2_hash(const char *s, size_t len)
{
    printf("Building hash for key %.*s\n", (int)len, s);
    unsigned long h = 5381;
    int i = 0;
    for(; i < len; ++i) {
        h = ((h << 5) + h) + s[i];
    }

    printf("Got hash %lu\n", h);
    return h;
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
    return 0;
}

struct marker *insert_marker(struct marker_map *map, const char *data, size_t start, size_t end)
{
    unsigned long pos = djb2_hash(data + start, end - start) % map->size;
    unsigned long s_pos = pos;

    printf("Inserting into pos: %zu\n", pos);
    do {
        if(!map->pool[pos].used) {
            map->pool[pos].used = 1;
            return &map->pool[pos];
        }
        if(++pos == map->size)
            pos = 0;
    } while(pos != s_pos);

    printf("No free slots!\n");
    // Eventually realloc and rehash
    return NULL;
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

void dealloc_map(struct marker_map *m) {
    size_t i = 0;
    if(m == NULL || m->pool == NULL)
        return;
/*
    for(; i < m->size; ++i) {
        Py_XDECREF(m->pool[i].loaded_json);
    }
*/
    free(m->pool);
    return;
}

