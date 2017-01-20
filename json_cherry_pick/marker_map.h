#ifndef _MARKER_MAP_H
#define _MARKER_MAP_H
#include <Python.h>

struct marker {
    PyObject *loaded_json;
    struct marker *parent;
    unsigned long hash;
    size_t key_start;
    size_t key_end;
    size_t val_start;
    size_t val_end;
    int used;
};

struct marker_map {
    struct marker *pool;
    size_t size;
    size_t nmemb;
};

int initialize_map(struct marker_map *m);
struct marker *insert_marker(struct marker_map *map, const char *data, size_t start, size_t end);
struct marker *fetch_marker(struct marker_map *map, const char *data, const char *key);
void dealloc_map(struct marker_map *m);
#endif // MarkerMap header
