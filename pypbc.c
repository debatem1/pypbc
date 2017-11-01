#include "pypbc.h"
#include <stdio.h>

/*******************************************************************************
pypbc.c

Modifications by Joseph deBlaquiere
Copyright (c) 2017

Originally written by Geremy Condra
Licensed under GPLv3
Released 11 October 2009

This file contains the types and functions needed to use PBC from Python3.
*******************************************************************************/

long PBC_EC_Compressed = (1);

PyDoc_STRVAR(pynum_to_mpz__doc__, 
	"Converts a Python long type to a GMP MPZ type");
void pynum_to_mpz(PyObject *n, mpz_t new_n) {
	// coerce it into a string
	PyObject *n_unicode = PyNumber_ToBase(n, 10);
	PyObject *n_bytes = PyUnicode_AsASCIIString(n_unicode);
	char *n_char = PyBytes_AsString(n_bytes);

	// build the mpz_t n
	mpz_init_set_str(new_n, n_char, 10);
}

PyDoc_STRVAR(mpz_to_pynum__doc__,
	"Converts a GMP MPZ type to a Python long");
PyObject *mpz_to_pynum(mpz_t n) {
	// get the mpz as a string
	char *s = mpz_get_str(NULL, 10, n);

	// convert the string to a python long
	PyObject *l = PyLong_FromString(s, NULL, 10);
	
	// clean up
	free(s);
	
	// return it
	return l;
}

PyDoc_STRVAR(set_point_format_compressed__doc__,
	"Set option to use compressed (sign + X) point format.");
PyObject *set_point_format_compressed(PyObject *self, PyObject *args) {
	PBC_EC_Compressed = 1;
	return PyLong_FromLong(PBC_EC_Compressed);
}

PyDoc_STRVAR(set_point_format_uncompressed__doc__,
	"Set option to use uncompressed (X,Y) point format.");
PyObject *set_point_format_uncompressed(PyObject *self, PyObject *args) {
	PBC_EC_Compressed = 0;
	return PyLong_FromLong(PBC_EC_Compressed);
}

PyDoc_STRVAR(get_random_prime__doc__,
	"Returns a random prime in the given bitlength.");
PyObject *get_random_prime(PyObject *self, PyObject *args) {
	// gets the number of bits from the args
	int num_bits;
	if (!PyArg_ParseTuple(args, "i", &num_bits)) {
		PyErr_SetString(PyExc_TypeError, "could not parse arguments");
		return NULL;
	}
	
	// create the storage number
	mpz_t p;
	mpz_init(p);

	// get a random n-bit number
	pbc_mpz_randomb(p, num_bits);

	// get the next prime
	mpz_nextprime(p, p);

	// get the mpz as a string
	PyObject *rand_prime = mpz_to_pynum(p);

	// clean up the mpz's
	mpz_clear(p);
	
	return rand_prime;
}

PyDoc_STRVAR(get_random__doc__,
	"Returns a random integer less than the given value.");
PyObject *get_random(PyObject *self, PyObject *args) {
	// gets the number of bits from the args
	PyObject *max;
	if (!PyArg_ParseTuple(args, "O", &max)) {
		PyErr_SetString(PyExc_TypeError, "could not parse arguments");
		return NULL;
	}
	
	// create the storage number
	mpz_t a, b;
	mpz_init(a);
	mpz_init(b);
	
	// cast it to an mpz
	pynum_to_mpz(max, a);
	
	// get a value
	pbc_mpz_random(b, a);
	
	// cast it back to a pylong
	PyObject *lng = mpz_to_pynum(b);
	
	// clean up
	mpz_clear(a);
	mpz_clear(b);
	
	// return it
	return lng;
}

/*******************************************************************************
*						Params							      *
*******************************************************************************/
PyDoc_STRVAR(Parameters__doc__,
"A representation of the parameters of an elliptic curve.\n\n\
There are three basic ways to instantiate a Parameters object:\n\
Parameters(param_string=s) -> a set of parameters built according to s.\n\
Parameters(n=x, short=True/False) -> a type A1 or F curve.\n\
Parameters(qbits=q, rbits=r, short=True/False) -> type A or E curve.\n\
\n\
These objects are essentially only used for creating Pairings.");

// allocate the object
PyObject *Parameters_new(PyTypeObject *type, PyObject *args, PyObject *kwds) {
	// create the new Parameterss object
	Parameters *self = (Parameters *)type->tp_alloc(type, 0);
	self->ready = 0;
	// make sure it actually worked
	if (!self) {
		PyErr_SetString(PyExc_TypeError, "could not create Parameters object.");
		return NULL;
	}
	
	// cast and return
	return (PyObject *)self;
}

// Parameters(param_string=str, n=long, qbits=long, rbits=long, short=True/False) -> Parameters
int Parameters_init(Parameters *self, PyObject *args, PyObject *kwargs) {
	char *kwds[] = {"param_string", "n", "qbits", "rbits", "short", NULL};
	// if the parameters are given as a string
	char *param_string = NULL;
	size_t s_len = 0;
	// for type A1 and F fields, F if short is provided and True
	PyObject *n = NULL;
	// for type A and E fields, E if short is provided and True
	int qbits = 0;
	int rbits = 0;
	// for the above
	PyObject *is_short = NULL;
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "|s#OiiO", kwds, &param_string, &s_len, &n, &qbits, &rbits, &is_short)) {
		PyErr_SetString(PyExc_TypeError, "could not parse arguments");
		return -1;
	}
	
	// we have three basic signatures- the first take n, the second take
	// qbits and rbits, and the third take just a string. I will refer to them
	// as s_type, n_type and qr_type, and the following checks to see 
	// which the poor sap is trying to build, and tries to stop them if they 
	// appear confused.
	int s_type = 0;
	int n_type = 0;
	int qr_type = 0;
	// if just the string is provided, we're s_type
	if (param_string && !n && !qbits && !rbits && !is_short) {
		s_type = 1;
	// if n is provided, and qbits and rbits are not, we're n_type
	} else if (n && !(qbits || rbits)) {
		n_type = 1;
	// if qbits and rbits are provided, and n is not, we're qr_type
	} else if (qbits && rbits && !n) {
		qr_type = 1;
	// poor bastard, they tried
	} else {
		PyErr_SetString(PyExc_ValueError, "Impossible to determine desired curve type, please provide s or n or (qbits and rbits).");
		return -1;
	}
	
	// now we handle s_type curve generation
	if (s_type) {
		pbc_param_init_set_buf(self->pbc_params, param_string, s_len);
	}
	
	// now we handle n_type curve generation
	if (n_type) {
		// check to make sure we got a long
		if (!PyLong_Check(n)) {
			PyErr_SetString(PyExc_TypeError, "Expected long, got something else.");
			return -1;
		}
		// now we see if we got the is_short argument.
		// if not, then we build a type a1 curve.
		// if so, then we build a type f curve.
		if (is_short == Py_True) {
			// convert n to an integer
			size_t bits = PyNumber_AsSsize_t(n, PyExc_OverflowError);
			pbc_param_init_f_gen(self->pbc_params, (int)bits);
		} else {
			// convert n to mpz_t
			mpz_t new_n;
			pynum_to_mpz(n, new_n);
			// build the Parameters
			pbc_param_init_a1_gen(self->pbc_params, new_n);
		}
	}
	
	// now we handle qr_type
	if (qr_type) {
		// now we check the is_short argument, generating A if not and E if so.
		if (is_short == Py_True) {
			pbc_param_init_e_gen(self->pbc_params, rbits, qbits);
		} else {
			pbc_param_init_a_gen(self->pbc_params, rbits, qbits);
		}
	}
	
	// you're ready!
	self->ready = 1;
	// all's clear
	return 0;
}

// Parameters(param_string=str, n=long, qbits=long, rbits=long, short=True/False) -> Parameters
PyObject* Parameters_str(Parameters *parameters) {
	FILE *fp;
	PyObject *param;
	// int size;
	// build the string buffer- AIEEE! MAGIC CONSTANT!
	char buffer[4096];
	// PBC wants to write parameters to a FILE, so we make one
	fp = fmemopen((void *)buffer, sizeof(buffer), "w+");
	// write pairing params into buffer
	if (fp != NULL) {
		pbc_param_out_str(fp, parameters->pbc_params);
		fclose(fp);
		param = PyUnicode_FromString(buffer);
	} else {
		param = PyUnicode_FromString("");
	}
	
	return param;
}

// deallocates the object when done
void Parameters_dealloc(Parameters *parameters) {
	// kill the Parameters
	if(parameters->ready) {
		pbc_param_clear(parameters->pbc_params);
	}
	// free the actual object
	Py_TYPE(parameters)->tp_free((PyObject*)parameters);
}

PyMemberDef Parameters_members[] = {
	{NULL}
};

PyMethodDef Parameters_methods[] = {
	{NULL}
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
	0,		               /* tp_traverse */
	0,		               /* tp_clear */
	0,		               /* tp_richcompare */
	0,		               /* tp_weaklistoffset */
	0,		               /* tp_iter */
	0,		               /* tp_iternext */
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
*						Pairings							      *
*******************************************************************************/

PyDoc_STRVAR(Pairing__doc__,
"Pairing(parameters) -> Pairing object\n\n\
Represents a bilinear pairing, frequently referred to as e-hat.\n");
// allocate the object
PyObject *Pairing_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	// create the new Pairing object
	Pairing *self = (Pairing *)type->tp_alloc(type, 0);
	// you are not prepared!
	self->ready = 0;
	// make sure it actually worked
	if (!self) {
		PyErr_SetString(PyExc_TypeError, "could not create Pairing object.");
		return NULL;
	}
	
	return (PyObject*) self;
}

// Pairing(params) -> Pairing
int Pairing_init(Pairing *self, PyObject *args) {
	// only argument is the Parameters
	PyObject *parameters;
	if (!PyArg_ParseTuple(args, "O", &parameters)) {
		PyErr_SetString(PyExc_TypeError, "could not parse arguments");
		// XXX notice this flags errors on -1, not NULL!
		return -1;
	}
	
	// check to make sure we're getting params
	if(!PyObject_TypeCheck(parameters, &ParametersType)) {
		PyErr_SetString(PyExc_TypeError, "expected Parameter, got something else.");
		return -1;
	}
	
	// cast the Parameters
	Parameters *param = (Parameters*)parameters;
	// use the Parameters to init the pairing
	pairing_init_pbc_param(self->pbc_pairing, param->pbc_params);
	// you're ready
	self->ready = 1;
	// all's clear	
	return 0;
}
		
// deallocates the object when done
void Pairing_dealloc(Pairing *pairing) {
	// kill the pairing element
	if (pairing->ready) {
		pairing_clear(pairing->pbc_pairing);
	}
	// free the actual object
	Py_TYPE(pairing)->tp_free((PyObject*)pairing);
}

// applies the bilinear map action
// pairing.apply(Element e1, Element e2) -> Element e3
PyObject* Pairing_apply(PyObject *self, PyObject *args) {
	// process our arguments
	// we need two elements
	PyObject *element_1;
	PyObject *element_2;
	if (!PyArg_ParseTuple(args, "OO", &element_1, &element_2)) {
		PyErr_SetString(PyExc_TypeError, "could not parse arguments");
		return NULL;
	}
	
	// check the types on the arguments
	if(!PyObject_TypeCheck(element_1, &ElementType)){
		PyErr_SetString(PyExc_TypeError, "expected Element, got something else.");
		return NULL;
	}
	if(!PyObject_TypeCheck(element_2, &ElementType)) {
		PyErr_SetString(PyExc_TypeError, "expected Element, got something else.");
		return NULL;
	}

	// extract the actual elements
	Element *e1 = (Element*)element_1;
	Element *e2 = (Element*)element_2;
	
	// extract the pairing object
	Pairing *p = (Pairing*)self;
	
	// we build a third element to store the outcome
	Element *e3 = (Element *)ElementType.tp_alloc(&ElementType, 0);
	element_init_GT(e3->pbc_element, p->pbc_pairing);
	e3->group = GT;
	
	// and apply the pairing
	pairing_apply(e3->pbc_element, e1->pbc_element, e2->pbc_element, p->pbc_pairing);
	
	// incref the pairing we depend on
	Py_INCREF(p);
	
	// set the pairing
	e3->pairing = e1->pairing;
	
	// mark it ready
	e3->ready = 1;

	// cast and return the object
	return (PyObject*)e3;
}

PyMemberDef Pairing_members[] = {
	{NULL}
};

PyMethodDef Pairing_methods[] = {
	{"apply", Pairing_apply, METH_VARARGS, "applies the pairing."},
	{NULL}
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
	0,			   /*tp_reserved*/
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
	0,		               /* tp_traverse */
	0,		               /* tp_clear */
	0,		               /* tp_richcompare */
	0,		               /* tp_weaklistoffset */
	0,		               /* tp_iter */
	0,		               /* tp_iternext */
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
*						Elements							      *
*******************************************************************************/

PyDoc_STRVAR(Element__doc__,
"Represents an element of a bilinear group.\n\n\
Basic usage:\n\
\n\
Element(pairing, G1||G2||GT||Zr, value=v) -> Element\n\
Element.one(pairing, G1||G2||GT||Zr) -> identity element for the given group.\n\
Element.zero(pairing, G1||G2||GT||Zr) -> identity element for the given group.\n\
Element.random(pairing, G1||G2||GT||Zr) -> random element of the given group.\n\
Element.from_hash(pairing, G1||G2||GT||Zr -> element whose value is determined by the given hash value.\n\
\n\
Most of the basic arithmetic operations apply. Please note that many of them\n\
do not make sense between groups, and that not all of these are checked for.");

Element *Element_create(void) {
	// build ourselves
	Element *self = (Element*)(&ElementType)->tp_alloc(&ElementType, 0);
	if(self == NULL) {
		return NULL;
	}
	self->pairing = NULL;
	self->ready = 0;
	return self;
}
// allocate the object
PyObject *Element_new(PyTypeObject *type, PyObject *args, PyObject *kwargs) {
	// build ourselves
	Element *self = Element_create();
	// cast it and send it on
	return (PyObject*)self;
}

// Element(pairing, group, value=Element/long) -> Element
int Element_init(PyObject *py_self, PyObject *args, PyObject *kwargs) {
	// required arguments are the pairing and the group
	PyObject *pypairing;
	enum Group group;
	int ii;
	// optional value argument
	PyObject *value = NULL;
	char *keys[] = {"pairing", "group", "value", NULL};
	if (!PyArg_ParseTupleAndKeywords(args, kwargs, "Oi|O", keys, &pypairing, &group, &value)) {
		PyErr_SetString(PyExc_TypeError, "could not parse arguments");
		// XXX notice this flags errors on -1, not NULL!
		return -1;
	}
	
	// check the type of arguments
	if(!PyObject_TypeCheck(pypairing, &PairingType)) {
		PyErr_SetString(PyExc_TypeError, "expected Pairing, got something else.");
		return -1;
	}
	
	// build the element pointer
	Element *self = (Element*)py_self;

	// store the pairing and incref it, since we depend on its existence
	Py_INCREF(pypairing);
	self->pairing = pypairing;
	
	// cast the arguments
	Pairing *prepairing = (Pairing*)pypairing;

	// use the arguments to init the element
	switch(group) {
		case G1: element_init_G1(self->pbc_element, prepairing->pbc_pairing); break;
		case G2: element_init_G2(self->pbc_element, prepairing->pbc_pairing); break;
		case GT: element_init_GT(self->pbc_element, prepairing->pbc_pairing); break;
		case Zr: element_init_Zr(self->pbc_element, prepairing->pbc_pairing); break;
		default: return -1;
	}
	
	// set the group argument
	self->group = group;

	// handle the value argument
	if (value != NULL) {
		// if it's a long...
		if (PyLong_Check(value)) {
			// convert it to an mpz
			mpz_t new_n;
			pynum_to_mpz(value, new_n);
			element_set_mpz(self->pbc_element, new_n);
		// if it's another element
		} else if (PyObject_TypeCheck(value, &ElementType)) {
			// set the value
			Element *e = (Element*)value;
			element_set(self->pbc_element, e->pbc_element);
		} else {
			int bytesize;
			char *str;
			Py_ssize_t s_str;
			str = PyUnicode_AsUTF8AndSize(value, &s_str);
			if (str != NULL) {
				unsigned char byteval[4096];
				// printf("parsing string for value \"%s\"\n", string);
				// value is a string, see if encoded EC Point or Fp2 tuple
				if (strncmp(str, "(", 1) == 0) {
					// Fp2 tuple? (x0, x1)
					// unhandled type, fail hard
					PyErr_SetString(PyExc_TypeError, "invalid type for argument 'value'");
					return -1;
				} else if (strncmp(str, "00", 2) == 0) {
					// assume EC Point at infinity
					element_set0(self->pbc_element);
					self->ready = 1;
					return 0;
				} else if (strncmp(str, "02", 2) == 0) {
					// even EC Point?
					// printf("even, compressed EC Point\n");
					bytesize = (s_str >> 1) - 1;
					byteval[bytesize] = 0x00;
				} else if (strncmp(str, "03", 2) == 0) {
					// odd EC Point?
					// printf("odd, compressed EC Point\n");
					bytesize = (s_str >> 1) - 1;
					byteval[bytesize] = 0x01;
				} else if (strncmp(str, "04", 2) == 0) {
					// uncompressed EC Point?
					// printf("uncompressed EC Point"\n);
					bytesize = ((s_str >> 1) - 1) >> 1;
					sscanf(&str[s_str - 1], "%2hhx", &byteval[bytesize]);
					byteval[bytesize] &= 0x01;
				}
				for (ii = 0 ; ii < bytesize ; ii++ ) {
					sscanf(&str[2+(2*ii)], "%2hhx", &byteval[ii]);
				}
				ii = element_from_bytes_compressed(self->pbc_element, byteval);
				// printf("element_from_bytes_compressed returned %d\n", ii);
			} else {
				// unrecognized type, fail hard
				PyErr_SetString(PyExc_TypeError, "invalid type for argument 'value'");
				return -1;
			}
		}
	} else {
		element_set0(self->pbc_element);
	}
	
	// you're ready!
	self->ready = 1;
	// we're clear
	return 0;
}

PyObject *Element_from_hash(PyObject *cls, PyObject *args) {
	// required arguments are the pairing, the group, and the hashed value
	PyObject *pypairing;
	enum Group group;
	char *hash;
	size_t hash_size;
	if (!PyArg_ParseTuple(args, "Ois#", &pypairing, &group, &hash, &hash_size)) {
		PyErr_SetString(PyExc_TypeError, "could not parse arguments");
		return NULL;
	}
	
	// build ourselves
	Element *self = Element_create();

	// check the type of arguments
	if(!PyObject_TypeCheck(pypairing, &PairingType)) {
		PyErr_SetString(PyExc_TypeError, "expected Pairing, got something else.");
		return NULL;
	}
	
	// store the pairing and incref it, since we depend on its existence
	Py_INCREF(pypairing);
	self->pairing = pypairing;
	
	// cast the arguments
	Pairing *prepairing = (Pairing*)pypairing;
	
	// use the arguments to init the element
	switch(group) {
		case G1: element_init_G1(self->pbc_element, prepairing->pbc_pairing); break;
		case G2: element_init_G2(self->pbc_element, prepairing->pbc_pairing); break;
		case GT: element_init_GT(self->pbc_element, prepairing->pbc_pairing); break;
		case Zr: element_init_Zr(self->pbc_element, prepairing->pbc_pairing); break;
		default: PyErr_SetString(PyExc_ValueError, "Invalid group."); return NULL;
	}
	
	// set the group argument
	self->group = group;

	// make the element from the hash
	element_from_hash(self->pbc_element, hash, hash_size);

	// you're ready!
	self->ready = 1;
	// we're clear
	return (PyObject*)self;
}

PyObject *Element_random(PyObject *cls, PyObject *args) {
	assert(cls != NULL);
	// required arguments are the pairing and the group
	PyObject *pypairing;
	enum Group group;
	if (!PyArg_ParseTuple(args, "Oi", &pypairing, &group)) {
		PyErr_SetString(PyExc_TypeError, "could not parse arguments");
		return NULL;
	}

	// check the type of arguments
	if(!PyObject_TypeCheck(pypairing, &PairingType)) {
		PyErr_SetString(PyExc_TypeError, "expected Pairing, got something else.");
		return NULL;
	}

	// build ourselves
	Element *self = Element_create();

	// store the pairing and incref it, since we depend on its existence
	Py_INCREF(pypairing);
	self->pairing = pypairing;
	
	// cast the arguments
	Pairing *prepairing = (Pairing*)pypairing;

	// use the arguments to init the element
	switch(group) {
		case G1: element_init_G1(self->pbc_element, prepairing->pbc_pairing); break;
		case G2: element_init_G2(self->pbc_element, prepairing->pbc_pairing); break;
		case Zr: element_init_Zr(self->pbc_element, prepairing->pbc_pairing); break;
		default: PyErr_SetString(PyExc_ValueError, "Invalid group.");return NULL;
	}

	// set the group argument
	self->group = group;
	
	// make the element random
	element_random(self->pbc_element);
	
	// you're ready!
	self->ready = 1;
	// we're clear
	return (PyObject*)self;
}

PyObject *Element_zero(PyObject *cls, PyObject *args) {
	// required arguments are the pairing and the group
	PyObject *pypairing;
	enum Group group;
	if (!PyArg_ParseTuple(args, "Oi", &pypairing, &group)) {
		PyErr_SetString(PyExc_TypeError, "could not parse arguments");
		return NULL;
	}

	// check the type of arguments
	if(!PyObject_TypeCheck(pypairing, &PairingType)) {
		PyErr_SetString(PyExc_TypeError, "expected Pairing, got something else.");
		return NULL;
	}
	
	// build ourselves
	Element *self = Element_create();

	// store the pairing and incref it, since we depend on its existence
	Py_INCREF(pypairing);
	self->pairing = pypairing;
	
	// cast the arguments
	Pairing *prepairing = (Pairing*)pypairing;
	
	// use the arguments to init the element
	switch(group) {
		case G1: element_init_G1(self->pbc_element, prepairing->pbc_pairing); break;
		case G2: element_init_G2(self->pbc_element, prepairing->pbc_pairing); break;
		case GT: element_init_GT(self->pbc_element, prepairing->pbc_pairing); break;
		case Zr: element_init_Zr(self->pbc_element, prepairing->pbc_pairing); break;
		default: PyErr_SetString(PyExc_ValueError, "Invalid group.");return NULL;
	}
	
	// set the group argument
	self->group = group;
		
	// set the element to 0
	element_set0(self->pbc_element);
	
	// you're ready!
	self->ready = 1;
	// we're clear
	return (PyObject*)self;
}

PyObject *Element_one(PyObject *cls, PyObject *args, PyObject *kwargs) {
	// required arguments are the pairing and the group
	PyObject *pypairing;
	enum Group group;
	if (!PyArg_ParseTuple(args, "Oi", &pypairing, &group)) {
		PyErr_SetString(PyExc_TypeError, "could not parse arguments");
		return NULL;
	}
	
	// check the type of arguments
	if(!PyObject_TypeCheck(pypairing, &PairingType)) {
		PyErr_SetString(PyExc_TypeError, "expected Pairing, got something else.");
		return NULL;
	}
	
	// build ourselves
	Element *self = Element_create();
		
	// store the pairing and incref it, since we depend on its existence
	Py_INCREF(pypairing);
	self->pairing = pypairing;
	
	// cast the arguments
	Pairing *prepairing = (Pairing*)pypairing;
	
	// use the arguments to init the element
	switch(group) {
		case G1: element_init_G1(self->pbc_element, prepairing->pbc_pairing); break;
		case G2: element_init_G2(self->pbc_element, prepairing->pbc_pairing); break;
		case GT: element_init_GT(self->pbc_element, prepairing->pbc_pairing); break;
		case Zr: element_init_Zr(self->pbc_element, prepairing->pbc_pairing); break;
		default: PyErr_SetString(PyExc_ValueError, "Invalid group.");return NULL;
	}

	// set the group argument
	self->group = group;
		
	// set the element to 1
	element_set1(self->pbc_element);
	
	// you're ready!
	self->ready = 1;
	// we're clear
	return (PyObject*)self;
}

// deallocates the object when done
void Element_dealloc(Element *element) {
	// clear the internal element
	if (element->ready){
		element_clear(element->pbc_element);
	}
	// decref the pairing
	Py_XDECREF(element->pairing);
	// free the object
	Py_TYPE(element)->tp_free((PyObject*)element);
}

// converts the element to a string
PyObject *Element_str(PyObject *element) {
	// extract the internal element
	Element *py_ele = (Element*)element;
	PyObject *result = NULL;
	int ii, jj, pad, size = 0;
	// int ii = 0;
	// build the string buffer- AIEEE! MAGIC CONSTANT!
	unsigned char string[4096];
	unsigned char hex_value[4096];
	// query the element dimension
	int dim = element_item_count(py_ele->pbc_element);
	switch(py_ele->group) {
		case G1:
		case G2:
			// printf("G1/G2 point dimension %d\n", element_item_count(py_ele->pbc_element));
			if (element_is0(py_ele->pbc_element) == 1) {
				size = (2 * element_length_in_bytes_compressed(py_ele->pbc_element)) - 1;
				for (ii = 0; ii < size; ii++) {
					string[ii] = 0x00 ;
				}
			} else {
				if (PBC_EC_Compressed) {
					size = element_to_bytes_compressed(&string[1], py_ele->pbc_element);
					string[0] = 0x02 | string[size];
					string[size] = 0;
				} else {
					if (dim != 2) {
						PyErr_SetString(PyExc_ValueError, "Invalid Elliptic Curve Point Dimension.");
						return NULL;
					}
					element_ptr ex, ey;
					string[0] = 0x04;
					size = 1;
					ex = element_item(py_ele->pbc_element, 0);
					ey = element_item(py_ele->pbc_element, 1);
					size += element_to_bytes(&string[size], ex);
					size += element_to_bytes(&string[size], ey);
					string[size] = 0;
				}
			}
			for (ii = 0 ; ii < size; ii++) {
				sprintf((char *)&hex_value[2*ii], "%02X", string[ii]);
			}
			return PyUnicode_FromStringAndSize(hex_value, size * 2);
		case GT:
			// printf("GT vector dimension %d\n", dim);
			size = 0;
			sprintf((char *)&hex_value[0], "(0x");
			pad = 3;
			for (ii = 0; ii < dim; ii++) {
				size = element_to_bytes(string, element_item(py_ele->pbc_element, ii));
				if (ii != 0 ) {
					sprintf((char *)&hex_value[pad], ", 0x") ;
					pad += 4;
				}
				for (jj = 0 ; jj < size; jj++) {
					sprintf((char *)&hex_value[(2*jj)+pad], "%02X", string[jj]);
				}
				pad += 2*size;
			}
			sprintf((char *)&hex_value[pad],")");
			return PyUnicode_FromStringAndSize(hex_value, pad + 1);
		case Zr:
			// printf("Zr vector dimension %d\n", dim);
			size = element_to_bytes(string, py_ele->pbc_element);
			sprintf((char *)&hex_value[0], "0x");
			for (ii = 0 ; ii < size; ii++) {
				sprintf((char *)&hex_value[2*(ii+1)], "%02X", string[ii]);
			}
			return PyUnicode_FromStringAndSize(hex_value, (size+1) * 2);
		default:
			break;
	}
	PyErr_SetString(PyExc_ValueError, "Invalid group.");
	return NULL;
}

// adds two elements together
PyObject *Element_add(PyObject* a, PyObject *b) {
	// convert both objects to Elements
	Element *e1 = (Element*)a;
	Element *e2 = (Element*)b;
	// make sure they're in the same ring
	if (e1->group != e2->group) {
		PyErr_SetString(PyExc_ValueError, "elements must be members of the same group.");
		return NULL;
	}
	// build the result element
	Element *e3 = Element_create();
	// note that the result is in the same ring *and pairing*
	element_init_same_as(e3->pbc_element, e1->pbc_element);
	e3->group = e1->group;
	Py_INCREF(e1->pairing);
	e3->pairing = e1->pairing;
	
	// add the elements and store the result in e3
	element_add(e3->pbc_element, e1->pbc_element, e2->pbc_element);
	// cast and return
	e3->ready = 1;
	return (PyObject*)e3;
}

// subtracts two elements
PyObject *Element_sub(PyObject* a, PyObject *b) {
	// convert both objects to Elements
	Element *e1 = (Element*)a;
	Element *e2 = (Element*)b;
	// make sure they're in the same ring
	if (e1->group != e2->group) {
		PyErr_SetString(PyExc_ValueError, "elements must be members of the same group.");
		return NULL;
	}
	// build the result element
	Element *e3 = Element_create();
	// note that the result is in the same ring *and pairing*
	element_init_same_as(e3->pbc_element, e1->pbc_element);
	e3->group = e1->group;
	Py_INCREF(e1->pairing);
	e3->pairing = e1->pairing;
	// add the elements and store the result in e3
	element_sub(e3->pbc_element, e1->pbc_element, e2->pbc_element);
	// cast and return
	e3->ready = 1;
	return (PyObject*)e3;
}

// multiplies two elements
// note that elements from any ring can be multiplied by those in Zr.
PyObject *Element_mult(PyObject* a, PyObject *b) {
	// convert a to an element
	Element *e1 = (Element*)a;
	
	// build the result element
	Element *e3 = (Element *)ElementType.tp_alloc(&ElementType, 0);
	
	// note that the result is in the same ring *and pairing*
	element_init_same_as(e3->pbc_element, e1->pbc_element);
	e3->group = e1->group;
	Py_INCREF(e1->pairing);
	e3->pairing = e1->pairing;

	// check to see if b is an integer
	if(PyLong_Check(b)) {
		// cast it to an MPZ
		mpz_t i;
		mpz_init(i);
		pynum_to_mpz(b, i);
		element_mul_mpz(e3->pbc_element, e1->pbc_element, i);
		mpz_clear(i);	
	} else if (PyObject_TypeCheck(b, &ElementType)) {
		Element *e2 = (Element*)b;
		// make sure they're in the same group
		if (e1->group != e2->group && e2->group != Zr) {
			PyErr_SetString(PyExc_ValueError, "elements must be in the same group or Zr.");
			return NULL;
		}
		// add the elements and store the result in e3
		if (e2->group != Zr) {
			element_mul(e3->pbc_element, e1->pbc_element, e2->pbc_element);
		} else {
			element_mul_zn(e3->pbc_element, e1->pbc_element, e2->pbc_element);
		}
	}
	// cast and return
	e3->ready = 1;
	return (PyObject*)e3;
}

// divide element a by element b
PyObject *Element_div(PyObject* a, PyObject *b) {
	// convert both objects to Elements
	Element *e1 = (Element*)a;
	Element *e2 = (Element*)b;
	// make sure they're in the same ring
	if (e1->group != e2->group) {
		PyErr_SetString(PyExc_ValueError, "elements must be in the same group.");
		return NULL;
	}
	// build the result element
	Element *e3 = (Element *)ElementType.tp_alloc(&ElementType, 0);
	// note that the result is in the same ring *and pairing*
	element_init_same_as(e3->pbc_element, e1->pbc_element);
	e3->group = e1->group;
	Py_INCREF(e1->pairing);
	e3->pairing = e1->pairing;
	// add the elements and store the result in e3
	element_div(e3->pbc_element, e1->pbc_element, e2->pbc_element);
	// cast and return
	e3->ready = 1;
	return (PyObject*)e3;
}

// raises element a to the power of b
// b can be either an element or an integer
PyObject *Element_pow(PyObject* a, PyObject *b, PyObject *c) {

	// check the types
	if (!PyObject_TypeCheck(a, &ElementType)) {
		PyErr_SetString(PyExc_TypeError, "Argument 1 must be an element.");
		return NULL;
	}
	
	// convert a to a pbc type
	Element *e1 = (Element*)a;
	
	// build the result element
	Element *e3 = (Element *)ElementType.tp_alloc(&ElementType, 0);
	e3->group = e1->group;
	Py_INCREF(e1->pairing);
	e3->pairing = e1->pairing;
	element_init_same_as(e3->pbc_element, e1->pbc_element);

	// convert b to pbc type
	if (PyLong_Check(b)) {	
		// convert it to an mpz
		mpz_t new_n;
		pynum_to_mpz(b, new_n);
		// perform the pow op
		element_pow_mpz(e3->pbc_element, e1->pbc_element, new_n);
	} else if (PyObject_TypeCheck(b, &ElementType)) {
		// convert it to an element
		Element *e2 = (Element*)b;
		// make sure its in the right ring
		if (e2->group != Zr) {
			PyErr_SetString(PyExc_ValueError, "element must be in Zr.");
			return NULL;
		}
		element_pow_zn(e3->pbc_element, e1->pbc_element, e2->pbc_element);
	} else {
		PyErr_SetString(PyExc_TypeError, "Argument 2 must be an integer or element.");
		return NULL;
	}
	
	// cast and return
	e3->ready = 1;
	return (PyObject*)e3;
}

// returns -a
PyObject *Element_neg(PyObject *a) {
	// check the type of a
	if (!PyObject_TypeCheck(a, &ElementType)) {
		PyErr_SetString(PyExc_TypeError, "argument must be an element.");
		return NULL;
	}
	
	// cast it
	Element *e1 = (Element*)a;
	
	// make sure we aren't in a bad group
	if (e1->group == GT) {
		PyErr_SetString(PyExc_ValueError, "Can't invert an element in GT.");
		return NULL;
	}		
	
	// build the result element
	Element *e2 = Element_create();
	element_init_same_as(e2->pbc_element, e1->pbc_element);
	e2->group = e1->group;
	Py_INCREF(e1->pairing);
	e2->pairing = e1->pairing;
	
	// perform the neg op
	element_neg(e2->pbc_element, e1->pbc_element);
	
	// you're ready	
	e2->ready = 1;
	// cast and return
	return (PyObject*)e2;
}

// returns a**-1
PyObject *Element_invert(PyObject *a) {
	// check the type of a
	if (!PyObject_TypeCheck(a, &ElementType)) {
		PyErr_SetString(PyExc_TypeError, "argument must be an element.");
		return NULL;
	}
	
	// cast it
	Element *e1 = (Element*)a;
	
	// make sure we aren't in a bad group
	if (e1->group == GT) {
		PyErr_SetString(PyExc_ValueError, "Can't invert an element in GT.");
		return NULL;
	}	
	
	// build the result element
	Element *e2 = Element_create();
	element_init_same_as(e2->pbc_element, e1->pbc_element);
	e2->group = e1->group;
	Py_INCREF(e1->pairing);
	e2->pairing = e1->pairing;
	
	// perform the neg op
	element_invert(e2->pbc_element, e1->pbc_element);
	
	// you're ready	
	e2->ready = 1;
	// cast and return
	return (PyObject*)e2;
}

PyObject *Element_cmp(PyObject *a, PyObject *b, int op) {

	// typecheck a
	if (!PyObject_TypeCheck(a, &ElementType)) {
		PyErr_SetString(PyExc_TypeError, "Cannot compare elements with non-elements.");
		return NULL;
	}
	
	// it's safe, cast it to an element
	Element *e1 = (Element*)a;
	
	// opcheck- only == and != are defined
	if(op != Py_EQ && op != Py_NE) {
		PyErr_SetString(PyExc_TypeError, "Invalid comparison between objects.");
		return NULL;
	}
	
	// type-and-value check b
	if (!PyObject_TypeCheck(b, &ElementType)) {
		if (PyLong_Check(b)) {
			size_t i = PyNumber_AsSsize_t(b, NULL);
			if (i == 1) {
				if(element_is1(e1->pbc_element)) {
					if (op == Py_EQ) Py_RETURN_TRUE; else Py_RETURN_FALSE;
				} else {
					if (op == Py_EQ) Py_RETURN_FALSE; else Py_RETURN_TRUE;
				}
			} else if (i == 0) {
				if(element_is0(e1->pbc_element)) {
					if (op == Py_EQ) Py_RETURN_TRUE; else Py_RETURN_FALSE;
				} else {
					if (op == Py_EQ) Py_RETURN_FALSE; else Py_RETURN_TRUE;
				}
			}
		}
		PyErr_SetString(PyExc_TypeError, "Cannot compare elements with non-elements other than 1 and 0.");
		return NULL;
	}
	
	// cast b to element
	Element *e2 = (Element*)b;
	// perform the comparison
	if(!element_cmp(e1->pbc_element, e2->pbc_element)) {
		if (op == Py_EQ) Py_RETURN_TRUE; else Py_RETURN_FALSE;
	} else {
		if (op == Py_EQ) Py_RETURN_FALSE; else Py_RETURN_TRUE;
	}
}

PyMemberDef Element_members[] = {
	{NULL}
};

PyMethodDef Element_methods[] = {
	{"from_hash", (PyCFunction)Element_from_hash, METH_VARARGS | METH_CLASS, "Creates an Element from the given hash value."},
	{"random", (PyCFunction)Element_random, METH_VARARGS | METH_CLASS, "Creates a random element from the given group."},
	{"zero", (PyCFunction)Element_zero, METH_VARARGS | METH_CLASS, "Creates an element representing the additive identity for its group."},
	{"one", (PyCFunction)Element_one, METH_VARARGS | METH_CLASS, "Creates an element representing the multiplicative identity for its group."},
	{NULL, NULL}
};

PyNumberMethods Element_num_meths = {
	Element_add,		//binaryfunc nb_add;
	Element_sub,		//binaryfunc nb_subtract;
	Element_mult,		//binaryfunc nb_multiply;
	0,				//binaryfunc nb_remainder;
	0,				//binaryfunc nb_divmod;
	Element_pow,		//ternaryfunc nb_power;
	(unaryfunc)Element_neg,		//unaryfunc nb_negative;
	0,				//unaryfunc nb_positive;
	0,				//unaryfunc nb_absolute;
	0,	//inquiry nb_bool;
	(unaryfunc)Element_invert,	//unaryfunc nb_invert;
	0,				//binaryfunc nb_lshift;
	0,				//binaryfunc nb_rshift;
	0,				//binaryfunc nb_and;
	0,				//binaryfunc nb_xor;
	0,				//binaryfunc nb_or;
	0,		//unaryfunc nb_int;
	0,				//void *nb_reserved;
	0,				//unaryfunc nb_float;

	0,				//binaryfunc nb_inplace_add;
	0,				//binaryfunc nb_inplace_subtract;
	0,				//binaryfunc nb_inplace_multiply;
	0,				//binaryfunc nb_inplace_remainder;
	0,				//ternaryfunc nb_inplace_power;
	0,				//binaryfunc nb_inplace_lshift;
	0,				//binaryfunc nb_inplace_rshift;
	0,				//binaryfunc nb_inplace_and;
	0,				//binaryfunc nb_inplace_xor;
	0,				//binaryfunc nb_inplace_or;
	0,
	0,				//binaryfunc nb_floor_divide;
	Element_div,		//binaryfunc nb_true_divide;
	0,				//binaryfunc nb_inplace_floor_divide;
	0,				//binaryfunc nb_inplace_true_divide;
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
	0,			   /*tp_reserved*/
	Element_str,                         /*tp_repr*/
	&Element_num_meths,                         /*tp_as_number*/
	0,                         /*tp_as_sequence*/
	0,                         /*tp_as_mapping*/
	0,                         /*tp_hash */
	0,                         /*tp_call*/
	Element_str,                         /*tp_str*/
	0,                         /*tp_getattro*/
	0,                         /*tp_setattro*/
	0,                         /*tp_as_buffer*/
	Py_TPFLAGS_DEFAULT | Py_TPFLAGS_BASETYPE, /*tp_flags*/
	Element__doc__,           /* tp_doc */
	0,		               /* tp_traverse */
	0,		               /* tp_clear */
	Element_cmp,		               /* tp_richcompare */
	0,		               /* tp_weaklistoffset */
	0,		               /* tp_iter */
	0,		               /* tp_iternext */
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
*						Module							      *
*******************************************************************************/

PyMethodDef pypbc_methods[] = {
	{"get_random_prime", get_random_prime, METH_VARARGS, "get a random n-bit prime"},
	{"get_random", get_random, METH_VARARGS, "get a random value less than n"},
	{"set_point_format_compressed", set_point_format_compressed, METH_NOARGS, "Set option to use compressed (sign + X) point format"},
	{"set_point_format_uncompressed", set_point_format_uncompressed, METH_NOARGS, "Set option to use uncompressed (X,Y) point format"},
	{NULL, NULL, 0, NULL}
};

PyModuleDef pypbc_module = {
	PyModuleDef_HEAD_INIT,
	"pypbc",
	"pypbc",
	-1,
	pypbc_methods
};

PyMODINIT_FUNC
PyInit_pypbc(void) 
{
	PyObject* m;

	if (PyType_Ready(&ParametersType) < 0)
		return NULL;

	if (PyType_Ready(&PairingType) < 0)
		return NULL;

	if (PyType_Ready(&ElementType) < 0)
		return NULL;

	m = PyModule_Create(&pypbc_module);

	if (m == NULL)
		return NULL;

	Py_INCREF(&PairingType);
	Py_INCREF(&ParametersType);
	Py_INCREF(&ElementType);
	// add the objects
	PyModule_AddObject(m, "Parameters", (PyObject *)&ParametersType);
	PyModule_AddObject(m, "Pairing", (PyObject *)&PairingType);
	PyModule_AddObject(m, "Element", (PyObject *)&ElementType);
	// add the constants
	PyModule_AddObject(m, "G1", PyLong_FromLong(G1));
	PyModule_AddObject(m, "G2", PyLong_FromLong(G2));
	PyModule_AddObject(m, "GT", PyLong_FromLong(GT));
	PyModule_AddObject(m, "Zr", PyLong_FromLong(Zr));
	return m;
}
