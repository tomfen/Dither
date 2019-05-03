#include <Python.h>
#include <arrayobject.h>
#include <float.h>

PyDoc_STRVAR(dither_example_doc, "example(obj, number)\
\
Example function");

struct color {
	float a;
	float b;
	float c;
};
typedef struct color color;

color* load_palette(PyArrayObject* npArr, int* size)
{
	int dims;
	npy_intp *shape;
	NpyIter *iter;
	NpyIter_IterNextFunc *iternext;
	char **dataptr;

	dims = PyArray_NDIM(npArr);
	shape = PyArray_SHAPE(npArr);

	if (dims != 2 || shape[1] != 3) {
		PyErr_SetString(PyExc_ValueError, "Pallete array shape must be (n, 3)");
		return NULL;
	}
	

	iter = NpyIter_New(npArr,
		NPY_ITER_READONLY,
		NPY_KEEPORDER,
		NPY_NO_CASTING,
		NULL);

	if (iter == NULL) {
		return NULL;
	}

	iternext = NpyIter_GetIterNext(iter, NULL);
	dataptr = NpyIter_GetDataPtrArray(iter);

	float* ret = calloc(shape[0], sizeof(color));

	int i = 0;
	do {
		ret[i++] = **(float**)dataptr;
	} while (iternext(iter));

	NpyIter_Deallocate(iter);

	*size = (int)shape[0];
	return (color*)ret;
}

void find_closest_color(color* current, color* new, color* error, int* index, color* palette, int paletteSize)
{
	float min_distance = FLT_MAX;
	color min;

	for (int i = 0; i < paletteSize; i++) {
		float distance =
			(palette[i].a - current->a)*(palette[i].a - current->a) +
			(palette[i].b - current->b)*(palette[i].b - current->b) +
			(palette[i].c - current->c)*(palette[i].c - current->c);

		if (distance < min_distance) {
			min_distance = distance;
			min = palette[i];
			*index = i;
		}
	}

	*new = min;
	*error = (color){ current->a - new->a, current->b - new->b , current->c - new->c };
}

void propagate_error(color* src, color* dst, float f)
{
	dst->a += src->a * f;
	dst->b += src->b * f;
	dst->c += src->c * f;
}

PyObject *floyd_steinberg(PyObject *arr, PyObject *args)
{
	PyArrayObject *op[2], *ditheredimg;
	npy_uint32 op_flags[2];

	PyArrayObject *image, *paletteArr;
	int dims;
	npy_intp *shape;
	NpyIter *iter;
	NpyIter_IterNextFunc *iternext;
	color* palette;
	int paletteSize;

	if (!PyArg_ParseTuple(args, "O!O!", &PyArray_Type, &image, &PyArray_Type, &paletteArr))
		return NULL;

	palette = load_palette(paletteArr, &paletteSize);

	if (palette == NULL) {
		return NULL;
	}

	dims = PyArray_NDIM(image);
	shape = PyArray_SHAPE(image);

	if (dims != 3 || shape[2] != 3) {
		PyErr_SetString(PyExc_ValueError, "Input array shape must be (a, b, 3)");
		return NULL;
	}

	op[0] = image;
	op[1] = NULL;
	op_flags[0] = NPY_ITER_READONLY;
	op_flags[1] = NPY_ITER_WRITEONLY | NPY_ITER_ALLOCATE;

	iter = NpyIter_MultiNew(2, op,
		NPY_ITER_MULTI_INDEX,
		NPY_KEEPORDER,
		NPY_NO_CASTING,
		op_flags,
		NULL);

	if (iter == NULL) {
		return NULL;
	}

	NpyIter_RemoveAxis(iter, 2);

	iternext = NpyIter_GetIterNext(iter, NULL);

	uint8_t** dataptrarray = NpyIter_GetDataPtrArray(iter);

	int indexes_arr_size = shape[0] * shape[1] * sizeof(uint8_t);
	npy_intp* indexes_dims = malloc(2 * sizeof(npy_intp));
	indexes_dims[0] = shape[0];
	indexes_dims[1] = shape[1];

	uint8_t* indexes = malloc(indexes_arr_size);
	uint8_t* indexes_ptr = indexes;


	color* qerror_row = calloc(shape[1] + 1, sizeof(color));
	color* qerror_next = calloc(shape[1] + 1, sizeof(color));

	unsigned int x = 0;

	do {
		color current = {
			((float)dataptrarray[0][0]) + qerror_row[x].a,
			((float)dataptrarray[0][1]) + qerror_row[x].b,
			((float)dataptrarray[0][2]) + qerror_row[x].c,
		};

		color new, error;
		int index;

		find_closest_color(&current, &new, &error, &index, palette, paletteSize);

		propagate_error(&error, &qerror_row[x + 1], 7.f / 16.f);
		
		if (x > 0)
			propagate_error(&error, &qerror_next[x - 1], 3 / 16.f);

		propagate_error(&error, &qerror_next[x], 5 / 16.f);

		propagate_error(&error, &qerror_next[x + 1], 1 / 16.f);

		*indexes_ptr++ = index;

		dataptrarray[1][0] = (uint8_t)new.a;
		dataptrarray[1][1] = (uint8_t)new.b;
		dataptrarray[1][2] = (uint8_t)new.c;

		x++;

		if (x >= shape[1]) {
			x = 0;

			color* __temp = qerror_next;
			qerror_next = qerror_row;
			qerror_row = __temp;


			memset(qerror_next, 0, (shape[1] + 1) * sizeof(color));
		}
	} while (iternext(iter));

	free(qerror_next);
	free(qerror_row);
	free(palette);

	/* Get the result from the iterator object array */
	ditheredimg = NpyIter_GetOperandArray(iter)[1];
	Py_INCREF(ditheredimg);

	if (NpyIter_Deallocate(iter) != NPY_SUCCEED) {
		Py_DECREF(ditheredimg);
		return NULL;
	}


	PyArrayObject* indexes_npy = PyArray_SimpleNewFromData(2, indexes_dims, NPY_UINT8, indexes);

	return Py_BuildValue("OO", ditheredimg, indexes_npy);
}

PyObject *closest(PyObject *arr, PyObject *args)
{
	PyArrayObject *op[2], *ditheredimg;
	npy_uint32 op_flags[2];

	PyArrayObject *image, *paletteArr;
	int dims;
	npy_intp *shape;
	NpyIter *iter;
	NpyIter_IterNextFunc *iternext;
	color* palette;
	int paletteSize;

	if (!PyArg_ParseTuple(args, "O!O!", &PyArray_Type, &image, &PyArray_Type, &paletteArr))
		return NULL;

	palette = load_palette(paletteArr, &paletteSize);

	if (palette == NULL) {
		return NULL;
	}

	dims = PyArray_NDIM(image);
	shape = PyArray_SHAPE(image);

	if (dims != 3 || shape[2] != 3) {
		PyErr_SetString(PyExc_ValueError, "Input array shape must be (a, b, 3)");
		return NULL;
	}

	op[0] = image;
	op[1] = NULL;
	op_flags[0] = NPY_ITER_READONLY;
	op_flags[1] = NPY_ITER_WRITEONLY | NPY_ITER_ALLOCATE;

	iter = NpyIter_MultiNew(2, op,
		NPY_ITER_MULTI_INDEX,
		NPY_KEEPORDER,
		NPY_NO_CASTING,
		op_flags,
		NULL);

	if (iter == NULL) {
		return NULL;
	}

	NpyIter_RemoveAxis(iter, 2);

	iternext = NpyIter_GetIterNext(iter, NULL);

	uint8_t** dataptrarray = NpyIter_GetDataPtrArray(iter);

	int indexes_arr_size = shape[0] * shape[1] * sizeof(uint8_t);
	npy_intp* indexes_dims = malloc(2 * sizeof(npy_intp));
	indexes_dims[0] = shape[0];
	indexes_dims[1] = shape[1];

	uint8_t* indexes = malloc(indexes_arr_size);
	uint8_t* indexes_ptr = indexes;

	unsigned int x = 0;

	color error = { 0, 0 ,0 };

	do {
		color current = {
			((float)dataptrarray[0][0]),
			((float)dataptrarray[0][1]),
			((float)dataptrarray[0][2]),
		};

		color new;
		int index;

		find_closest_color(&current, &new, &error, &index, palette, paletteSize);


		*indexes_ptr++ = index;

		dataptrarray[1][0] = (uint8_t)new.a;
		dataptrarray[1][1] = (uint8_t)new.b;
		dataptrarray[1][2] = (uint8_t)new.c;

		x++;

		if (x >= shape[1]) {
			x = 0;
		}
	} while (iternext(iter));

	free(palette);

	/* Get the result from the iterator object array */
	ditheredimg = NpyIter_GetOperandArray(iter)[1];
	Py_INCREF(ditheredimg);

	if (NpyIter_Deallocate(iter) != NPY_SUCCEED) {
		Py_DECREF(ditheredimg);
		return NULL;
	}


	PyArrayObject* indexes_npy = PyArray_SimpleNewFromData(2, indexes_dims, NPY_UINT8, indexes);

	return Py_BuildValue("OO", ditheredimg, indexes_npy);
}

/*
 * List of functions to add to dither in exec_dither().
 */
static PyMethodDef dither_functions[] = {
    { "floyd_steinberg", (PyCFunction)floyd_steinberg, METH_VARARGS, dither_example_doc },
	{ "closest", (PyCFunction)closest, METH_VARARGS, dither_example_doc },
    { NULL, NULL, 0, NULL } /* marks end of array */
};

/*
 * Initialize dither. May be called multiple times, so avoid
 * using static state.
 */
int exec_dither(PyObject *module) {
    PyModule_AddFunctions(module, dither_functions);

    PyModule_AddStringConstant(module, "__author__", "Tomasz");
    PyModule_AddStringConstant(module, "__version__", "0.0.1");

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
    "cdither",
    dither_doc,
    0,              /* m_size */
    NULL,           /* m_methods */
    dither_slots,
    NULL,           /* m_traverse */
    NULL,           /* m_clear */
    NULL,           /* m_free */
};

PyMODINIT_FUNC PyInit_cdither() {
	import_array();
    return PyModuleDef_Init(&dither_def);
}
