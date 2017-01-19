//#include <Python.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

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

extern int initialize_map(struct marker_map*);
extern struct marker *insert_marker(struct marker_map*, const char *, size_t, size_t);
extern void dealloc_map(struct marker_map*);

#define CONSUME_STRING(d, p) while((d)[(p)++] != '"') { \
                                if((d)[(p)] == '\\') {++(p); ++(p);}}

#define CONSUME_LIST(d, p) { int depth = 1;                                         \
                            while(depth) {                                          \
                                switch((d)[(p)++]) {                                \
                                case '"': CONSUME_STRING((d), (p)); break;          \
                                case '[': depth++; break;                           \
                                case ']': depth--; break;                           \
                                }}}

// The work horse. Build that map man!
struct marker_map scan(const char *data, size_t len)
{
    struct marker *stack[100] = {0}, *mark;
    struct marker_map map = {.pool = NULL, .size = 0};
    size_t pos = 0, prev_pos = 0, stack_ptr = 0;

    if(initialize_map(&map) == -1)
        goto err;

    printf("Got string: %s\n", data);
    // First verify the object
    if(data[pos++] != '{')
        goto err;

    while(pos < len) {
        // Next key
        while(isspace(data[pos])) ++pos;
        if(data[pos++] != '"')
            goto err;

        prev_pos = pos;
        CONSUME_STRING(data, pos);
        printf("Trying to insert for: %.*s\n", (int)(pos - 1 - prev_pos), data + prev_pos);
        mark = insert_marker(&map, data, prev_pos, pos - 1);
        if(!mark)
            goto err;
        mark->key_start = prev_pos; // dodge "
        mark->key_end = pos - 1;
        printf("Got key: %.*s\n", (int)(mark->key_end - mark->key_start), data + mark->key_start);
        // Find ':'
        while(isspace(data[pos])) ++pos;
        if(data[pos++] != ':')
            goto err;

        while(isspace(data[pos])) ++pos;
        mark->val_start = pos;
        switch(data[pos++]) {
        case '{':
            stack[stack_ptr++] = mark;
            continue;
        case '"':
            CONSUME_STRING(data, pos);
            mark->val_end = pos;
            break;
        case '[':
            CONSUME_LIST(data, pos);
            mark->val_end = pos;
            break;
        default:
            while(data[pos] != '}' && data[pos] != ',') ++pos;
            mark->val_end = pos;
        }
        if(stack_ptr > 0) {
            mark->parent = stack[stack_ptr - 1];
        }
        printf("Got val: %.*s\n", (int)(mark->val_end - mark->val_start), data + mark->val_start);
        // Check the terminator
        while(stack_ptr && data[pos] == '}') {
            printf("Popping stack\n");
            mark = stack[--stack_ptr];
            mark->val_end = ++pos;
        }
        ++pos;
        printf("Iterating at pos: %zu within len: %zu\n", pos, len);
    }

    printf("returning map\n");
    return map;

err:
    dealloc_map(&map);
    map.pool = NULL;
    map.size = 0;
    return map;
}

int main(int argc, char *argv[]) {
    struct marker_map m;
    int i = 0, count = 0;
    FILE *f;
    char *data;
    size_t sz = 0;
    if(argc != 2) {
        fprintf(stderr, "Invalid invocation\n");
        exit(1);
    }

    f = fopen(argv[1], "r");
    fseek(f, 0, SEEK_END);
    sz = ftell(f);
    fseek(f, 0, SEEK_SET);

    data = malloc(sz + 1);
    data[sz] = 0;
    fread(data, 1, sz, f);
    //m = scan(argv[1], strlen(argv[1]));

    m = scan(data, sz);

    for(; i < m.size; ++i) {
        if(m.pool[i].used) {
            count++;
            printf("Key: %.*s -> Val: %.*s\n",
                (int)(m.pool[i].key_end - m.pool[i].key_start), data + m.pool[i].key_start,
                (int)(m.pool[i].val_end - m.pool[i].val_start), data + m.pool[i].val_start);
        }
    }

    printf("Total elements: %d\n", count);
    dealloc_map(&m);
}
