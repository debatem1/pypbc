// python stuff
#ifndef PY_SSIZE_T_CLEAN
    #define PY_SSIZE_T_CLEAN
#endif
#include <Python.h>
#include "structmember.h"

// pbc stuff
#include <pbc/pbc.h>

/*******************************************************************************
pypbc.h

Written by Geremy Condra
Licensed under GPLv3
Released 11 October 2009

This header contains the declarations for the functions needed to use PBC
from Python3
*******************************************************************************/

#ifndef PYPBC_H
#define PYPBC_H

// we need debugging symbols for compile warnings
#define PBC_DEBUG

// used to see which group a given element is in
enum Group {G1, G2, GT, Zr};

// We're going to need a few types
// the param type
typedef struct {
    PyObject_HEAD
    pbc_param_t pbc_params;
    int ready;
} Parameters;

PyObject *Parameters_new(PyTypeObject *type, PyObject *args, PyObject *kwds);
int Parameters_init(Parameters *self, PyObject *args, PyObject *kwargs);
void Parameters_dealloc(Parameters *parameter);

PyMemberDef Parameters_members[];
PyMethodDef Parameters_methods[];
PyTypeObject ParametersType;

// the pairing type
typedef struct {
    PyObject_HEAD
    pairing_t pbc_pairing;
    int ready;
} Pairing;

PyObject *Pairing_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
int Pairing_init(Pairing *self, PyObject *args);
void Pairing_dealloc(Pairing *pairing);
PyObject* Pairing_apply(PyObject *self, PyObject *args);

PyMemberDef Pairing_members[];
PyMethodDef Pairing_methods[];
PyTypeObject PairingType;

// the element type
typedef struct {
    PyObject_HEAD
    enum Group group;
    PyObject *pairing;
    element_t pbc_element;
    int ready;
} Element;

PyMemberDef Element_members[];
PyMethodDef Element_methods[];
PyTypeObject ElementType;

PyObject *Element_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
int Element_init(PyObject *self, PyObject *args, PyObject *kwargs);
void Element_dealloc(Element *element);

#endif
