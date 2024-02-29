#! /usr/bin/env python3

from setuptools import setup, Extension

pypbc_module = Extension("pypbc", sources=["pypbc.c"], libraries=["pbc"])

setup(
    name="pypbc",
    version="1.0",
    description="a simple set of bindings to PBC's interface.",
    author="Jemtaly (original by Geremy Condra)",
    author_email="Jemtaly@outlook.com",
    url="github.com/Jemtaly/pypbc",
    ext_modules=[pypbc_module],
)
