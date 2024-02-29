#pragma once

// python stuff
#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "structmember.h"

// pbc stuff
#include <pbc/pbc.h>

/*******************************************************************************
* pypbc.c                                                                      *
*                                                                              *
* Modifications by Jemtaly                                                     *
* Copyright (c) 2024                                                           *
*                                                                              *
* Originally written by Geremy Condra                                          *
* Licensed under GPLv3                                                         *
* Released 11 October 2009                                                     *
*                                                                              *
* This file contains the types and functions needed to use PBC from Python3.   *
*******************************************************************************/

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

Parameters *Parameters_create();
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

Pairing *Pairing_create();
PyObject *Pairing_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
int Pairing_init(Pairing *self, PyObject *args);
void Pairing_dealloc(Pairing *pairing);

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

Element *Element_create();
PyMemberDef Element_members[];
PyMethodDef Element_methods[];
PyTypeObject ElementType;

PyObject *Element_new(PyTypeObject *type, PyObject *args, PyObject *kwargs);
int Element_init(PyObject *self, PyObject *args, PyObject *kwargs);
void Element_dealloc(Element *element);
