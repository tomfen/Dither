#include <Python.h>

/*
 * Implements an example function.
 */
PyDoc_STRVAR(dither_example_doc, "example(obj, number)\
\
Example function");

PyObject *dither_example(PyObject *self, PyObject *args, PyObject *kwargs) {
    /* Shared references that do not need Py_DECREF before returning. */
    PyObject *obj = NULL;
    int number = 0;

    /* Parse positional and keyword arguments */
    static char* keywords[] = { "obj", "number", NULL };
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oi", keywords, &obj, &number)) {
        return NULL;
    }

    /* Function implementation starts here */

    if (number < 0) {
        PyErr_SetObject(PyExc_ValueError, obj);
        return NULL;    /* return NULL indicates error */
    }

    Py_RETURN_NONE;
}

/*
 * List of functions to add to dither in exec_dither().
 */
static PyMethodDef dither_functions[] = {
    { "example", (PyCFunction)dither_example, METH_VARARGS | METH_KEYWORDS, dither_example_doc },
    { NULL, NULL, 0, NULL } /* marks end of array */
};

/*
 * Initialize dither. May be called multiple times, so avoid
 * using static state.
 */
int exec_dither(PyObject *module) {
    PyModule_AddFunctions(module, dither_functions);

    PyModule_AddStringConstant(module, "__author__", "Tomek");
    PyModule_AddStringConstant(module, "__version__", "1.0.0");
    PyModule_AddIntConstant(module, "year", 2018);

    return 0; /* success */
}

/*
 * Documentation for dither.
 */
PyDoc_STRVAR(dither_doc, "The dither module");


static PyModuleDef_Slot dither_slots[] = {
    { Py_mod_exec, exec_dither },
    { 0, NULL }
};

static PyModuleDef dither_def = {
    PyModuleDef_HEAD_INIT,
    "dither",
    dither_doc,
    0,              /* m_size */
    NULL,           /* m_methods */
    dither_slots,
    NULL,           /* m_traverse */
    NULL,           /* m_clear */
    NULL,           /* m_free */
};

PyMODINIT_FUNC PyInit_dither() {
    return PyModuleDef_Init(&dither_def);
}
