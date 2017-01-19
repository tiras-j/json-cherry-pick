#include <Python.h>
#include <bytesobject.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

static PyObject *json_module = NULL;
static PyObject *json_loads = NULL;

static PyObject *jcp_extract(PyObject *self, PyObject *args, PyObject *kwargs);
static PyObject *jcp_extract_all(PyObject *self, PyObject *args, PyObject *kwargs);
static PyObject *jcp_pluck_list(PyObject *self, PyObject *args, PyObject *kwargs);
static PyObject *jcp_key_exists(PyObject *self, PyObject *args, PyObject *kwargs);

static PyMethodDef JCPMethods[] = {
    {"extract", (PyCFunction) jcp_extract, METH_VARARGS | METH_KEYWORDS, "Extract the first instance of key."},
    {"extract_all", (PyCFunction) jcp_extract_all, METH_VARARGS | METH_KEYWORDS, "Extract all instances of key."},
    {"pluck_list", (PyCFunction) jcp_pluck_list, METH_VARARGS | METH_KEYWORDS, "Pluck and specific index of a list."},
    {"key_exists", (PyCFunction) jcp_key_exists, METH_VARARGS | METH_KEYWORDS, "Check if a key exists in the entry."},
    {NULL, NULL, 0, NULL}
};

struct marker {
    const char *ptr;
    ssize_t len;
};

typedef struct marker (*SCAN)(const char*, const char*);

/******************** UTF-8 parser ************************************/

#define S_CHAR(c) (c >= ' ' && c <= '~' && c != '\\' && c != '"')

static const char *HEXDIGITS = "0123456789abcdef";
static unsigned short m[] = {0b00111111, 0b00011111, 0b00001111, 0b00000111};

#define HIGH_SURROGATE(c) (0xD800 + (((c) - 0x1000) & 0xFFC00))
#define LOW_SURROGATE(c) (0xDC00 + (((c) - 0x1000) & 0x003FF))


#define U4(b0, b1, b2, b3) ((((b0) & m[3]) << 18) | \
                            (((b1) & m[0]) << 12) | \
                            (((b2) & m[0]) << 6)  | \
                            (((b3) & m[0])))

#define U3(b0, b1, b2) ((((b0) & m[2]) << 12) | \
                        (((b1) & m[0]) << 6)  | \
                        (((b2) & m[0])))

#define U2(b0, b1) ((((b0) & m[1]) << 6) | \
                    (((b1) & m[0])))

static int is_non_ascii(const char *s)
{
    while(*s && *s > 0) ++s;
    return *s?1:0;
}

static char *escape_utf8(const char *s)
{
    size_t req_size = 1, idx = 0;
    const char *ptr = s;
    char *output;
    unsigned int c = 0;
    /* mask values for bit pattern of first byte in multi-byte
     UTF-8 sequences: 
       192 - 110xxxxx - for U+0080 to U+07FF 
       224 - 1110xxxx - for U+0800 to U+FFFF 
       240 - 11110xxx - for U+010000 to U+1FFFFF */
    static unsigned short mask[] = {192, 224, 240}; 

    while(*ptr) {
        if S_CHAR(*ptr) {
            ++req_size;
            ++ptr;
            continue;
        }
        switch(*ptr) {
        case '\\': case '"': case '\b': case '\f':
        case '\n': case '\r': case '\t':
            req_size += 2; break;
        default:
            if ((*ptr & mask[2]) == mask[2]) {
                req_size += 12;
                ptr += 3;
            } else if ((*ptr & mask[1]) == mask[1]) {
                req_size += 6;
                ptr += 2;
            } else {
                req_size += 6;
                ptr += 1;
            }
        }
        ++ptr;
    }

    output = calloc(req_size, 1);

    // Now escape:
    ptr = s;
    while(*ptr) {
        if S_CHAR(*ptr) {
            output[idx++] = *ptr++;
            continue;
        }
        output[idx++] = '\\';
        switch(*ptr) {
        case '\\': output[idx++] = *ptr; break;
        case '"': output[idx++] = *ptr; break;
        case '\b': output[idx++] = 'b'; break;
        case '\f': output[idx++] = 'f'; break;
        case '\n': output[idx++] = 'n'; break;
        case '\r': output[idx++] = 'r'; break;
        case '\t': output[idx++] = 't'; break;
        default:
            if ((*ptr & mask[0]) == mask[0]) {
                c = U2(ptr[0], ptr[1]);
                ptr++;
            } else if ((*ptr & mask[1]) == mask[1]) {
                c = U3(ptr[0], ptr[1], ptr[2]);
                ptr++; ptr++;
            } else {
                c = U4(ptr[0], ptr[1], ptr[2], ptr[3]);
                ptr++; ptr++; ptr++;
            }
            if (c > 0x1000) {
                /* UTF-16 surrogate pair */
                unsigned int v = HIGH_SURROGATE(c);
                output[idx++] = 'u';
                output[idx++] = HEXDIGITS[(v >> 12) & 0xf];
                output[idx++] = HEXDIGITS[(v >>  8) & 0xf];
                output[idx++] = HEXDIGITS[(v >>  4) & 0xf];
                output[idx++] = HEXDIGITS[(v      ) & 0xf];
                c = LOW_SURROGATE(c);
                output[idx++] = '\\';
            }
            output[idx++] = 'u';
            output[idx++] = HEXDIGITS[(c >> 12) & 0xf];
            output[idx++] = HEXDIGITS[(c >>  8) & 0xf];
            output[idx++] = HEXDIGITS[(c >>  4) & 0xf];
            output[idx++] = HEXDIGITS[(c      ) & 0xf];
        }
        ptr++;
    }

    return output;
}
/******************** Scan functions **********************************/

// Assume pointing at start quote -> we return with pointer at end quote
#define CONSUME_STRING(data, len) while((data)[++(len)] != '"') {        \
                                      if((data)[(len)] == 0) return -1;  \
                                      if((data)[(len)] == '\\') ++(len);}

static struct marker NULL_MARKER = {.ptr = NULL, .len = -1};

// Find the length of the value starting at data.
static ssize_t scan_pair_value(const char *data)
{
    int depth = 0, target = 0;
    ssize_t len = 0;

    switch(*data) {
    case '{': target = '}'; break;
    case '[': target = ']'; break;
    }

    if(target) {
        for(;;) {
            switch(data[len]) {
            case '{': case '[': depth++; break;
            case '}': case ']': depth--; break;
            case '"': CONSUME_STRING(data, len); break;
            case 0: return -1;
            }
            if(data[len++] == target && depth == 0)
                return len;
        }
    } else {
        for(;;) {
            switch(data[len]) {
            case '}': case ',': case ']': case 0:
                return len;
            case '"':
                CONSUME_STRING(data, len); break;
            }
            len++;
        }
    }
}

// Scan will seek from data to the first occurrence of
// "key" and return the associated pair value.
// If the key is not found, a marker with NULL, -1 is
// returned
static const char* scan_pair_key(const char *data, const char *key)
{
    const char* search_ptr = data;
    const char* index;
    size_t key_len = strlen(key);
    for(;;) {
        if((index = strstr(search_ptr, key)) == NULL)
            return index;

        // Verify the key. We need to be immediately preceded
        // and followed by non-escaped quotations, further followed
        // by any amount of whitespace to a ':' to indicate a key
        // portion of a JSON pair.
        if( !(index[-1] == '"' && index[-2] != '\\') || !(index[key_len] == '"') ) {
            search_ptr++;
            continue;
        }

        // Move forward the length of the key
        index += key_len;

        // Eat whitespace (if any) until ':'
        while(isspace(*(++index)));
        if( !(*index == ':') ) {
            search_ptr++;
            continue;
        }

        // Set index to the first non-whitespace following ':'
        // this is the start of our value
        while(isspace(*(++index)));
        return index;
    }
}

static struct marker scan_no_tok(const char *data, const char *key)
{
    struct marker m = {.ptr = data, .len = -1};
    if( (m.ptr = scan_pair_key(data, key)) == NULL || (m.len = scan_pair_value(m.ptr)) == -1) {
        return NULL_MARKER;
    }
    return m;
}

static struct marker scan(const char *data, const char *key)
{
    struct marker m_prev, m = {.ptr = data, .len = -1};
    char* new_key = strdup(key);
    char* token = strtok(new_key, ".");

    while(token) {
        m_prev = m;
        if((m.ptr = scan_pair_key(m.ptr, token)) == NULL || (m.len = scan_pair_value(m.ptr)) == -1) {
            return NULL_MARKER;
        }
        // Check scope from prev
        if(m_prev.len != -1 && (m.ptr - m_prev.ptr) > m_prev.len) {
            return NULL_MARKER;
        }
        token = strtok(NULL, ".");
    }
    free(new_key);    
    return m;
}


static struct marker index_into_list(const char *data, int index)
{
    struct marker m =  {.ptr = data, .len = -1};

    // Handle the empty array case
    while(isspace(*(++m.ptr)));
    if(*m.ptr == ']' || *m.ptr == 0){
        return NULL_MARKER;
    }

    // Scan to our target
    while(index--) {
        if((m.len = scan_pair_value(m.ptr)) == -1)
            return NULL_MARKER;
        m.ptr += m.len;
        if(*m.ptr == ']' || *m.ptr == 0)
            return NULL_MARKER;
        while(isspace(*(++m.ptr)));
    }

    // Grab the target
    if((m.len = scan_pair_value(m.ptr)) == -1)
        return NULL_MARKER;
    return m;
}

/********************************* End Scan Functions **********************/

static PyObject *call_json_loads(struct marker m)
{
    PyObject *arglist, *str, *res;

    str = PyUnicode_FromStringAndSize(m.ptr, m.len); 
    arglist = Py_BuildValue("(O)", str);
    res = PyObject_CallObject(json_loads, arglist);
    Py_DECREF(arglist);
    Py_DECREF(str);
    return res;
}

static PyObject *jcp_pluck_list(PyObject *self, PyObject *args, PyObject *kwargs)
{
    const char *data;
    const char *key;
    char *conv_key = NULL;
    int index;
    unsigned char tokenize = 1;
    SCAN scan_func;
    struct marker result;

    static char *kwlist[] = {"data", "key", "index", "tokenize", NULL};

    if(!PyArg_ParseTupleAndKeywords(args, kwargs, "ssi|b", kwlist, &data, &key, &index, &tokenize))
        return NULL;

    if(index < 0) {
        return PyErr_Format(PyExc_IndexError, "Reverse indexing not yet supported");
    }

    if(is_non_ascii(key)) {
        conv_key = escape_utf8(key);
    }

    scan_func = tokenize?scan:scan_no_tok;
    result = conv_key?scan_func(data, conv_key):scan_func(data, key);

    if(conv_key) {
        free(conv_key);
    }

    if(result.ptr == NULL) {
        return PyErr_Format(PyExc_KeyError, "%s not found", key);
    }
    if(*result.ptr != '[') {
        return PyErr_Format(PyExc_ValueError, "%s is not an indexable object", key);
    }
    result = index_into_list(result.ptr, index);
    if(result.ptr == NULL) {
        return PyErr_Format(PyExc_IndexError, "%d is out of range", index);
    }
    
    return call_json_loads(result);
}

static PyObject *jcp_extract(PyObject *self, PyObject *args, PyObject *kwargs)
{
    const char *data;
    const char *key;
    char *conv_key = NULL;
    SCAN scan_func;
    unsigned char tokenize = 1;
    struct marker result;

    static char *kwlist[] = {"data", "key", "tokenize", NULL};
    if(!PyArg_ParseTupleAndKeywords(args, kwargs, "ss|b", kwlist, &data, &key, &tokenize))
        return NULL;

    if(is_non_ascii(key)) {
        conv_key = escape_utf8(key);
    }

    scan_func = tokenize?scan:scan_no_tok;
    result = conv_key?scan_func(data, conv_key):scan_func(data, key);

    if(conv_key) {
        free(conv_key);
    }

    if(result.ptr == NULL) {
        return PyErr_Format(PyExc_KeyError, "%s not found", key);
    }
    
    return call_json_loads(result);
}

static PyObject *jcp_extract_all(PyObject *self, PyObject *args, PyObject *kwargs)
{
    PyObject *list, *obj;
    const char *data;
    const char *key;
    char *conv_key = NULL;
    SCAN scan_func;    
    unsigned char tokenize = 1;
    struct marker result;
    static char *kwlist[] = {"data", "key", "tokenize", NULL};

    if(!PyArg_ParseTupleAndKeywords(args, kwargs, "ss|b", kwlist, &data, &key, &tokenize))
        return NULL;

    if(is_non_ascii(key)) {
        conv_key = escape_utf8(key);
    }

    scan_func = tokenize?scan:scan_no_tok;
    result = conv_key?scan_func(data, conv_key):scan_func(data, key);

    if(result.ptr == NULL) {
        if(conv_key) {
            free(conv_key);
        }
        return PyErr_Format(PyExc_KeyError, "%s not found", key);
    }

    obj = call_json_loads(result);
    list = PyList_New(1);
    PyList_SET_ITEM(list, 0, obj);

    for(;;) {
        result = conv_key?scan_func(result.ptr, conv_key):scan_func(result.ptr, key);
        if(result.ptr == NULL)
            break;
        obj = call_json_loads(result);
        if(obj == NULL) {
            if(conv_key) {
                free(conv_key);
            }
            Py_DECREF(list);
            return NULL;
        }
        if(PyList_Append(list, obj) == -1) {
            if(conv_key) {
                free(conv_key);
            }
            Py_DECREF(obj);
            Py_DECREF(list);
            return NULL;
        }
        Py_DECREF(obj);
    }

    if(conv_key) {
        free(conv_key);
    }
    return list;
}

static PyObject *jcp_key_exists(PyObject *self, PyObject *args, PyObject *kwargs)
{
    const char *data;
    const char *key;
    char *conv_key = NULL;
    SCAN scan_func;    
    unsigned char tokenize = 1;
    struct marker result;
    static char *kwlist[] = {"data", "key", "tokenize", NULL};

    if(!PyArg_ParseTupleAndKeywords(args, kwargs, "ss|b", kwlist, &data, &key, &tokenize))
        return NULL;

    if(is_non_ascii(key)) {
        conv_key = escape_utf8(key);
    }

    // Re-use the scanning functions here.
    // Not *quite* as efficient as strtok/strstr-ing ourselves
    // but the difference is fairly negligible since it's still
    // string walking. And we get the scoping guarantees of tokenizatino
    scan_func = tokenize?scan:scan_no_tok;
    result = conv_key?scan_func(data, conv_key):scan_func(data, key);

    if(conv_key) {
        free(conv_key);
    }

    if(result.ptr == NULL) {
        Py_RETURN_FALSE;
    }

    Py_RETURN_TRUE;
}
    
#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef jcpmodule = {
    PyModuleDef_HEAD_INIT,
    "c_jcp",
    NULL,
    -1,
    JCPMethods
};

#define INITERROR return NULL
PyMODINIT_FUNC PyInit_c_jcp(void)
#else
PyMODINIT_FUNC initc_jcp(void)
#define INITERROR return
#endif
{
#if PY_MAJOR_VERSION >=3
    PyObject *module = PyModule_Create(&jcpmodule);
#else
    PyObject *module = Py_InitModule("c_jcp", JCPMethods);
#endif

    if(module == NULL)
        INITERROR;

    json_module = PyImport_ImportModule("json");
    json_loads = PyObject_GetAttr(json_module, PyUnicode_FromString("loads"));

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}
