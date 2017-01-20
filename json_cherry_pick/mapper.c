#include <Python.h>
#include <structmember.h>
#include "marker_map.h"

typedef struct {
    PyObject_HEAD
    PyObject *data;
    const char *data_as_str;
    struct marker_map map;
} MarkerMap;

// MarkerMap functions
static Py_ssize_t MarkerMap_len(PyObject *self);
static int MarkerMap_contains(PyObject *self, PyObject *key);
static PyObject *MarkerMap_get_object(PyObject *self, PyObject *key);
static PyObject *MarkerMap_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
static int MarkerMap_init(MarkerMap *self, PyObject *args, PyObject *kwargs);
static PyObject *MarkerMap_key_exists(MarkerMap *self, PyObject *args);
static void MarkerMap_dealloc(MarkerMap *self);

// Module functions
static PyObject *call_json_loads(const char *data, struct marker *m);
static int scan(struct marker_map *m, const char *data, size_t len);

// Imported functions
static PyObject *json_module = NULL;
static PyObject *json_loads = NULL;

static PyMethodDef mapper_methods[] = {
    {NULL}
};

static PyMethodDef MarkerMap_methods[] = {
    {"key_exists", (PyCFunction) MarkerMap_key_exists, METH_VARARGS,
     "Check if a key is in the map without loading it"
    },
    {NULL}
};

static PySequenceMethods MarkerMap_as_sequence = {
    MarkerMap_len,      /* sq_length */
    0,
    0,
    0,
    0,
    0,
    0,
    MarkerMap_contains,  /* sq_contains */
    0,
    0
};

static PyMappingMethods MarkerMap_as_map = {
    MarkerMap_len,
    MarkerMap_get_object,
    0
};

static PyTypeObject MarkerMapType = {
#if PY_MAJOR_VERSION < 3
    PyObject_HEAD_INIT(NULL)
    0,
#else
    PyVarObject_HEAD_INIT(NULL, 0)
#endif
    "MarkerMap",
    sizeof(MarkerMap),
    0,                         /*tp_itemsize*/
    (destructor) MarkerMap_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    &MarkerMap_as_sequence,    /*tp_as_sequence*/
    &MarkerMap_as_map,          /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT,        /*tp_flags*/
    "Map of JSON string key indices",        /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    MarkerMap_methods,         /* tp_methods */
    0,                         /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)MarkerMap_init,      /* tp_init */
    0,                             /* tp_alloc */
    MarkerMap_new,                 /* tp_new */
};

static PyObject *MarkerMap_new(PyTypeObject *type, PyObject *args, PyObject *kwargs)
{
    MarkerMap *self;
    self = (MarkerMap*) type->tp_alloc(type, 0);
    if(self != NULL) {
        if(initialize_map(&self->map) == -1) {
            Py_DECREF(self);
            return NULL;
        }
    }
    return (PyObject *)self;
}

static int MarkerMap_init(MarkerMap *self, PyObject *args, PyObject *kwargs)
{
    static char *kwlist[] = {"data", NULL};
    if(!PyArg_ParseTupleAndKeywords(args, kwargs, "O", kwlist, &self->data))
        return -1;

#if PY_MAJOR_VERSION < 3
    if(!PyString_Check(self->data)) {
        PyErr_SetString(PyExc_TypeError, "Must provide a string object");
        self->data = NULL;
        return -1;
    }
    Py_INCREF(self->data);
    self->data_as_str = PyString_AS_STRING(self->data);
    if(scan(&self->map, self->data_as_str, PyString_GET_SIZE(self->data) - 1) == -1) {
        PyErr_SetString(PyExc_ValueError, "Error processing input string - is it valid JSON?");
        return -1;
    }
#else
    // Validate unicode 1byte (i.e. ascii utf-8 only)
    if(! (PyUnicode_Check(self->data) && PyUnicode_READY(self->data) == 0 && PyUnicode_KIND(self->data) == PyUnicode_1BYTE_KIND)) {
        PyErr_SetString(PyExc_TypeError, "Must provide a string object");
        self->data = NULL;
        return -1;
    }
    Py_INCREF(self->data);
    self->data_as_str = PyUnicode_1BYTE_DATA(self->data);
    if(scan(&self->map, self->data_as_str, PyUnicode_GET_LENGTH(self->data) - 1) == -1) {
        PyErr_SetString(PyExc_ValueError, "Error processing input string - is it valid JSON?");
        return -1;
    }
#endif
    return 0;
}

static void MarkerMap_dealloc(MarkerMap *self)
{
    dealloc_map(&self->map);
    Py_DECREF(self->data);
    self->data_as_str = NULL;
}

static Py_ssize_t MarkerMap_len(PyObject *self)
{
    return ((MarkerMap*)self)->map.nmemb;
}

static int MarkerMap_contains(PyObject *self, PyObject *key)
{
    const char *k;
    struct marker *m = NULL;
#if PY_MAJOR_VERSION < 3
    if(!PyString_Check(key)) {
        PyErr_SetString(PyExc_TypeError, "Keys must be string type");
        return -1;
    }
    k = PyString_AS_STRING(key);
#else
    if(!(PyUnicode_Check(key) && PyUnicode_READY(key) == 0 && PyUnicode_KIND(key) == PyUnicode_1BYTE_KIND)) {
        PyErr_SetString(PyExc_TypeError, "Must provide utf-8 encoded string");
        return -1;
    }
    k = PyUnicode_1BYTE_DATA(key);
#endif

    if((m = fetch_marker(&((MarkerMap*)self)->map, ((MarkerMap*)self)->data_as_str, k)) == NULL) {
        return 0;
    }
    return 1;

}

static PyObject *MarkerMap_get_object(PyObject *self, PyObject *key)
{
    const char *k;
    struct marker *m = NULL;
#if PY_MAJOR_VERSION < 3
    if(!PyString_Check(key)) {
        PyErr_SetString(PyExc_TypeError, "Keys must be string type");
        return NULL;
    }
    k = PyString_AS_STRING(key);
#else
    if(!(PyUnicode_Check(key) && PyUnicode_READY(key) == 0 && PyUnicode_KIND(key) == PyUnicode_1BYTE_KIND)) {
        PyErr_SetString(PyExc_TypeError, "Must provide ascii string");
        return NULL;
    }
    k = PyUnicode_1BYTE_DATA(key);
#endif
    if((m = fetch_marker(&((MarkerMap *)self)->map, ((MarkerMap*)self)->data_as_str, k)) == NULL) {
        PyErr_SetString(PyExc_KeyError, "Key not found");
        return NULL;
    }

    if(m->loaded_json) {
        return m->loaded_json;
    }

    m->loaded_json = call_json_loads(((MarkerMap*)self)->data_as_str, m);
    Py_INCREF(m->loaded_json);
    return m->loaded_json;
}

static PyObject *MarkerMap_key_exists(MarkerMap *self, PyObject *args)
{
    const char *k;
    struct marker *m;
    if(!PyArg_ParseTuple(args, "s", &k)) {
        PyErr_SetString(PyExc_TypeError, "Keys must be string type");
        return NULL;
    }

    if((m = fetch_marker(&self->map, self->data_as_str, k)) == NULL) {
        Py_RETURN_FALSE;
    }
    Py_RETURN_TRUE;
}

static PyObject *call_json_loads(const char *data, struct marker *m)
{
    PyObject *arglist, *str, *res;

    str = PyUnicode_FromStringAndSize(data + m->val_start, (m->val_end - m->val_start));
    arglist = Py_BuildValue("(O)", str);
    res = PyObject_CallObject(json_loads, arglist);
    Py_DECREF(arglist);
    Py_DECREF(str);
    return res;
}

#if PY_MAJOR_VERSION >= 3

static struct PyModuleDef mapper_module = {
    PyModuleDef_HEAD_INIT,
    "mapper",
    NULL,
    -1,
    mapper_methods
};

#define INITERROR return NULL
PyMODINIT_FUNC PyInit_mapper(void)
#else
PyMODINIT_FUNC initmapper(void)
#define INITERROR return
#endif
{
    PyObject *module;
    if(PyType_Ready(&MarkerMapType) < 0)
        INITERROR;

#if PY_MAJOR_VERSION >=3
    module = PyModule_Create(&mapper_module);
#else
    module = Py_InitModule("mapper", mapper_methods);
#endif

    if(module == NULL)
        INITERROR;

    json_module = PyImport_ImportModule("json");
    json_loads = PyObject_GetAttr(json_module, PyUnicode_FromString("loads"));

    Py_INCREF(&MarkerMapType);
    PyModule_AddObject(module, "MarkerMap", (PyObject *)&MarkerMapType);

#if PY_MAJOR_VERSION >= 3
    return module;
#endif
}

/*************************** Scan ******************************************/
//TODO: Make the string consumption more resilient

#define CONSUME_STRING(d, p) while((d)[(p)] != '"') { \
                                if((d)[(p)] == '\\') ++(p); \
                                ++(p);} ++(p);

#define CONSUME_LIST(d, p) { int depth = 1;                                         \
                            while(depth) {                                          \
                                switch((d)[(p)++]) {                                \
                                case '"': CONSUME_STRING((d), (p)); break;          \
                                case '[': depth++; break;                           \
                                case ']': depth--; break;                           \
                                }}}

// The work horse. Build that map man!
static int scan(struct marker_map *map, const char *data, size_t len)
{
    struct marker *stack[100] = {0}, *mark;
    size_t pos = 0, prev_pos = 0, stack_ptr = 0;

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
        mark = insert_marker(map, data, prev_pos, pos - 1);
        if(!mark)
            goto err;
        mark->key_start = prev_pos; // dodge "
        mark->key_end = pos - 1;
        //printf("Got key: %.*s\n", (int)(mark->key_end - mark->key_start), data + mark->key_start);
        // Find ':'
        while(isspace(data[pos])) ++pos;
        if(data[pos++] != ':')
            goto err;

        while(isspace(data[pos])) ++pos;
        mark->val_start = pos;
        switch(data[pos++]) {
        case '{':
            // Check empty dict case
            // TODO: Technically there could be empty space between the '{}'
            // But json library doesn't do that... still we should check for it
            if(data[pos] == '}') {
                mark->val_end = pos++;
                break;
            } else {
                stack[stack_ptr++] = mark;
                continue;
            }
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
        //printf("Got val: %.*s\n", (int)(mark->val_end - mark->val_start), data + mark->val_start);
        if(stack_ptr > 0) {
            mark->parent = stack[stack_ptr - 1];
        }
        // Check the terminator
        while(stack_ptr && data[pos] == '}') {
            mark = stack[--stack_ptr];
            mark->val_end = ++pos;
        }
        ++pos;
        //printf("Iterating at %zu of %zu at val %c\n", pos, len, data[pos]);
    }

    return 0;

err:
    return -1;
}
