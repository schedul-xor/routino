/***************************************
 Python router interface definition.

 Part of the Routino routing software.
 ******************/ /******************
 This file Copyright 2018, 2020 Andrew M. Bishop

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU Affero General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU Affero General Public License for more details.

 You should have received a copy of the GNU Affero General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
 ***************************************/


/* Name the module 'router' in the 'routino' package */

%module(package="routino.router") router


/* Include the 'routino.h' header file from the library in the auto-generated code */

%{
#include "routino.h"
%}


/* Return NULL-terminated arrays of strings as a list of strings */

%typemap(ret) char** {
  $result = PyList_New(0);

  char **p=$1;

  while(*p)
    {
     PyList_Append($result, PyString_FromString(*p));
     p++;
    }
}


/* Handle lists of Routino Waypoints as an array */

%typemap(in) Routino_Waypoint ** {
  /* Check if is a list */
  if (PyList_Check($input))
    {
     int size = PyList_Size($input);
     int i = 0;
     $1 = (Routino_Waypoint **) malloc(size*sizeof(Routino_Waypoint *));
     for (i = 0; i < size; i++)
        if (!SWIG_IsOK(SWIG_ConvertPtr(PyList_GetItem($input, i), (void **) &$1[i], $descriptor(Routino_Waypoint*), 0)))
           SWIG_exception_fail(SWIG_TypeError, "in method '$symname', expecting type Routino_Waypoint");
  } else {
    PyErr_SetString(PyExc_TypeError, "not a list");
    SWIG_fail;
  }
}

%typemap(freearg) Routino_Waypoint ** {
  free((Routino_Waypoint *) $1);
}


/* Handle the Routino_ProgressFunc pointer function */

%typemap(in) Routino_ProgressFunc {
    if($input != Py_None)
       $1 = (Routino_ProgressFunc)PyLong_AsVoidPtr($input);
}


/* Rename variables and functions by stripping 'Routino_' or 'ROUTINO_' prefixes */

%rename("%(regex:/R[Oo][Uu][Tt][Ii][Nn][Oo]_(.*)/\\1/)s") "";

/* Rename the Routino_CalculateRoute() function so we can replace with a Python wrapper */

%rename("_CalculateRoute") "Routino_CalculateRoute";

/* Rename the Routino_LoadDatabase() function so we can replace with a Python wrapper */

%rename("_LoadDatabase") "Routino_LoadDatabase";


/* Add some custom Python code to the module */

%pythoncode %{

import ctypes

# Set up a replacement function for a macro in the original

def CheckAPIVersion():
    return _router.Check_API_Version(_router.API_VERSION)

# Set up a replacement function so that we do not need to pass the size of the list

def CalculateRoute(database, profile, translation, waypoints, options, progress=None):
    if progress is not None:
        # typedef int (*Routino_ProgressFunc)(double complete);
        callback_type = ctypes.CFUNCTYPE(ctypes.c_int, ctypes.c_double)
        progress = ctypes.cast(callback_type(progress), ctypes.c_void_p).value
    return _router._CalculateRoute(database, profile, translation, waypoints, len(waypoints), options, progress)

# Set up a replacement function to make the second argument optional

def LoadDatabase(dirname, prefix=None):
    return _router._LoadDatabase(dirname, prefix)

# Create a function for concatenating directory names, prefixes and filenames

def FileName(dirname, prefix, name):

    filename=""

    if dirname is not None:
        filename=dirname + "/"

    if prefix is not None:
        filename += prefix + "-"

    filename += name

    return filename
%}


/* Use the 'routino.h' header file from the library to generate the wrapper (everything is read-only) */

%immutable;

%include "../src/routino.h"
