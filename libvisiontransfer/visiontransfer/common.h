/*******************************************************************************
 * Copyright (c) 2024 Allied Vision Technologies GmbH
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *******************************************************************************/

#ifndef VISIONTRANSFER_COMMON_H
#define VISIONTRANSFER_COMMON_H

#define VISIONTRANSFER_MAJOR_VERSION	0
#define VISIONTRANSFER_MINOR_VERSION	0
#define VISIONTRANSFER_PATCH_VERSION	0

#ifdef _MSC_VER
    // Visual studio needs an explicit export statement
#   define VT_EXPORT __declspec(dllexport)
#else
#   define VT_EXPORT
#endif

// Macro for marking functions / variables as deprecated
#ifdef VISIONTRANSFER_NO_DEPRECATION_WARNINGS
    // For referencing all of our own code (e.g. for Python wrapper)
#   define DEPRECATED(msg)
#else
#   if __cplusplus >= 201402L
#       define DEPRECATED(msg) [[deprecated(msg)]]
#   elif defined(__GNUC__) || defined(__clang__)
#       define DEPRECATED(msg) __attribute__ ((deprecated(msg)))
#   elif defined(_MSC_VER)
#       define DEPRECATED(msg) __declspec(deprecated(msg))
#   else
#       define DEPRECATED(msg)
#   endif
#endif

#ifndef _MSC_VER
#ifndef VISIONTRANSFER_NO_OLD_ABI_WARNING
#  // Warn for old glibc++ ABI, unless it's our own internal compatibility build
#  if defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 0
#    warning "CAUTION: You are using _GLIBCXX_USE_CXX11_ABI=0. Make sure to link a libvisiontransfer built with the same setting."
#  endif
#endif
#endif

// C++ version; special case for VS instead of demanding /Z:__cplusplus
#if defined(_MSVC_LANG)
#define VISIONTRANSFER_CPLUSPLUS_VERSION _MSVC_LANG
#else
#define VISIONTRANSFER_CPLUSPLUS_VERSION __cplusplus
#endif

#endif
