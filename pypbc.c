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

// initialize a GMP integer from a Python number
void mpz_init_from_pynum(mpz_t n_mpz, PyObject *n) {
    PyObject *n_unicode = PyNumber_ToBase(n, 10);
    PyObject *n_bytes = PyUnicode_AsASCIIString(n_unicode);
    char *n_str = PyBytes_AsString(n_bytes);
    mpz_init_set_str(n_mpz, n_str, 10);
    Py_DECREF(n_unicode);
    Py_DECREF(n_bytes);
}

// get a Python number from a GMP integer
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
    "A representation of the parameters of an elliptic curve.\n"
    "\n"
    "There are three basic ways to instantiate a Parameters object:\n"
    "\n"
    "Parameters(param_string=s) -> a set of parameters built according to s\n"
    "Parameters(n=x, short=True||False) -> a type F or A1 curve\n"
    "Parameters(qbits=q, rbits=r, short=True||False) -> type E or A curve\n"
    "\n"
    "These objects are essentially only used for creating Pairings.");

Parameters *Parameters_create(void) {
    // allocate the object
    Parameters *params = (Parameters *)ParametersType.tp_alloc(&ParametersType, 0);
    // check if the object was allocated
    if (!params) {
        PyErr_SetString(PyExc_TypeError, "could not create Parameters object");
        return NULL;
    }
    // set the ready flag to 0
    params->ready = 0;
    // return the object
    return params;
}

PyObject *Parameters_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
    // create the object
    return (PyObject *)Parameters_create();
}

void Parameters_dealloc(Parameters *params) {
    // clear the parameters if they're ready
    if (params->ready) {
        pbc_param_clear(params->pbc_params);
    }
    // free the object
    Py_TYPE(params)->tp_free((PyObject *)params);
}

int Parameters_init(Parameters *params, PyObject *args, PyObject *kwargs) {
    // we have a few different ways to initialize the Parameters
    char *kwds[] = {"param_string", "n", "qbits", "rbits", "short", NULL};
    char *param_string = NULL;
    PyObject *n_py = NULL;
    int qbits = 0;
    int rbits = 0;
    PyObject *is_short = NULL;
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|$sOiiO", kwds, &param_string, &n_py, &qbits, &rbits, &is_short)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return -1;
    }
    // check the type of the argument
    if (is_short && !PyBool_Check(is_short)) {
        PyErr_SetString(PyExc_TypeError, "'short' must be a boolean");
        return -1;
    }
    // check the arguments
    if (param_string && !n_py && !qbits && !rbits && !is_short) {
        // initialize the parameters from a string
        pbc_param_init_set_str(params->pbc_params, param_string);
    } else if (n_py && !qbits && !rbits && !param_string) {
        // check the type of the argument
        if (!PyLong_Check(n_py)) {
            PyErr_SetString(PyExc_TypeError, "'n' must be an integer");
            return -1;
        }
        // initialize the parameters from n
        if (is_short == Py_True) {
            // cast the argument
            Py_ssize_t bits = PyNumber_AsSsize_t(n_py, PyExc_OverflowError);
            // initialize a type F curve
            pbc_param_init_f_gen(params->pbc_params, (int)bits);
        } else {
            // convert the argument to an mpz
            mpz_t n_mpz;
            mpz_init_from_pynum(n_mpz, n_py);
            // initialize a type A1 curve
            pbc_param_init_a1_gen(params->pbc_params, n_mpz);
            // clear the mpz
            mpz_clear(n_mpz);
        }
    } else if (qbits && rbits && !n_py && !param_string) {
        // initialize the parameters from qbits and rbits
        if (is_short == Py_True) {
            // initialize a type E curve
            pbc_param_init_e_gen(params->pbc_params, rbits, qbits);
        } else {
            // initialize a type A curve
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

PyObject *Parameters_str(PyObject *params_py) {
    // cast the argument
    Parameters *params = (Parameters *)params_py;
    // declare a buffer
    char buffer[4096];
    // open a file in memory
    FILE *fp = fmemopen((void *)buffer, sizeof(buffer), "w+");
    // check if the file was opened
    if (fp == NULL) {
        PyErr_SetString(PyExc_IOError, "could not write parameters to buffer");
        return NULL;
    }
    // write the parameters to the buffer
    pbc_param_out_str(fp, params->pbc_params);
    // close the file
    fclose(fp);
    // return the buffer as a string
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
    "pypbc.Parameters",                       /* tp_name */
    sizeof(Parameters),                       /* tp_basicsize */
    0,                                        /* tp_itemsize */
    (destructor)Parameters_dealloc,           /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_compare */
    Parameters_str,                           /* tp_repr */
    0,                                        /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    0,                                        /* tp_hash */
    0,                                        /* tp_call */
    Parameters_str,                           /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    Parameters__doc__,                        /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    Parameters_methods,                       /* tp_methods */
    Parameters_members,                       /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    (initproc)Parameters_init,                /* tp_init */
    0,                                        /* tp_alloc */
    Parameters_new,                           /* tp_new */
};

/*******************************************************************************
*                                   Pairings                                   *
*******************************************************************************/

PyDoc_STRVAR(Pairing__doc__,
    "Represents a bilinear pairing, frequently referred to as e-hat.\n"
    "\n"
    "Basic usage:\n"
    "\n"
    "Pairing(params) -> Pairing object\n"
    "\n"
    "This object is used to apply the bilinear map to two elements.");

Pairing *Pairing_create(void) {
    // allocate the object
    Pairing *pairing = (Pairing *)PairingType.tp_alloc(&PairingType, 0);
    // check if the object was allocated
    if (!pairing) {
        PyErr_SetString(PyExc_TypeError, "could not create Pairing object");
        return NULL;
    }
    // set the ready flag to 0
    pairing->ready = 0;
    // return the object
    return pairing;
}

PyObject *Pairing_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    // create the object
    return (PyObject *)Pairing_create();
}

void Pairing_dealloc(Pairing *pairing) {
    // clear the pairing if it's ready
    if (pairing->ready) {
        pairing_clear(pairing->pbc_pairing);
    }
    // free the object
    Py_TYPE(pairing)->tp_free((PyObject *)pairing);
}

int Pairing_init(Pairing *pairing, PyObject *args) {
    // only argument is the parameters
    PyObject *params_py;
    if (!PyArg_ParseTuple(args, "O", &params_py)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return -1;
    }
    // check the type of the argument
    if (!PyObject_TypeCheck(params_py, &ParametersType)) {
        PyErr_SetString(PyExc_TypeError, "expected Parameter, got something else");
        return -1;
    }
    // cast the argument
    Parameters *params = (Parameters *)params_py;
    // use the Parameters to init the pairing
    pairing_init_pbc_param(pairing->pbc_pairing, params->pbc_params);
    // set the ready flag
    pairing->ready = 1;
    return 0;
}

PyObject *Pairing_apply(PyObject *pairing_py, PyObject *args) {
    // we expect two elements
    PyObject *lft_py;
    PyObject *rgt_py;
    if (!PyArg_ParseTuple(args, "OO", &lft_py, &rgt_py)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return NULL;
    }
    // check the types on the arguments
    if (!PyObject_TypeCheck(lft_py, &ElementType) || !PyObject_TypeCheck(rgt_py, &ElementType)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be two Elements");
        return NULL;
    }
    // declare the result element
    Element *res_ele;
    // cast the arguments
    Pairing *pairing = (Pairing *)pairing_py;
    Element *lft_ele = (Element *)lft_py;
    Element *rgt_ele = (Element *)rgt_py;
    // make sure they're in different groups
    if ((lft_ele->group != G1 || rgt_ele->group != G2) && (lft_ele->group != G2 || rgt_ele->group != G1)) {
        PyErr_SetString(PyExc_ValueError, "Elements must be in G1 and G2");
        return NULL;
    }
    // build the result element and compute the pairing
    res_ele = Element_create();
    element_init_GT(res_ele->pbc_element, pairing->pbc_pairing);
    res_ele->group = GT;
    res_ele->pairing = lft_ele->pairing;
    pairing_apply(res_ele->pbc_element, lft_ele->pbc_element, rgt_ele->pbc_element, pairing->pbc_pairing);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(res_ele->pairing);
    res_ele->ready = 1;
    return (PyObject *)res_ele;
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
    "pypbc.Pairing",                          /* tp_name */
    sizeof(Pairing),                          /* tp_basicsize */
    0,                                        /* tp_itemsize */
    (destructor)Pairing_dealloc,              /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_reserved */
    0,                                        /* tp_repr */
    0,                                        /* tp_as_number */
    0,                                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    0,                                        /* tp_hash */
    0,                                        /* tp_call */
    0,                                        /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    Pairing__doc__,                           /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    0,                                        /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    Pairing_methods,                          /* tp_methods */
    Pairing_members,                          /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    (initproc)Pairing_init,                   /* tp_init */
    0,                                        /* tp_alloc */
    Pairing_new,                              /* tp_new */
};

/*******************************************************************************
*                                   Elements                                   *
*******************************************************************************/

PyDoc_STRVAR(Element__doc__,
    "Represents an element of a bilinear group.\n"
    "\n"
    "Basic usage:\n"
    "\n"
    "Element(pairing, Zr, value=int) -> Element\n"
    "Element(pairing, G1||G2||GT||Zr, string=str) -> Element\n"
    "\n"
    "Most of the basic arithmetic operations apply. Please note that many of them\n"
    "do not make sense between groups, and that not all of these are checked for.");

Element *Element_create(void) {
    // allocate the object
    Element *element = (Element *)ElementType.tp_alloc(&ElementType, 0);
    // check if the object was allocated
    if (!element) {
        PyErr_SetString(PyExc_TypeError, "could not create Element object");
        return NULL;
    }
    // set the ready flag to 0
    element->ready = 0;
    // return the object
    return element;
}

PyObject *Element_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
    // create the object
    return (PyObject *)Element_create();
}

void Element_dealloc(Element *element) {
    // clear the element and decrement the reference count on the pairing if it's ready
    if (element->ready){
        element_clear(element->pbc_element);
        Py_DECREF(element->pairing);
    }
    // free the object
    Py_TYPE(element)->tp_free((PyObject *)element);
}

int Element_init(PyObject *element_py, PyObject *args, PyObject *kwargs) {
    // required arguments are the pairing and the group
    PyObject *pairing_py;
    enum Group group;
    char *string = NULL;
    PyObject *val_py = NULL;
    char *keys[] = {"pairing", "group", "value", "string", NULL};
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oi|$Os", keys, &pairing_py, &group, &val_py, &string)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return -1;
    }
    // check the type of arguments
    if (!PyObject_TypeCheck(pairing_py, &PairingType)) {
        PyErr_SetString(PyExc_TypeError, "the 'pairing' argument must be a Pairing object");
        return -1;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)pairing_py;
    Element *element = (Element *)element_py;
    // use the arguments to init the element
    switch (group) {
        case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
        case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
        case GT: element_init_GT(element->pbc_element, pairing->pbc_pairing); break;
        case Zr: element_init_Zr(element->pbc_element, pairing->pbc_pairing); break;
        default: PyErr_SetString(PyExc_ValueError, "invalid group"); return -1;
    }
    element->group = group;
    element->pairing = pairing_py;
    // check the arguments
    if (string && !val_py) {
        // set the element to the string
        element_set_str(element->pbc_element, string, 10);
    } else if (val_py && !string) {
        // check the type of the value
        if (!PyLong_Check(val_py)) {
            PyErr_SetString(PyExc_TypeError, "'value' must be an integer");
            return -1;
        }
        // make sure the group is Zr
        if (group != Zr) {
            PyErr_SetString(PyExc_TypeError, "cannot provide a integer value for a non-Zr group");
            return -1;
        }
        // convert the value to an mpz
        mpz_t mpz_val;
        mpz_init_from_pynum(mpz_val, val_py);
        // set the element to the value
        element_set_mpz(element->pbc_element, mpz_val);
        // clean up the mpz
        mpz_clear(mpz_val);
    } else {
        PyErr_SetString(PyExc_ValueError, "must provide either 'value' or 'string', but not both");
        return -1;
    }
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(pairing_py);
    element->ready = 1;
    return 0;
}

PyObject *Element_zero(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *pairing_py;
    enum Group group;
    if (!PyArg_ParseTuple(args, "Oi", &pairing_py, &group)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return NULL;
    }
    // check the type of arguments
    if (!PyObject_TypeCheck(pairing_py, &PairingType)) {
        PyErr_SetString(PyExc_TypeError, "the 'pairing' argument must be a Pairing object");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)pairing_py;
    // build the result element and initialize it with the pairing and group
    Element *element = Element_create();
    PyObject *element_py = (PyObject *)element;
    switch (group) {
        case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
        case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
        case GT: element_init_GT(element->pbc_element, pairing->pbc_pairing); break;
        case Zr: element_init_Zr(element->pbc_element, pairing->pbc_pairing); break;
        default: Py_DECREF(element_py); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    element->group = group;
    element->pairing = pairing_py;
    // set the element to 0
    element_set0(element->pbc_element);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(pairing_py);
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_one(PyObject *cls, PyObject *args, PyObject *kwargs) {
    // required arguments are the pairing and the group
    PyObject *pairing_py;
    enum Group group;
    if (!PyArg_ParseTuple(args, "Oi", &pairing_py, &group)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return NULL;
    }
    // check the type of arguments
    if (!PyObject_TypeCheck(pairing_py, &PairingType)) {
        PyErr_SetString(PyExc_TypeError, "the 'pairing' argument must be a Pairing object");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)pairing_py;
    // build the result element and initialize it with the pairing and group
    Element *element = Element_create();
    PyObject *element_py = (PyObject *)element;
    switch (group) {
        case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
        case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
        case GT: element_init_GT(element->pbc_element, pairing->pbc_pairing); break;
        case Zr: element_init_Zr(element->pbc_element, pairing->pbc_pairing); break;
        default: Py_DECREF(element_py); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    element->group = group;
    element->pairing = pairing_py;
    // set the element to 1
    element_set1(element->pbc_element);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(pairing_py);
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_random(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *pairing_py;
    enum Group group;
    if (!PyArg_ParseTuple(args, "Oi", &pairing_py, &group)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return NULL;
    }
    // check the type of arguments
    if (!PyObject_TypeCheck(pairing_py, &PairingType)) {
        PyErr_SetString(PyExc_TypeError, "the 'pairing' argument must be a Pairing object");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)pairing_py;
    // build the result element and initialize it with the pairing and group
    Element *element = Element_create();
    PyObject *element_py = (PyObject *)element;
    switch (group) {
        case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
        case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
        case Zr: element_init_Zr(element->pbc_element, pairing->pbc_pairing); break;
        default: Py_DECREF(element_py); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    element->group = group;
    element->pairing = pairing_py;
    // make the element random
    element_random(element->pbc_element);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(pairing_py);
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_from_hash(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *pairing_py;
    enum Group group;
    PyObject *bytes;
    if (!PyArg_ParseTuple(args, "OiO", &pairing_py, &group, &bytes)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return NULL;
    }
    // check the type of arguments
    if (!PyObject_TypeCheck(pairing_py, &PairingType)) {
        PyErr_SetString(PyExc_TypeError, "the 'pairing' argument must be a Pairing object");
        return NULL;
    }
    if (!PyBytes_Check(bytes)) {
        PyErr_SetString(PyExc_TypeError, "the 'bytes' argument must be a bytes object");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)pairing_py;
    // build the result element and initialize it with the pairing and group
    Element *element = Element_create();
    PyObject *element_py = (PyObject *)element;
    switch (group) {
        case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
        case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
        case GT: element_init_GT(element->pbc_element, pairing->pbc_pairing); break;
        case Zr: element_init_Zr(element->pbc_element, pairing->pbc_pairing); break;
        default: Py_DECREF(element_py); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    element->group = group;
    element->pairing = pairing_py;
    // convert the bytes to an element
    int size = PyBytes_Size(bytes);
    unsigned char *string = (unsigned char *)PyBytes_AsString(bytes);
    element_from_hash(element->pbc_element, string, size);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(pairing_py);
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_from_bytes(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *pairing_py;
    enum Group group;
    PyObject *bytes;
    if (!PyArg_ParseTuple(args, "OiO", &pairing_py, &group, &bytes)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return NULL;
    }
    // check the type of arguments
    if (!PyObject_TypeCheck(pairing_py, &PairingType)) {
        PyErr_SetString(PyExc_TypeError, "the 'pairing' argument must be a Pairing object");
        return NULL;
    }
    if (!PyBytes_Check(bytes)) {
        PyErr_SetString(PyExc_TypeError, "the 'bytes' argument must be a bytes object");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)pairing_py;
    // build the result element and initialize it with the pairing and group
    Element *element = Element_create();
    PyObject *element_py = (PyObject *)element;
    switch (group) {
        case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
        case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
        case GT: element_init_GT(element->pbc_element, pairing->pbc_pairing); break;
        case Zr: element_init_Zr(element->pbc_element, pairing->pbc_pairing); break;
        default: Py_DECREF(element_py); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    element->group = group;
    element->pairing = pairing_py;
    // convert the bytes to an element
    unsigned char *string = (unsigned char *)PyBytes_AsString(bytes);
    element_from_bytes(element->pbc_element, string);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(pairing_py);
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_from_bytes_compressed(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *pairing_py;
    enum Group group;
    PyObject *bytes;
    if (!PyArg_ParseTuple(args, "OiO", &pairing_py, &group, &bytes)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return NULL;
    }
    // check the type of arguments
    if (!PyObject_TypeCheck(pairing_py, &PairingType)) {
        PyErr_SetString(PyExc_TypeError, "the 'pairing' argument must be a Pairing object");
        return NULL;
    }
    if (!PyBytes_Check(bytes)) {
        PyErr_SetString(PyExc_TypeError, "the 'bytes' argument must be a bytes object");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)pairing_py;
    // build the result element and initialize it with the pairing and group
    Element *element = Element_create();
    PyObject *element_py = (PyObject *)element;
    switch (group) {
        case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
        case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
        default: Py_DECREF(element_py); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    element->group = group;
    element->pairing = pairing_py;
    // convert the bytes to an element
    unsigned char *string = (unsigned char *)PyBytes_AsString(bytes);
    element_from_bytes_compressed(element->pbc_element, string);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(pairing_py);
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_from_bytes_x_only(PyObject *cls, PyObject *args) {
    // required arguments are the pairing and the group
    PyObject *pairing_py;
    enum Group group;
    PyObject *bytes;
    if (!PyArg_ParseTuple(args, "OiO", &pairing_py, &group, &bytes)) {
        PyErr_SetString(PyExc_TypeError, "could not parse arguments");
        return NULL;
    }
    // check the type of arguments
    if (!PyObject_TypeCheck(pairing_py, &PairingType)) {
        PyErr_SetString(PyExc_TypeError, "the 'pairing' argument must be a Pairing object");
        return NULL;
    }
    if (!PyBytes_Check(bytes)) {
        PyErr_SetString(PyExc_TypeError, "the 'bytes' argument must be a bytes object");
        return NULL;
    }
    // cast the arguments
    Pairing *pairing = (Pairing *)pairing_py;
    // build the result element and initialize it with the pairing and group
    Element *element = Element_create();
    PyObject *element_py = (PyObject *)element;
    switch (group) {
        case G1: element_init_G1(element->pbc_element, pairing->pbc_pairing); break;
        case G2: element_init_G2(element->pbc_element, pairing->pbc_pairing); break;
        default: Py_DECREF(element_py); PyErr_SetString(PyExc_ValueError, "invalid group"); return NULL;
    }
    element->group = group;
    element->pairing = pairing_py;
    // convert the bytes to an element
    unsigned char *string = (unsigned char *)PyBytes_AsString(bytes);
    element_from_bytes_x_only(element->pbc_element, string);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(pairing_py);
    element->ready = 1;
    return (PyObject *)element;
}

PyObject *Element_to_bytes(PyObject *element_py) {
    // cast the argument
    Element *element = (Element *)element_py;
    // get the size of the buffer and allocate it
    int size = element_length_in_bytes(element->pbc_element);
    unsigned char buffer[size];
    // convert the element to bytes
    element_to_bytes(buffer, element->pbc_element);
    // return the buffer as a bytes object
    return PyBytes_FromStringAndSize((char *)buffer, size);
}

PyObject *Element_to_bytes_compressed(PyObject *element_py) {
    // cast the argument
    Element *element = (Element *)element_py;
    // make sure the element is in G1 or G2
    if (element->group != G1 && element->group != G2) {
        PyErr_SetString(PyExc_TypeError, "Element must be in G1 or G2");
        return NULL;
    }
    // get the size of the buffer and allocate it
    int size = element_length_in_bytes_compressed(element->pbc_element);
    unsigned char buffer[size];
    // convert the element to compressed bytes
    element_to_bytes_compressed(buffer, element->pbc_element);
    // return the buffer as a bytes object
    return PyBytes_FromStringAndSize((char *)buffer, size);
}

PyObject *Element_to_bytes_x_only(PyObject *element_py) {
    // cast the argument
    Element *element = (Element *)element_py;
    // make sure the element is in G1 or G2
    if (element->group != G1 && element->group != G2) {
        PyErr_SetString(PyExc_TypeError, "Element must be in G1 or G2");
        return NULL;
    }
    // get the size of the buffer and allocate it
    int size = element_length_in_bytes_x_only(element->pbc_element);
    unsigned char buffer[size];
    // convert the element to x-only bytes
    element_to_bytes_x_only(buffer, element->pbc_element);
    // return the buffer as a bytes object
    return PyBytes_FromStringAndSize((char *)buffer, size);
}

PyObject *Element_add(PyObject *lft_py, PyObject *rgt_py) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft_py, &ElementType) || !PyObject_TypeCheck(rgt_py, &ElementType)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be two Elements");
        return NULL;
    }
    // declare the result element
    Element *res_ele;
    // convert both objects to Elements
    Element *lft_ele = (Element *)lft_py;
    Element *rgt_ele = (Element *)rgt_py;
    // make sure they're in the same ring
    if (lft_ele->group != rgt_ele->group) {
        PyErr_SetString(PyExc_ValueError, "arguments must be two Elements of the same group");
        return NULL;
    }
    // build the result element and initialize it to the same group as the left element
    res_ele = Element_create();
    element_init_same_as(res_ele->pbc_element, lft_ele->pbc_element);
    res_ele->group = lft_ele->group;
    res_ele->pairing = lft_ele->pairing;
    // add the two elements
    element_add(res_ele->pbc_element, lft_ele->pbc_element, rgt_ele->pbc_element);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(res_ele->pairing);
    res_ele->ready = 1;
    return (PyObject *)res_ele;
}

PyObject *Element_sub(PyObject *lft_py, PyObject *rgt_py) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft_py, &ElementType) || !PyObject_TypeCheck(rgt_py, &ElementType)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be two Elements");
        return NULL;
    }
    // declare the result element
    Element *res_ele;
    // convert both objects to Elements
    Element *lft_ele = (Element *)lft_py;
    Element *rgt_ele = (Element *)rgt_py;
    // make sure they're in the same ring
    if (lft_ele->group != rgt_ele->group) {
        PyErr_SetString(PyExc_ValueError, "arguments must be two Elements of the same group");
        return NULL;
    }
    // build the result element and initialize it to the same group as the left element
    res_ele = Element_create();
    element_init_same_as(res_ele->pbc_element, lft_ele->pbc_element);
    res_ele->group = lft_ele->group;
    res_ele->pairing = lft_ele->pairing;
    // subtract the two elements
    element_sub(res_ele->pbc_element, lft_ele->pbc_element, rgt_ele->pbc_element);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(res_ele->pairing);
    res_ele->ready = 1;
    return (PyObject *)res_ele;
}

PyObject *Element_mult(PyObject *lft_py, PyObject *rgt_py) {
    // declare the result element
    Element *res_ele;
    // check the type of arguments
    if (PyObject_TypeCheck(lft_py, &ElementType) && PyObject_TypeCheck(rgt_py, &ElementType)) {
        // convert both objects to Elements
        Element *lft_ele = (Element *)lft_py;
        Element *rgt_ele = (Element *)rgt_py;
        // make sure they're in the same ring or one is in Zr
        if (lft_ele->group == rgt_ele->group) {
            // build the result element and initialize it to the same group as the left element
            res_ele = Element_create();
            element_init_same_as(res_ele->pbc_element, lft_ele->pbc_element);
            res_ele->group = lft_ele->group;
            res_ele->pairing = lft_ele->pairing;
            // multiply the two elements
            element_mul(res_ele->pbc_element, lft_ele->pbc_element, rgt_ele->pbc_element);
            // increment the reference count on the pairing and set the ready flag
            Py_INCREF(res_ele->pairing);
            res_ele->ready = 1;
            return (PyObject *)res_ele;
        } else if (rgt_ele->group == Zr) {
            // build the result element and initialize it to the same group as the left element
            res_ele = Element_create();
            element_init_same_as(res_ele->pbc_element, lft_ele->pbc_element);
            res_ele->group = lft_ele->group;
            res_ele->pairing = lft_ele->pairing;
            // multiply the two elements
            element_mul_zn(res_ele->pbc_element, lft_ele->pbc_element, rgt_ele->pbc_element);
        } else if (lft_ele->group == Zr) {
            // build the result element and initialize it to the same group as the right element
            res_ele = Element_create();
            element_init_same_as(res_ele->pbc_element, rgt_ele->pbc_element);
            res_ele->group = rgt_ele->group;
            res_ele->pairing = rgt_ele->pairing;
            // multiply the two elements
            element_mul_zn(res_ele->pbc_element, rgt_ele->pbc_element, lft_ele->pbc_element);
        } else {
            PyErr_SetString(PyExc_ValueError, "Elements must be in the same group, or one must be in Zr");
            return NULL;
        }
    } else if (PyLong_Check(rgt_py)) {
        // convert the left object to an Element
        Element *lft_ele = (Element *)lft_py;
        // convert the right object to an mpz
        mpz_t mpz_rgt;
        mpz_init_from_pynum(mpz_rgt, rgt_py);
        // build the result element and initialize it to the same group as the left element
        res_ele = Element_create();
        element_init_same_as(res_ele->pbc_element, lft_ele->pbc_element);
        res_ele->group = lft_ele->group;
        res_ele->pairing = lft_ele->pairing;
        // multiply the two elements
        element_mul_mpz(res_ele->pbc_element, lft_ele->pbc_element, mpz_rgt);
        // clean up the mpz
        mpz_clear(mpz_rgt);
    } else if (PyLong_Check(lft_py)) {
        // convert the right object to an Element
        Element *rgt_ele = (Element *)rgt_py;
        // convert the left object to an mpz
        mpz_t mpz_lft;
        mpz_init_from_pynum(mpz_lft, lft_py);
        // build the result element and initialize it to the same group as the right element
        res_ele = Element_create();
        element_init_same_as(res_ele->pbc_element, rgt_ele->pbc_element);
        res_ele->group = rgt_ele->group;
        res_ele->pairing = rgt_ele->pairing;
        // multiply the two elements
        element_mul_mpz(res_ele->pbc_element, rgt_ele->pbc_element, mpz_lft);
        // clean up the mpz
        mpz_clear(mpz_lft);
    } else {
        PyErr_SetString(PyExc_TypeError, "arguments must be two Elements or an Element and an integer");
        return NULL;
    }
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(res_ele->pairing);
    res_ele->ready = 1;
    return (PyObject *)res_ele;
}

PyObject *Element_div(PyObject *lft_py, PyObject *rgt_py) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft_py, &ElementType) || !PyObject_TypeCheck(rgt_py, &ElementType)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be two Elements");
        return NULL;
    }
    // declare the result element
    Element *res_ele;
    // convert both objects to Elements
    Element *lft_ele = (Element *)lft_py;
    Element *rgt_ele = (Element *)rgt_py;
    // make sure they're in the same ring or one is in Zr
    if (lft_ele->group != rgt_ele->group && (rgt_ele->group != Zr || lft_ele->group == GT)) {
        PyErr_SetString(PyExc_ValueError, "Elements must be in the same group, or the right one must be in Zr and the left one in G1 or G2");
        return NULL;
    }
    // build the result element
    res_ele = Element_create();
    element_init_same_as(res_ele->pbc_element, lft_ele->pbc_element);
    res_ele->group = lft_ele->group;
    res_ele->pairing = lft_ele->pairing;
    // divide the two elements
    element_div(res_ele->pbc_element, lft_ele->pbc_element, rgt_ele->pbc_element);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(res_ele->pairing);
    res_ele->ready = 1;
    return (PyObject *)res_ele;
}

PyObject *Element_pow(PyObject *lft_py, PyObject *rgt_py, PyObject *mod_py) {
    // check the type of the first argument
    if (!PyObject_TypeCheck(lft_py, &ElementType)) {
        PyErr_SetString(PyExc_TypeError, "the first argument must be an Element");
        return NULL;
    }
    // declare the result element
    Element *res_ele;
    // convert the first argument to an Element
    Element *lft_ele = (Element *)lft_py;
    // check the type of the second argument
    if (PyObject_TypeCheck(rgt_py, &ElementType)) {
        // convert the second argument to an Element
        Element *rgt_ele = (Element *)rgt_py;
        // make sure the second element is in Zr
        if (rgt_ele->group == Zr) {
            // build the result element
            res_ele = Element_create();
            element_init_same_as(res_ele->pbc_element, lft_ele->pbc_element);
            res_ele->group = lft_ele->group;
            res_ele->pairing = lft_ele->pairing;
            // raise the element to the power
            element_pow_zn(res_ele->pbc_element, lft_ele->pbc_element, rgt_ele->pbc_element);
        } else {
            PyErr_SetString(PyExc_TypeError, "the second Element must be in Zr");
            return NULL;
        }
    } else if (PyLong_Check(rgt_py)) {
        // convert it to an mpz
        mpz_t mpz_lft;
        mpz_init_from_pynum(mpz_lft, rgt_py);
        // build the result element
        res_ele = Element_create();
        element_init_same_as(res_ele->pbc_element, lft_ele->pbc_element);
        res_ele->group = lft_ele->group;
        res_ele->pairing = lft_ele->pairing;
        // raise the element to the power
        element_pow_mpz(res_ele->pbc_element, lft_ele->pbc_element, mpz_lft);
        // clean up the mpz
        mpz_clear(mpz_lft);
    } else {
        PyErr_SetString(PyExc_TypeError, "the second argument must be an Element or an integer");
        return NULL;
    }
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(res_ele->pairing);
    res_ele->ready = 1;
    return (PyObject *)res_ele;
}

PyObject *Element_neg(PyObject *arg_py) {
    // declare the result element
    Element *res_ele;
    // cast the argument
    Element *arg_ele = (Element *)arg_py;
    // make sure we aren't in a bad group
    if (arg_ele->group == GT) {
        PyErr_SetString(PyExc_ValueError, "cannot invert an element in GT");
        return NULL;
    }
    // build the result element
    res_ele = Element_create();
    element_init_same_as(res_ele->pbc_element, arg_ele->pbc_element);
    res_ele->group = arg_ele->group;
    res_ele->pairing = arg_ele->pairing;
    // negate the element
    element_neg(res_ele->pbc_element, arg_ele->pbc_element);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(res_ele->pairing);
    res_ele->ready = 1;
    return (PyObject *)res_ele;
}

PyObject *Element_invert(PyObject *arg_py) {
    // declare the result element
    Element *res_ele;
    // cast the argument
    Element *arg_ele = (Element *)arg_py;
    // make sure we aren't in a bad group
    if (arg_ele->group == GT) {
        PyErr_SetString(PyExc_ValueError, "cannot invert an element in GT");
        return NULL;
    }
    // build the result element and initialize it to the same group as the argument
    res_ele = Element_create();
    element_init_same_as(res_ele->pbc_element, arg_ele->pbc_element);
    res_ele->group = arg_ele->group;
    res_ele->pairing = arg_ele->pairing;
    // invert the element
    element_invert(res_ele->pbc_element, arg_ele->pbc_element);
    // increment the reference count on the pairing and set the ready flag
    Py_INCREF(res_ele->pairing);
    res_ele->ready = 1;
    return (PyObject *)res_ele;
}

PyObject *Element_cmp(PyObject *lft_py, PyObject *rgt_py, int op) {
    // check the type of arguments
    if (!PyObject_TypeCheck(lft_py, &ElementType) || !PyObject_TypeCheck(rgt_py, &ElementType)) {
        PyErr_SetString(PyExc_TypeError, "arguments must be two Elements");
        return NULL;
    }
    // convert both objects to Elements
    Element *lft_ele = (Element *)lft_py;
    Element *rgt_ele = (Element *)rgt_py;
    // make sure they're in the same ring
    if (lft_ele->group != rgt_ele->group) {
        PyErr_SetString(PyExc_ValueError, "arguments must be two Elements of the same group");
        return NULL;
    }
    // compare the two elements
    if (op == Py_EQ) {
        if (element_cmp(lft_ele->pbc_element, rgt_ele->pbc_element)) {
            Py_RETURN_FALSE;
        } else {
            Py_RETURN_TRUE;
        }
    } else if (op == Py_NE) {
        if (element_cmp(lft_ele->pbc_element, rgt_ele->pbc_element)) {
            Py_RETURN_TRUE;
        } else {
            Py_RETURN_FALSE;
        }
    } else {
        PyErr_SetString(PyExc_ValueError, "invalid comparison operator");
        return NULL;
    }
}

PyObject *Element_is0(PyObject *element_py) {
    // cast the argument
    Element *element = (Element *)element_py;
    // check if the element is 0
    if (element_is0(element->pbc_element)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

PyObject *Element_is1(PyObject *element_py) {
    // cast the argument
    Element *element = (Element *)element_py;
    // check if the element is 1
    if (element_is1(element->pbc_element)) {
        Py_RETURN_TRUE;
    } else {
        Py_RETURN_FALSE;
    }
}

Py_ssize_t Element_len(PyObject *element_py) {
    // cast the argument
    Element *element = (Element *)element_py;
    // check if the element is in Zr
    if (element->group == Zr) {
        PyErr_SetString(PyExc_TypeError, "Elements of type Zr are not dimensioned");
        return -1;
    }
    // return the length of the element
    return element_item_count(element->pbc_element);
}

PyObject *Element_item(PyObject *element_py, Py_ssize_t sz_i) {
    // cast the argument
    Element *element = (Element *)element_py;
    // check if the element is in Zr
    if (element->group == Zr) {
        PyErr_SetString(PyExc_ValueError, "Elements of type Zr are not dimensioned");
        return NULL;
    }
    // check if the index is in range
    Py_ssize_t sz_c = element_item_count(element->pbc_element);
    if (sz_i < 0 || sz_i >= sz_c) {
        PyErr_SetString(PyExc_IndexError, "index out of range");
        return NULL;
    }
    // get the item
    element_ptr item = element_item(element->pbc_element, sz_i);
    // get the size of the item and allocate a buffer
    int size = element_length_in_bytes(element->pbc_element);
    unsigned char buffer[size];
    // convert the item to bytes
    element_to_bytes(buffer, item);
    // allocate a buffer for the hex string
    char strbuf[4096];
    // convert the bytes to a hex string
    for (int i = 0; i < size; i++) {
        sprintf(&strbuf[2*i], "%02X", buffer[i]);
    }
    strbuf[2*size] = '\0';
    // return the hex string
    return PyLong_FromString(strbuf, NULL, 16);
}

PyObject *Element_int(PyObject *element_py) {
    // cast the argument
    Element *element = (Element *)element_py;
    // check if the element is in Zr
    if (element->group != Zr) {
        PyErr_SetString(PyExc_ValueError, "cannot convert multidimensional point to int");
        return NULL;
    }
    // get the size of the buffer and allocate it
    int size = element_length_in_bytes(element->pbc_element);
    unsigned char buffer[size];
    // convert the element to bytes
    element_to_bytes(buffer, element->pbc_element);
    // allocate a buffer for the hex string
    char strbuf[4096];
    // convert the bytes to a hex string
    for (int i = 0; i < size; i++) {
        sprintf(&strbuf[2*i], "%02X", buffer[i]);
    }
    strbuf[2*size] = '\0';
    // return the hex string
    return PyLong_FromString(strbuf, NULL, 16);
}

PyObject *Element_str(PyObject *element_py) {
    // cast the argument
    Element *element = (Element *)element_py;
    // allocate a buffer
    char strbuf[4096];
    // convert the element to a string
    int size = element_snprint(strbuf, sizeof(strbuf), element->pbc_element);
    // return the string
    return PyUnicode_FromStringAndSize(strbuf, size);
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
    Element_add,               // binaryfunc nb_add;
    Element_sub,               // binaryfunc nb_subtract;
    Element_mult,              // binaryfunc nb_multiply;
    0,                         // binaryfunc nb_remainder;
    0,                         // binaryfunc nb_divmod;
    Element_pow,               // ternaryfunc nb_power;
    (unaryfunc)Element_neg,    // unaryfunc nb_negative;
    0,                         // unaryfunc nb_positive;
    0,                         // unaryfunc nb_absolute;
    0,                         // inquiry nb_bool;
    (unaryfunc)Element_invert, // unaryfunc nb_invert;
    0,                         // binaryfunc nb_lshift;
    0,                         // binaryfunc nb_rshift;
    0,                         // binaryfunc nb_and;
    0,                         // binaryfunc nb_xor;
    0,                         // binaryfunc nb_or;
    (unaryfunc)Element_int,    // unaryfunc nb_int;
    0,                         // void *nb_reserved;
    0,                         // unaryfunc nb_float;
    0,                         // binaryfunc nb_inplace_add;
    0,                         // binaryfunc nb_inplace_subtract;
    0,                         // binaryfunc nb_inplace_multiply;
    0,                         // binaryfunc nb_inplace_remainder;
    0,                         // ternaryfunc nb_inplace_power;
    0,                         // binaryfunc nb_inplace_lshift;
    0,                         // binaryfunc nb_inplace_rshift;
    0,                         // binaryfunc nb_inplace_and;
    0,                         // binaryfunc nb_inplace_xor;
    0,                         // binaryfunc nb_inplace_or;
    0,                         // binaryfunc nb_floor_divide;
    Element_div,               // binaryfunc nb_true_divide;
    0,                         // binaryfunc nb_inplace_floor_divide;
    0,                         // binaryfunc nb_inplace_true_divide;
};

PySequenceMethods Element_sq_meths = {
    Element_len,               // inquiry sq_length;
    0,                         // binaryfunc sq_concat;
    0,                         // intargfunc sq_repeat;
    Element_item,              // intargfunc sq_item;
    0,                         // intintargfunc sq_slice;
    0,                         // intobjargproc sq_ass_item;
    0,                         // intintobjargproc sq_ass_slice
};

PyTypeObject ElementType = {
    PyVarObject_HEAD_INIT(NULL, 0)
    "pypbc.Element",                          /* tp_name */
    sizeof(Element),                          /* tp_basicsize */
    0,                                        /* tp_itemsize */
    (destructor)Element_dealloc,              /* tp_dealloc */
    0,                                        /* tp_print */
    0,                                        /* tp_getattr */
    0,                                        /* tp_setattr */
    0,                                        /* tp_reserved */
    Element_str,                              /* tp_repr */
    &Element_num_meths,                       /* tp_as_number */
    &Element_sq_meths,                        /* tp_as_sequence */
    0,                                        /* tp_as_mapping */
    0,                                        /* tp_hash */
    0,                                        /* tp_call */
    Element_str,                              /* tp_str */
    0,                                        /* tp_getattro */
    0,                                        /* tp_setattro */
    0,                                        /* tp_as_buffer */
    Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /* tp_flags */
    Element__doc__,                           /* tp_doc */
    0,                                        /* tp_traverse */
    0,                                        /* tp_clear */
    Element_cmp,                              /* tp_richcompare */
    0,                                        /* tp_weaklistoffset */
    0,                                        /* tp_iter */
    0,                                        /* tp_iternext */
    Element_methods,                          /* tp_methods */
    Element_members,                          /* tp_members */
    0,                                        /* tp_getset */
    0,                                        /* tp_base */
    0,                                        /* tp_dict */
    0,                                        /* tp_descr_get */
    0,                                        /* tp_descr_set */
    0,                                        /* tp_dictoffset */
    (initproc)Element_init,                   /* tp_init */
    0,                                        /* tp_alloc */
    Element_new,                              /* tp_new */
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
    if (PyType_Ready(&ParametersType) < 0) {
        return NULL;
    }
    if (PyType_Ready(&PairingType) < 0) {
        return NULL;
    }
    if (PyType_Ready(&ElementType) < 0) {
        return NULL;
    }
    // create the module
    PyObject *module = PyModule_Create(&pypbc_module);
    if (module == NULL) {
        return NULL;
    }
    // increment the reference count for the types
    Py_INCREF(&PairingType);
    Py_INCREF(&ParametersType);
    Py_INCREF(&ElementType);
    // add the types to the module
    PyModule_AddObject(module, "Parameters", (PyObject *)&ParametersType);
    PyModule_AddObject(module, "Pairing", (PyObject *)&PairingType);
    PyModule_AddObject(module, "Element", (PyObject *)&ElementType);
    // add the group constants
    PyModule_AddObject(module, "G1", PyLong_FromLong(G1));
    PyModule_AddObject(module, "G2", PyLong_FromLong(G2));
    PyModule_AddObject(module, "GT", PyLong_FromLong(GT));
    PyModule_AddObject(module, "Zr", PyLong_FromLong(Zr));
    return module;
}
