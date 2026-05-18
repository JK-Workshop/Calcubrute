// Copyright (c) JK Workshop - All rights reserved

#if !defined(JK_CALCUBRUTE_CPYTHON_H)
#define JK_CALCUBRUTE_CPYTHON_H

#define PY_SSIZE_T_CLEAN
#include <Python.h>

#include <JK/Calcubrute.h>

struct PyContext
{
    PyObject_HEAD;
    CCBContext context;
};

inline void
PyContextInit(struct PyContext* p_self,
              )

#endif // JK_CALCUBRUTE_CPYTHON_H
