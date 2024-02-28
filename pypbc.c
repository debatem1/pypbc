#include "pypbc.h"
#include <stdio.h>

/*******************************************************************************
* pypbc.c                                                                      *
*                                                                              *
* Modifications by Jemtaly                                                     *
* Copyright (c) 2024                                                           *
*                                                                              *
* Modifications by Joseph deBlaquiere                                          *
* Copyright (c) 2017                                                           *
*                                                                              *
* Originally written by Geremy Condra                                          *
* Licensed under GPLv3                                                         *
* Released 11 October 2009                                                     *
*                                                                              *
* This file contains the types and functions needed to use PBC from Python3.   *
*******************************************************************************/

PyDoc_STRVAR(mpz_init_from_pynum__doc__,
    "Converts a Python long type to a GMP MPZ type.");
void mpz_init_from_pynum(mpz_t n_mpz, PyObject *n) {
    PyObject *n_unicode = PyNumber_ToBase(n, 10);
    PyObject *n_bytes = PyUnicode_AsASCIIString(n_unicode);
    char *n_str = PyBytes_AsString(n_bytes);
    mpz_init_set_str(n_mpz, n_str, 10);
    Py_DECREF(n_unicode);
    Py_DECREF(n_bytes);
}

PyDoc_STRVAR(mpz_to_pynum__doc__,
    "Converts a GMP MPZ type to a Python long.");
PyObject *mpz_to_pynum(mpz_t n_mpz) {
    char *n_str = mpz_get_str(NULL, 10, n_mpz);
    PyObject *n = PyLong_FromString(n_str, NULL, 10);
    free(n_str);
    return n;
}

/*******************************************************************************
*                                    Params                                    *
*******************************************************************************/

PyDoc_STRVAR(Parameters__doc__,
"A representation of the parameters of an elliptic curve.\n\
\n\
There are three basic ways to instantiate a Parameters object:\n\
\n\
Parameters(param_string=s) -> a set of parameters built according to s.\n\
Parameters(n=x, short=True/False) -> a type A1 or F curve.\n\
Parameters(qbits=q, rbits=r, short=True/False) -> type A or E curve.\n\
\n\
These objects are essentially only used for creating Pairings.");

// allocate the object
PyObject *Parameters_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    Parameters *params = (Parameters *)type->tp_alloc(type, 0);
    if (!params) {
        PyErr_SetString(PyExc_TypeError, "could not create Parameters object");
        return NULL;
    }
    params->ready = 0;
    return (PyObject *)params;
}

// deallocates the object when done
void Parameters_dealloc(Parameters *params) {
    // clear the internal parameters
    if (params->ready) {
        pbc_param_clear(params->pbc_params);
    }
    // free the actual object
    Py_TYPE(params)->tp_free((PyObject *)params);
}

// Parameters(param_string=str, n=long, qbits=long, rbits=long, short=True/False) -> Parameters
int Parameters_init(Parameters *params, PyObject *args, PyObject *kwargs) {
    // we have a few different ways to initialize the Parameters
    char *kwds[] = {"param_string", "n", "qbits", "rbits", "short", NULL};
    char *param_string = NULL;
    PyObject *n = NULL;
    int qbits = 0;
    int rbits = 0;
    PyObject *is_short = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|$sOiiO", kwds, &param_string, &n, &qbits, &rbits, &is_short)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return -1;
    }
    // check the types on the arguments
    if (is_short && !PyBool_Check(is_short)) {
        PyErr_SetString(PyExc_TypeError, "'short' must be a boolean");
        return -1;
    }
    // use the arguments to init the parameters
    if (param_string && !n && !qbits && !rbits && !is_short) {
        pbc_param_init_set_str(params->pbc_params, param_string);
    } else if (n && !qbits && !rbits && !param_string) {
        if (!PyLong_Check(n)) {
            PyErr_SetString(PyExc_TypeError, "'n' must be an integer");
            return -1;
        }
        if (is_short == Py_True) {
            Py_ssize_t bits = PyNumber_AsSsize_t(n, PyExc_OverflowError);
            pbc_param_init_f_gen(params->pbc_params, (int)bits);
        } else {
            mpz_t n_mpz;
            mpz_init_from_pynum(n_mpz, n);
            pbc_param_init_a1_gen(params->pbc_params, n_mpz);
            mpz_clear(n_mpz);
        }
    } else if (qbits && rbits && !n && !param_string) {
        if (is_short == Py_True) {
            pbc_param_init_e_gen(params->pbc_params, rbits, qbits);
        } else {
            pbc_param_init_a_gen(params->pbc_params, rbits, qbits);
        }
    } else {
        PyErr_SetString(PyExc_ValueError, "too many or too few arguments");
        return -1;
    }
    // set the ready flag
    params->ready = 1;
    return 0;
}

// Parameters(param_string=str, n=long, qbits=long, rbits=long, short=True/False) -> Parameters
PyObject *Parameters_str(PyObject *py_params) {
    Parameters *params = (Parameters *)py_params;
    char buffer[4096];
    // open a file-like object to write to
    FILE *fp = fmemopen((void *)buffer, sizeof(buffer), "w+");
    if (fp == NULL) {
        PyErr_SetString(PyExc_IOError, "could not write parameters to buffer");
        return NULL;
    }
    pbc_param_out_str(fp, params->pbc_params);
    fclose(fp);
    return PyUnicode_FromString(buffer);
}

PyMemberDef Parameters_members[] = {
    {NULL},
};

PyMethodDef Parameters_methods[] = {
    {NULL},
};

PyTypeObject ParametersType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pypbc.Parameters",             /*tp_name*/
    sizeof(Parameters),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Parameters_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,                         /*tp_compare*/
    Parameters_str,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    Parameters_str,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    Parameters__doc__,           /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    Parameters_methods,             /* tp_methods */
    Parameters_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Parameters_init,      /* tp_init */
    0,                         /* tp_alloc */
    Parameters_new,                 /* tp_new */
};

/*******************************************************************************
*                                   Pairings                                   *
*******************************************************************************/

PyDoc_STRVAR(Pairing__doc__,
"Represents a bilinear pairing, frequently referred to as e-hat.\n\
\n\
Basic usage:\n\
\n\
Pairing(parameters) -> Pairing object\n\
\n\
This object is used to apply the bilinear map to two elements.");

// allocate the object
PyObject *Pairing_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    // create the new pairing object
    Pairing *pairing = (Pairing *)type->tp_alloc(type, 0);
    if (!pairing) {
        PyErr_SetString(PyExc_TypeError, "could not create Pairing object");
        return NULL;
    }
    pairing->ready = 0;
    return (PyObject *)pairing;
}

// deallocates the object when done
void Pairing_dealloc(Pairing *pairing) {
    // clear the internal pairing
    if (pairing->ready) {
        pairing_clear(pairing->pbc_pairing);
    }
    // free the actual object
    Py_TYPE(pairing)->tp_free((PyObject *)pairing);
}

// Pairing(params) -> Pairing
int Pairing_init(Pairing *pairing, PyObject *args) {
    // only argument is the parameters
    PyObject *py_params;
    if (!PyArg_ParseTuple(args, "O", &py_params)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return -1;
    }
    // check the types on the arguments
    if (!PyObject_TypeCheck(py_params, &ParametersType)) {
        PyErr_SetString(PyExc_TypeError, "expected Parameter, got something else");
        return -1;
    }
    // cast the arguments
    Parameters *params = (Parameters *)py_params;
    // use the Parameters to init the pairing
    pairing_init_pbc_param(pairing->pbc_pairing, params->pbc_params);
    // set the ready flag
    pairing->ready = 1;
    return 0;
}

// applies the bilinear map action
PyObject *Pairing_apply(PyObject *py_pairing, PyObject *args) {
    // we expect two elements
    PyObject *py_lft;
    PyObject *py_rgt;
    if (!PyArg_ParseTuple(args, "OO", &py_lft, &py_rgt)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return NULL;
    }
    // check the types on the arguments
    if (!PyObject_TypeCheck(py_lft, &ElementType) || !PyObject_TypeCheck(py_rgt, &ElementType)) {
        PyErr_SetString(PyExc_TypeError, "expected Element, got something else");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)py_pairing;
    Element *elem_lft = (Element *)py_lft;
    Element *elem_rgt = (Element *)py_rgt;
    // make sure they're in different groups
    if ((elem_lft->group != G1 || elem_rgt->group != G2) && (elem_lft->group != G2 || elem_rgt->group != G1)) {
        PyErr_SetString(PyExc_ValueError, "arguments must be one Element from G1 and one from G2");
        return NULL;
    }
    // build the result element and compute the pairing
    Element *elem_res = (Element *)ElementType.tp_alloc(&ElementType, 0);
    element_init_GT(elem_res->pbc_element, pairing->pbc_pairing);
    pairing_apply(elem_res->pbc_element, elem_lft->pbc_element, elem_rgt->pbc_element, pairing->pbc_pairing);
    // set the group and pairing
    elem_res->group = GT;
    Py_INCREF(elem_lft->pairing);
    elem_res->pairing = elem_lft->pairing;
    elem_res->ready = 1;
    return (PyObject *)elem_res;
}

PyMemberDef Pairing_members[] = {
    {NULL},
};

PyMethodDef Pairing_methods[] = {
    {"apply", Pairing_apply, METH_VARARGS, "Applies the pairing."},
    {NULL},
};

PyTypeObject PairingType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pypbc.Pairing",             /*tp_name*/
    sizeof(Pairing),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Pairing_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,               /*tp_reserved*/
    0,                         /*tp_repr*/
    0,                         /*tp_as_number*/
    0,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    0,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    Pairing__doc__,           /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    0,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    Pairing_methods,             /* tp_methods */
    Pairing_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Pairing_init,      /* tp_init */
    0,                         /* tp_alloc */
    Pairing_new,                 /* tp_new */
};

/*******************************************************************************
*                                   Elements                                   *
*******************************************************************************/

PyDoc_STRVAR(Element__doc__,
"Represents an element of a bilinear group.\n\
\n\
Basic usage:\n\
\n\
Element(pairing, G1||G2||GT||Zr, value=v) -> Element\n\
\n\
Most of the basic arithmetic operations apply. Please note that many of them\n\
do not make sense between groups, and that not all of these are checked for.");

Element *Element_create(void) {
    Element *element = (Element *)(&ElementType)->tp_alloc(&ElementType, 0);
    if (!element) {
        PyErr_SetString(PyExc_TypeError, "could not create Element object");
        return NULL;
    }
    element->pairing = NULL;
    element->ready = 0;
    return element;
}

// allocate the object
PyObject *Element_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    Element *element = Element_create();
    return (PyObject *)element;
}

// deallocates the object when done
void Element_dealloc(Element *element) {
    // clear the internal element
    if (element->ready){
        element_clear(element->pbc_element);
    }
    // decref the pairing
    Py_XDECREF(element->pairing);
    // free the actual object
    Py_TYPE(element)->tp_free((PyObject *)element);
}

// Element(pairing, group, value=Element/long) -> Element
int Element_init(PyObject *py_element, PyObject *args, PyObject *kwargs) {
    // required arguments are the pairing and the group
    PyObject *py_pairing;
    enum Group group;
    char *string = NULL;
    PyObject *py_val = NULL;
    char *keys[] = {"pairing", "group", "value", "string", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oi|$Os", keys, &py_pairing, &group, &py_val, &string)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return -1;
    }
    // check the type of arguments
    if (!PyObject_TypeCheck(py_pairing, &PairingType)) {
        PyErr_SetString(PyExc_TypeError, "the 'pairing' argument must be a Pairing object");
        return -1;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)py_pairing;
    Element *element = (Element *)py_element;
    // use the arguments to init the element
    switch (group) {
        case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
        case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
        case GT: element_init_GT(element->pbc_element, pairing->pbc_pairing); break;
        case Zr: element_init_Zr(element->pbc_element, pairing->pbc_pairing); break;
        default: PyErr_SetString(PyExc_ValueError, "invalid group"); return -1;
    }
    // set the value of the element
    if (!string && !py_val) {
        element_set0(element->pbc_element);
    } else if (string && !py_val) {
        element_set_str(element->pbc_element, string, 10);
    } else if (!string && py_val) {
        if (PyLong_Check(py_val)) {
            if (group != Zr) {
                PyErr_SetString(PyExc_TypeError, "cannot provide a integer value for a non-Zr group");
                return -1;
            }
            mpz_t mpz_val;
            mpz_init_from_pynum(mpz_val, py_val);
            element_set_mpz(element->pbc_element, mpz_val);
            mpz_clear(mpz_val);
        } else if (PyObject_TypeCheck(py_val, &ElementType)) {
            Element *elem_val = (Element *)py_val;
            if (elem_val->group != group) {
                PyErr_SetString(PyExc_TypeError, "cannot provide an Element of a different group");
                return -1;
            }
            element_set(element->pbc_element, elem_val->pbc_element);
        } else {
            PyErr_SetString(PyExc_TypeError, "'value' must be an Element or an integer");
            return -1;
        }
    } else {
        PyErr_SetString(PyExc_TypeError, "cannot provide both 'value' and 'string'");
        return -1;
    }
    // set the group and pairing
    element->group = group;
    Py_INCREF(py_pairing);
    element->pairing = py_pairing;
    element->ready = 1;
    return 0;
}

PyObject *Element_zero(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *py_pairing;
    enum Group group;
    if (!PyArg_ParseTuple(args, "Oi", &py_pairing, &group)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return NULL;
    }
    // check the type of arguments
    if (!PyObject_TypeCheck(py_pairing, &PairingType)) {
        PyErr_SetString(PyExc_TypeError, "'pairing' must be a Pairing object");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)py_pairing;
    Element *element = Element_create();
    PyObject *py_element = (PyObject *)element;
    // use the arguments to init the element
    switch (group) {
        case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
        case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
        case GT: element_init_GT(element->pbc_element, pairing->pbc_pairing); break;
        case Zr: element_init_Zr(element->pbc_element, pairing->pbc_pairing); break;
        default: Py_DECREF(py_element); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    // set the element to 0
    element_set0(element->pbc_element);
    // set the group and pairing
    element->group = group;
    Py_INCREF(py_pairing);
    element->pairing = py_pairing;
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_one(PyObject *cls, PyObject *args, PyObject *kwargs) {
    // required arguments are the pairing and the group
    PyObject *py_pairing;
    enum Group group;
    if (!PyArg_ParseTuple(args, "Oi", &py_pairing, &group)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return NULL;
    }
    // check the type of arguments
    if (!PyObject_TypeCheck(py_pairing, &PairingType)) {
        PyErr_SetString(PyExc_TypeError, "'pairing' must be a Pairing object");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)py_pairing;
    Element *element = Element_create();
    PyObject *py_element = (PyObject *)element;
    // use the arguments to init the element
    switch (group) {
        case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
        case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
        case GT: element_init_GT(element->pbc_element, pairing->pbc_pairing); break;
        case Zr: element_init_Zr(element->pbc_element, pairing->pbc_pairing); break;
        default: Py_DECREF(py_element); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    // set the element to 1
    element_set1(element->pbc_element);
    // set the group and pairing
    element->group = group;
    Py_INCREF(py_pairing);
    element->pairing = py_pairing;
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_random(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *py_pairing;
    enum Group group;
    if (!PyArg_ParseTuple(args, "Oi", &py_pairing, &group)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return NULL;
    }
    // check the type of arguments
    if (!PyObject_TypeCheck(py_pairing, &PairingType)) {
        PyErr_SetString(PyExc_TypeError, "'pairing' must be a Pairing object");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)py_pairing;
    Element *element = Element_create();
    PyObject *py_element = (PyObject *)element;
    // use the arguments to init the element
    switch (group) {
        case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
        case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
        case Zr: element_init_Zr(element->pbc_element, pairing->pbc_pairing); break;
        default: Py_DECREF(py_element); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    // make the element random
    element_random(element->pbc_element);
    // set the group and pairing
    element->group = group;
    Py_INCREF(py_pairing);
    element->pairing = py_pairing;
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_from_hash(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *py_pairing;
    enum Group group;
    PyObject *bytes;
    if (!PyArg_ParseTuple(args, "OiO", &py_pairing, &group, &bytes)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return NULL;
    }
    // check the type of arguments
    if (!PyObject_TypeCheck(py_pairing, &PairingType)) {
        PyErr_SetString(PyExc_TypeError, "'pairing' must be a Pairing object");
        return NULL;
    }
    if (!PyBytes_Check(bytes)) {
        PyErr_SetString(PyExc_TypeError, "'bytes' must be a bytes object");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)py_pairing;
    Element *element = Element_create();
    PyObject *py_element = (PyObject *)element;
    // use the arguments to init the element
    switch (group) {
        case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
        case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
        case GT: element_init_GT(element->pbc_element, pairing->pbc_pairing); break;
        case Zr: element_init_Zr(element->pbc_element, pairing->pbc_pairing); break;
        default: Py_DECREF(py_element); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    // convert the bytes to an element
    int length = PyBytes_Size(bytes);
    unsigned char *buffer = (unsigned char *)PyBytes_AsString(bytes);
    element_from_hash(element->pbc_element, buffer, length);
    // set the group and pairing
    element->group = group;
    Py_INCREF(py_pairing);
    element->pairing = py_pairing;
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_from_bytes(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *py_pairing;
    enum Group group;
    PyObject *bytes;
    if (!PyArg_ParseTuple(args, "OiO", &py_pairing, &group, &bytes)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return NULL;
    }
    // check the type of arguments
    if (!PyObject_TypeCheck(py_pairing, &PairingType)) {
        PyErr_SetString(PyExc_TypeError, "'pairing' must be a Pairing object");
        return NULL;
    }
    if (!PyBytes_Check(bytes)) {
        PyErr_SetString(PyExc_TypeError, "'bytes' must be a bytes object");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)py_pairing;
    Element *element = Element_create();
    PyObject *py_element = (PyObject *)element;
    // use the arguments to init the element
    switch (group) {
        case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
        case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
        case GT: element_init_GT(element->pbc_element, pairing->pbc_pairing); break;
        case Zr: element_init_Zr(element->pbc_element, pairing->pbc_pairing); break;
        default: Py_DECREF(py_element); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    // convert the bytes to an element
    unsigned char *buffer = (unsigned char *)PyBytes_AsString(bytes);
    element_from_bytes(element->pbc_element, buffer);
    // set the group and pairing
    element->group = group;
    Py_INCREF(py_pairing);
    element->pairing = py_pairing;
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_from_bytes_compressed(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *py_pairing;
    enum Group group;
    PyObject *bytes;
    if (!PyArg_ParseTuple(args, "OiO", &py_pairing, &group, &bytes)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return NULL;
    }
    // check the type of arguments
    if (!PyObject_TypeCheck(py_pairing, &PairingType)) {
        PyErr_SetString(PyExc_TypeError, "'pairing' must be a Pairing object");
        return NULL;
    }
    if (!PyBytes_Check(bytes)) {
        PyErr_SetString(PyExc_TypeError, "'bytes' must be a bytes object");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)py_pairing;
    Element *element = Element_create();
    PyObject *py_element = (PyObject *)element;
    // use the arguments to init the element
    switch (group) {
        case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
        case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
        default: Py_DECREF(py_element); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    // convert the bytes to an element
    unsigned char *buffer = (unsigned char *)PyBytes_AsString(bytes);
    element_from_bytes_compressed(element->pbc_element, buffer);
    // set the group and pairing
    element->group = group;
    Py_INCREF(py_pairing);
    element->pairing = py_pairing;
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_from_bytes_x_only(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *py_pairing;
    enum Group group;
    PyObject *bytes;
    if (!PyArg_ParseTuple(args, "OiO", &py_pairing, &group, &bytes)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return NULL;
    }
    // check the type of arguments
    if (!PyObject_TypeCheck(py_pairing, &PairingType)) {
        PyErr_SetString(PyExc_TypeError, "'pairing' must be a Pairing object");
        return NULL;
    }
    if (!PyBytes_Check(bytes)) {
        PyErr_SetString(PyExc_TypeError, "'bytes' must be a bytes object");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)py_pairing;
    Element *element = Element_create();
    PyObject *py_element = (PyObject *)element;
    // use the arguments to init the element
    switch (group) {
        case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
        case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
        default: Py_DECREF(py_element); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    // convert the bytes to an element
    unsigned char *buffer = (unsigned char *)PyBytes_AsString(bytes);
    element_from_bytes_x_only(element->pbc_element, buffer);
    // set the group and pairing
    element->group = group;
    Py_INCREF(py_pairing);
    element->pairing = py_pairing;
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_to_bytes(PyObject *py_element, PyObject *args) {
    Element *ele = (Element *)py_element;
    int length = element_length_in_bytes(ele->pbc_element);
    unsigned char buffer[length];
    element_to_bytes(buffer, ele->pbc_element);
    return PyBytes_FromStringAndSize((char *)buffer, length);
}

PyObject *Element_to_bytes_compressed(PyObject *py_element, PyObject *args) {
    Element *ele = (Element *)py_element;
    if (ele->group != G1 && ele->group != G2) {
        PyErr_SetString(PyExc_TypeError, "Element must be in G1 or G2");
        return NULL;
    }
    int length = element_length_in_bytes_compressed(ele->pbc_element);
    unsigned char buffer[length];
    element_to_bytes_compressed(buffer, ele->pbc_element);
    return PyBytes_FromStringAndSize((char *)buffer, length);
}

PyObject *Element_to_bytes_x_only(PyObject *py_element, PyObject *args) {
    Element *ele = (Element *)py_element;
    if (ele->group != G1 && ele->group != G2) {
        PyErr_SetString(PyExc_TypeError, "Element must be in G1 or G2");
        return NULL;
    }
    int length = element_length_in_bytes_x_only(ele->pbc_element);
    unsigned char buffer[length];
    element_to_bytes_x_only(buffer, ele->pbc_element);
    return PyBytes_FromStringAndSize((char *)buffer, length);
}

// adds two elements together
PyObject *Element_add(PyObject *py_lft, PyObject *py_rgt) {
    // check the type of arguments
    if (!PyObject_TypeCheck(py_lft, &ElementType) || !PyObject_TypeCheck(py_rgt, &ElementType)) {
        PyErr_SetString(PyExc_TypeError, "expected Element, got something else");
        return NULL;
    }
    // convert both objects to Elements
    Element *elem_lft = (Element *)py_lft;
    Element *elem_rgt = (Element *)py_rgt;
    // make sure they're in the same ring
    if (elem_lft->group != elem_rgt->group) {
        PyErr_SetString(PyExc_ValueError, "arguments must be two Elements of the same group");
        return NULL;
    }
    // build the result element
    Element *elem_res = Element_create();
    element_init_same_as(elem_res->pbc_element, elem_lft->pbc_element);
    element_add(elem_res->pbc_element, elem_lft->pbc_element, elem_rgt->pbc_element);
    // set the group and pairing
    elem_res->group = elem_lft->group;
    Py_INCREF(elem_lft->pairing);
    elem_res->pairing = elem_lft->pairing;
    elem_res->ready = 1;
    return (PyObject *)elem_res;
}

// subtracts two elements
PyObject *Element_sub(PyObject *py_lft, PyObject *py_rgt) {
    // check the type of arguments
    if (!PyObject_TypeCheck(py_lft, &ElementType) || !PyObject_TypeCheck(py_rgt, &ElementType)) {
        PyErr_SetString(PyExc_TypeError, "expected Element, got something else");
        return NULL;
    }
    // convert both objects to Elements
    Element *elem_lft = (Element *)py_lft;
    Element *elem_rgt = (Element *)py_rgt;
    // make sure they're in the same ring
    if (elem_lft->group != elem_rgt->group) {
        PyErr_SetString(PyExc_ValueError, "arguments must be two Elements of the same group");
        return NULL;
    }
    // build the result element
    Element *elem_res = Element_create();
    element_init_same_as(elem_res->pbc_element, elem_lft->pbc_element);
    element_sub(elem_res->pbc_element, elem_lft->pbc_element, elem_rgt->pbc_element);
    // set the group and pairing
    elem_res->group = elem_lft->group;
    Py_INCREF(elem_lft->pairing);
    elem_res->pairing = elem_lft->pairing;
    elem_res->ready = 1;
    return (PyObject *)elem_res;
}

// multiplies two elements
// note that elements from any ring can be multiplied by those in Zr.
PyObject *Element_mult(PyObject *py_lft, PyObject *py_rgt) {
    // convert a to an element
    int lft_is_elem = PyObject_TypeCheck(py_lft, &ElementType);
    int rgt_is_elem = PyObject_TypeCheck(py_rgt, &ElementType);
    if (lft_is_elem && rgt_is_elem) {
        // build the result element
        Element *elem_lft = (Element *)py_lft;
        Element *elem_rgt = (Element *)py_rgt;
        if (elem_lft->group == elem_rgt->group) {
            Element *elem_res = (Element *)ElementType.tp_alloc(&ElementType, 0);
            element_init_same_as(elem_res->pbc_element, elem_lft->pbc_element);
            element_mul(elem_res->pbc_element, elem_lft->pbc_element, elem_rgt->pbc_element);
            // set the group and pairing
            elem_res->group = elem_lft->group;
            Py_INCREF(elem_lft->pairing);
            elem_res->pairing = elem_lft->pairing;
            elem_res->ready = 1;
            return (PyObject *)elem_res;
        } else if (elem_rgt->group == Zr) {
            Element *elem_res = (Element *)ElementType.tp_alloc(&ElementType, 0);
            element_init_same_as(elem_res->pbc_element, elem_lft->pbc_element);
            element_mul_zn(elem_res->pbc_element, elem_lft->pbc_element, elem_rgt->pbc_element);
            // set the group and pairing
            elem_res->group = elem_lft->group;
            Py_INCREF(elem_lft->pairing);
            elem_res->pairing = elem_lft->pairing;
            elem_res->ready = 1;
            return (PyObject *)elem_res;
        } else if (elem_lft->group == Zr) {
            Element *elem_res = (Element *)ElementType.tp_alloc(&ElementType, 0);
            element_init_same_as(elem_res->pbc_element, elem_rgt->pbc_element);
            element_mul_zn(elem_res->pbc_element, elem_rgt->pbc_element, elem_lft->pbc_element);
            // set the group and pairing
            elem_res->group = elem_rgt->group;
            Py_INCREF(elem_rgt->pairing);
            elem_res->pairing = elem_rgt->pairing;
            elem_res->ready = 1;
            return (PyObject *)elem_res;
        }
    } else if (lft_is_elem && PyLong_Check(py_rgt)) {
        Element *elem_lft = (Element *)py_lft;
        mpz_t mpz_rgt;
        mpz_init_from_pynum(mpz_rgt, py_rgt);
        Element *elem_res = (Element *)ElementType.tp_alloc(&ElementType, 0);
        element_init_same_as(elem_res->pbc_element, elem_lft->pbc_element);
        element_mul_mpz(elem_res->pbc_element, elem_lft->pbc_element, mpz_rgt);
        mpz_clear(mpz_rgt);
        // set the group and pairing
        elem_res->group = elem_lft->group;
        Py_INCREF(elem_lft->pairing);
        elem_res->pairing = elem_lft->pairing;
        elem_res->ready = 1;
        return (PyObject *)elem_res;
    } else if (rgt_is_elem && PyLong_Check(py_lft)) {
        Element *elem_rgt = (Element *)py_rgt;
        mpz_t mpz_lft;
        mpz_init_from_pynum(mpz_lft, py_lft);
        Element *elem_res = (Element *)ElementType.tp_alloc(&ElementType, 0);
        element_init_same_as(elem_res->pbc_element, elem_rgt->pbc_element);
        element_mul_mpz(elem_res->pbc_element, elem_rgt->pbc_element, mpz_lft);
        mpz_clear(mpz_lft);
        // set the group and pairing
        elem_res->group = elem_rgt->group;
        Py_INCREF(elem_rgt->pairing);
        elem_res->pairing = elem_rgt->pairing;
        elem_res->ready = 1;
        return (PyObject *)elem_res;
    }
    PyErr_SetString(PyExc_TypeError, "arguments must be two Elements of the same group, an Element and an Element in Zr, or an Element and an integer");
    return NULL;
}

// divide element a by element py_rgt
PyObject *Element_div(PyObject *py_lft, PyObject *py_rgt) {
    // check the type of arguments
    if (!PyObject_TypeCheck(py_lft, &ElementType) || !PyObject_TypeCheck(py_rgt, &ElementType)) {
        PyErr_SetString(PyExc_TypeError, "expected Element, got something else");
        return NULL;
    }
    // convert both objects to Elements
    Element *elem_lft = (Element *)py_lft;
    Element *elem_rgt = (Element *)py_rgt;
    // make sure they're in the same ring
    if (elem_lft->group != elem_rgt->group && (elem_rgt->group != Zr || elem_lft->group == GT)) {
        PyErr_SetString(PyExc_ValueError, "arguments must be two Elements of the same group, or an Element in G1 or G2 and an Element in Zr");
        return NULL;
    }
    // build the result element
    Element *elem_res = (Element *)ElementType.tp_alloc(&ElementType, 0);
    // note that the result is in the same ring *and pairing*
    element_init_same_as(elem_res->pbc_element, elem_lft->pbc_element);
    // add the elements and store the result in elem_res
    element_div(elem_res->pbc_element, elem_lft->pbc_element, elem_rgt->pbc_element);
    // set the group and pairing
    elem_res->group = elem_lft->group;
    Py_INCREF(elem_lft->pairing);
    elem_res->pairing = elem_lft->pairing;
    elem_res->ready = 1;
    return (PyObject *)elem_res;
}

// raises element a to the power of b
PyObject *Element_pow(PyObject *py_lft, PyObject *py_rgt, PyObject *py_mod) {
    // check the type of arguments
    if (!PyObject_TypeCheck(py_lft, &ElementType)) {
        PyErr_SetString(PyExc_TypeError, "expected Element, got something else");
        return NULL;
    }
    // convert a to a pbc type
    Element *elem_lft = (Element *)py_lft;
    if (PyLong_Check(py_rgt)) {
        // convert it to an mpz
        mpz_t mpz_lft;
        mpz_init_from_pynum(mpz_lft, py_rgt);
        Element *elem_res = (Element *)ElementType.tp_alloc(&ElementType, 0);
        element_init_same_as(elem_res->pbc_element, elem_lft->pbc_element);
        element_pow_mpz(elem_res->pbc_element, elem_lft->pbc_element, mpz_lft);
        mpz_clear(mpz_lft);
        // set the group and pairing
        elem_res->group = elem_lft->group;
        Py_INCREF(elem_lft->pairing);
        elem_res->pairing = elem_lft->pairing;
        elem_res->ready = 1;
        return (PyObject *)elem_res;
    } else if (PyObject_TypeCheck(py_rgt, &ElementType)) {
        Element *elem_rgt = (Element *)py_rgt;
        if (elem_rgt->group == Zr) {
            Element *elem_res = (Element *)ElementType.tp_alloc(&ElementType, 0);
            element_init_same_as(elem_res->pbc_element, elem_lft->pbc_element);
            element_pow_zn(elem_res->pbc_element, elem_lft->pbc_element, elem_rgt->pbc_element);
            // set the group and pairing
            elem_res->group = elem_lft->group;
            Py_INCREF(elem_lft->pairing);
            elem_res->pairing = elem_lft->pairing;
            elem_res->ready = 1;
            return (PyObject *)elem_res;
        }
    }
    PyErr_SetString(PyExc_TypeError, "argument 2 must be an integer or an element in Zr");
    return NULL;
}

// returns -a
PyObject *Element_neg(PyObject *py_arg) {
    // cast it
    Element *elem_arg = (Element *)py_arg;
    // make sure we aren't in a bad group
    if (elem_arg->group == GT) {
        PyErr_SetString(PyExc_ValueError, "cannot invert an element in GT");
        return NULL;
    }
    // build the result element
    Element *elem_res = Element_create();
    element_init_same_as(elem_res->pbc_element, elem_arg->pbc_element);
    element_neg(elem_res->pbc_element, elem_arg->pbc_element);
    // set the group and pairing
    elem_res->group = elem_arg->group;
    Py_INCREF(elem_arg->pairing);
    elem_res->pairing = elem_arg->pairing;
    elem_res->ready = 1;
    // cast and return
    return (PyObject *)elem_res;
}

// returns a**-1
PyObject *Element_invert(PyObject *py_arg) {
    // cast it
    Element *elem_arg = (Element *)py_arg;
    // make sure we aren't in a bad group
    if (elem_arg->group == GT) {
        PyErr_SetString(PyExc_ValueError, "cannot invert an element in GT");
        return NULL;
    }
    // build the result element
    Element *elem_res = Element_create();
    // perform the neg op
    element_invert(elem_res->pbc_element, elem_arg->pbc_element);
    element_init_same_as(elem_res->pbc_element, elem_arg->pbc_element);
    // set the group and pairing
    elem_res->group = elem_arg->group;
    Py_INCREF(elem_arg->pairing);
    elem_res->pairing = elem_arg->pairing;
    elem_res->ready = 1;
    // cast and return
    return (PyObject *)elem_res;
}

PyObject *Element_cmp(PyObject *py_lft, PyObject *py_rgt, int op) {
    // check the type of arguments
    if (!PyObject_TypeCheck(py_lft, &ElementType) || !PyObject_TypeCheck(py_rgt, &ElementType)) {
        PyErr_SetString(PyExc_TypeError, "expected Element, got something else");
        return NULL;
    }
    Element *elem_lft = (Element *)py_lft;
    Element *elem_rgt = (Element *)py_rgt;
    if (elem_lft->group != elem_rgt->group) {
        PyErr_SetString(PyExc_ValueError, "arguments must be two Elements of the same group");
        return NULL;
    }
    if (op == Py_EQ) {
        if (element_cmp(elem_lft->pbc_element, elem_rgt->pbc_element)) {
            Py_RETURN_FALSE;
        } else {
            Py_RETURN_TRUE;
        }
    } else if (op == Py_NE) {
        if (element_cmp(elem_lft->pbc_element, elem_rgt->pbc_element)) {
            Py_RETURN_TRUE;
        } else {
            Py_RETURN_FALSE;
        }
    } else {
        PyErr_SetString(PyExc_ValueError, "invalid comparison operator");
        return NULL;
    }
}

PyObject *Element_is0(PyObject *py_element) {
    Element *element = (Element *)py_element;
    if (element_is0(element->pbc_element)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

PyObject *Element_is1(PyObject *py_element) {
    Element *element = (Element *)py_element;
    if (element_is1(element->pbc_element)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

// returns dimension of Element (PBC convention is zero for Zr)
Py_ssize_t Element_len(PyObject *py_element) {
    Element *element = (Element *)py_element;
    if (element->group == Zr) {
        PyErr_SetString(PyExc_TypeError, "Elements of type Zr are not dimensioned");
        return -1;
    }
    return element_item_count(element->pbc_element);
}

// returns python Integer representation for ith item of Element (error for Zr)
PyObject *Element_GetItem(PyObject *py_lft, Py_ssize_t si) {
    unsigned char string[4096];
    char hex_value[4096];
    Element *element = (Element *)py_lft;
    if (element->group == Zr) {
        PyErr_SetString(PyExc_ValueError, "Elements of type Zr are not dimensioned");
        return NULL;
    }
    // query the element dimension
    Py_ssize_t sd = element_item_count(element->pbc_element);
    if (si < 0 || si >= sd) {
        PyErr_SetString(PyExc_ValueError, "index out of range");
        return NULL;
    }
    element_ptr temp = element_item(element->pbc_element, si);
    int size = element_to_bytes(string, temp);
    sprintf(&hex_value[0], "0x");
    for (int i = 0; i < size; i++) {
        sprintf(&hex_value[2 * i], "%02X", string[i]);
    }
    hex_value[2 * size] = '\0';
    return PyLong_FromString(hex_value, NULL, 16);
}

// returns python Integer representation for Element (only for Zr)
PyObject *Element_int(PyObject *py_element) {
    // build the string buffer- AIEEE! MAGIC CONSTANT!
    unsigned char string[4096];
    char hex_value[4096];
    Element *element = (Element *)py_element;
    if (element->group != Zr) {
        PyErr_SetString(PyExc_ValueError, "cannot convert multidimensional point to int");
        return NULL;
    }
    int size = element_to_bytes(string, element->pbc_element);
    sprintf(&hex_value[0], "0x");
    for (int i = 0; i < size; i++) {
        sprintf(&hex_value[2 * i], "%02X", string[i]);
    }
    hex_value[2 * size] = '\0';
    return PyLong_FromString(hex_value, NULL, 16);
}

// converts the element to a string
PyObject *Element_str(PyObject *py_element) {
    Element *element = (Element *)py_element;
    char string[4096];
    // convert the element to a string
    int size = element_snprint(string, sizeof(string), element->pbc_element);
    return PyUnicode_FromStringAndSize(string, size);
}

PyMemberDef Element_members[] = {
    {NULL},
};

PyMethodDef Element_methods[] = {
    {"from_hash", (PyCFunction)Element_from_hash, METH_VARARGS | METH_CLASS, "Creates an Element from the given hash value."},
    {"random", (PyCFunction)Element_random, METH_VARARGS | METH_CLASS, "Creates a random element from the given group."},
    {"zero", (PyCFunction)Element_zero, METH_VARARGS | METH_CLASS, "Creates an element representing the additive identity for its group."},
    {"one", (PyCFunction)Element_one, METH_VARARGS | METH_CLASS, "Creates an element representing the multiplicative identity for its group."},
    {"to_bytes", (PyCFunction)Element_to_bytes, METH_NOARGS, "Converts the element to a byte string."},
    {"to_bytes_x_only", (PyCFunction)Element_to_bytes_x_only, METH_NOARGS, "Converts the element to a byte string using the x-only format."},
    {"to_bytes_compressed", (PyCFunction)Element_to_bytes_compressed, METH_NOARGS, "Converts the element to a byte string using the compressed format."},
    {"from_bytes", (PyCFunction)Element_from_bytes, METH_VARARGS | METH_CLASS, "Creates an element from a byte string."},
    {"from_bytes_compressed", (PyCFunction)Element_from_bytes_compressed, METH_VARARGS | METH_CLASS, "Creates an element from a byte string using the compressed format."},
    {"from_bytes_x_only", (PyCFunction)Element_from_bytes_x_only, METH_VARARGS | METH_CLASS, "Creates an element from a byte string using the x-only format."},
    {"is0", (PyCFunction)Element_is0, METH_NOARGS, "Returns True if the element is 0."},
    {"is1", (PyCFunction)Element_is1, METH_NOARGS, "Returns True if the element is 1."},
    {NULL},
};

PyNumberMethods Element_num_meths = {
    Element_add,        //binaryfunc nb_add;
    Element_sub,        //binaryfunc nb_subtract;
    Element_mult,        //binaryfunc nb_multiply;
    0,                //binaryfunc nb_remainder;
    0,                //binaryfunc nb_divmod;
    Element_pow,        //ternaryfunc nb_power;
    (unaryfunc)Element_neg,        //unaryfunc nb_negative;
    0,                //unaryfunc nb_positive;
    0,                //unaryfunc nb_absolute;
    0,                //inquiry nb_bool;
    (unaryfunc)Element_invert,    //unaryfunc nb_invert;
    0,                //binaryfunc nb_lshift;
    0,                //binaryfunc nb_rshift;
    0,                //binaryfunc nb_and;
    0,                //binaryfunc nb_xor;
    0,                //binaryfunc nb_or;
    (unaryfunc)Element_int,        //unaryfunc nb_int;
    0,                //void *nb_reserved;
    0,                //unaryfunc nb_float;
    0,                //binaryfunc nb_inplace_add;
    0,                //binaryfunc nb_inplace_subtract;
    0,                //binaryfunc nb_inplace_multiply;
    0,                //binaryfunc nb_inplace_remainder;
    0,                //ternaryfunc nb_inplace_power;
    0,                //binaryfunc nb_inplace_lshift;
    0,                //binaryfunc nb_inplace_rshift;
    0,                //binaryfunc nb_inplace_and;
    0,                //binaryfunc nb_inplace_xor;
    0,                //binaryfunc nb_inplace_or;
    0,                //binaryfunc nb_floor_divide;
    Element_div,        //binaryfunc nb_true_divide;
    0,                //binaryfunc nb_inplace_floor_divide;
    0,                //binaryfunc nb_inplace_true_divide;
};

PySequenceMethods Element_sq_meths = {
    Element_len,       /* inquiry sq_length;             /* __len__ */
    0,    /* binaryfunc sq_concat;          /* __add__ */
    0,    /* intargfunc sq_repeat;          /* __mul__ */
    Element_GetItem,   /* intargfunc sq_item;            /* __getitem__ */
    0,  /* intintargfunc sq_slice;        /* __getslice__ */
    0,   /* intobjargproc sq_ass_item;     /* __setitem__ */
    0,  /* intintobjargproc sq_ass_slice; /* __setslice__ */
};

PyTypeObject ElementType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pypbc.Element",             /*tp_name*/
    sizeof(Element),             /*tp_basicsize*/
    0,                         /*tp_itemsize*/
    (destructor)Element_dealloc, /*tp_dealloc*/
    0,                         /*tp_print*/
    0,                         /*tp_getattr*/
    0,                         /*tp_setattr*/
    0,               /*tp_reserved*/
    Element_str,                         /*tp_repr*/
    &Element_num_meths,                         /*tp_as_number*/
    &Element_sq_meths,                         /*tp_as_sequence*/
    0,                         /*tp_as_mapping*/
    0,                         /*tp_hash */
    0,                         /*tp_call*/
    Element_str,                         /*tp_str*/
    0,                         /*tp_getattro*/
    0,                         /*tp_setattro*/
    0,                         /*tp_as_buffer*/
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
    Element__doc__,           /* tp_doc */
    0,                       /* tp_traverse */
    0,                       /* tp_clear */
    Element_cmp,                       /* tp_richcompare */
    0,                       /* tp_weaklistoffset */
    0,                       /* tp_iter */
    0,                       /* tp_iternext */
    Element_methods,             /* tp_methods */
    Element_members,             /* tp_members */
    0,                         /* tp_getset */
    0,                         /* tp_base */
    0,                         /* tp_dict */
    0,                         /* tp_descr_get */
    0,                         /* tp_descr_set */
    0,                         /* tp_dictoffset */
    (initproc)Element_init,      /* tp_init */
    0,                         /* tp_alloc */
    Element_new,                 /* tp_new */
};

/*******************************************************************************
*                                    Module                                    *
*******************************************************************************/

PyMethodDef pypbc_methods[] = {
    {NULL},
};

PyModuleDef pypbc_module = {
    PyModuleDef_HEAD_INIT,
    "pypbc",
    "pypbc",
    -1,
    pypbc_methods,
};

PyMODINIT_FUNC PyInit_pypbc(void) {
    // initialize the types
    if (PyType_Ready(&ParametersType) < 0)
        return NULL;
    if (PyType_Ready(&PairingType) < 0)
        return NULL;
    if (PyType_Ready(&ElementType) < 0)
        return NULL;
    // create the module
    PyObject *m = PyModule_Create(&pypbc_module);
    if (m == NULL)
        return NULL;
    // increment the reference count for the types
    Py_INCREF(&PairingType);
    Py_INCREF(&ParametersType);
    Py_INCREF(&ElementType);
    // add the types to the module
    PyModule_AddObject(m, "Parameters", (PyObject *)&ParametersType);
    PyModule_AddObject(m, "Pairing", (PyObject *)&PairingType);
    PyModule_AddObject(m, "Element", (PyObject *)&ElementType);
    // add the group constants
    PyModule_AddObject(m, "G1", PyLong_FromLong(G1));
    PyModule_AddObject(m, "G2", PyLong_FromLong(G2));
    PyModule_AddObject(m, "GT", PyLong_FromLong(GT));
    PyModule_AddObject(m, "Zr", PyLong_FromLong(Zr));
    return m;
}
