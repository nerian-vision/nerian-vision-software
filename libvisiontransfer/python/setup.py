#!/usr/bin/env python3

###############################################################################/
# Copyright (c) 2022 Nerian Vision GmbH
#
# Permission is hereby granted, free of charge, to any person obtaining a copy
# of this software and associated documentation files (the "Software"), to deal
# in the Software without restriction, including without limitation the rights
# to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
# copies of the Software, and to permit persons to whom the Software is
# furnished to do so, subject to the following conditions:
#
# The above copyright notice and this permission notice shall be included in
# all copies or substantial portions of the Software.
###############################################################################/

from setuptools import setup, Extension
from Cython.Build import cythonize

import numpy as np
import os

# default to CMake build based directory structure
srcbase = os.getenv("LIBVISIONTRANSFER_SRCDIR", "../..")
libbase = os.getenv("LIBVISIONTRANSFER_LIBDIR", "../..")
libname = os.getenv("LIBVISIONTRANSFER_LIB", "/libvisiontransfer-static.a")
extra_libs_str = os.getenv("LIBVISIONTRANSFER_EXTRA_LIBS", "")
extra_libs = [s.strip() for s in extra_libs_str.split(',') if s.strip()!='']

# could be another var, but this test suffices for now
is_msvc = len(extra_libs) > 0

print('libvisiontransfer src dir:  '+srcbase)
print('libvisiontransfer lib dir:  '+libbase)
print('libvisiontransfer lib name: '+libname)

incdir = srcbase
libdir = libbase

setup(
    name="visiontransfer",
    author="Nerian Vision GmbH",
    author_email="service@nerian.com",
    version="1.0.0",
    packages=["visiontransfer"],
    ext_modules=cythonize(
        Extension(
            name="visiontransfer",
            sources=["visiontransfer/visiontransfer.pyx"],
            include_dirs=[np.get_include(), incdir],
            libraries=[*extra_libs],
            extra_objects=[libbase + libname],
            extra_compile_args=(['/Zc:__cplusplus'] if is_msvc else []),  # force MSVC to report correct __cplusplus
            language="c++",
            define_macros=[("VISIONTRANSFER_NO_DEPRECATION_WARNINGS", "1")], # silently wrap anything we want
            #define_macros=[("NPY_NO_DEPRECATED_API", "NPY_1_7_API_VERSION")], # for numpy; Cython>=3.0 only
        )
    , compiler_directives = { 'embedsignature': True })
)
