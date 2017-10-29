#! /usr/bin/env python3

from distutils.core import setup, Extension

pbc = Extension(	"pypbc",
				libraries=["pbc"],
				sources=["pypbc.c"]
			)

setup(	name="pypbc",
		version="0.1",
		description="a simple set of bindings to PBC's interface.",
		author="Geremy Condra",
		author_email="debatem1@gmail.com",
		url="geremycondra.net",
		py_modules=["test", "KSW"],
		ext_modules=[pbc]
)
