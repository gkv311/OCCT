// Created on: 2001-03-20
// Created by: Andrey BETENEV
// Copyright (c) 2001-2014 OPEN CASCADE SAS
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of Open CASCADE
// commercial license or contractual agreement.

#ifndef Standard_Assert_HeaderFile
#define Standard_Assert_HeaderFile

#include <Standard_ProgramError.hxx>

//!@file
//! This header file defines a set of ASSERT macros intended for use
//! in algorithms for debugging purposes and as a tool to organise
//! checks for abnormal situations in the uniform way.
//!
//! In contrast to C assert() function that terminates the process, these
//! macros provide choice of the action to be performed if assert failed,
//! thus allowing execution to continue when possible.
//! Except for the message for developer that appears only in Debug mode,
//! the macros behave in the same way in both Release and Debug modes.
//!
//! The preprocessor macro Standard_ASSERT_ENABLE can be defined
//! to enable the same behavior as in debug mode also in Release build.
//!
//! The ASSERT macros differ in the way they react on a wrong situation:
//! - Standard_ASSERT_RAISE:  raises exception Standard_ProgramError
//! - Standard_ASSERT_RETURN: returns specified value (last argument may
//!                           be left empty to return void)
//! - Standard_ASSERT_SKIP:   does nothing
//! - Standard_ASSERT_VOID:   does nothing; even does not evaluate first arg
//!                           when in Release mode
//! - Standard_ASSERT_INVOKE: causes unconditional assert
//! - Standard_ASSERT:        base macro (used by other macros);
//!                           does operation indicated in argument "todo"
//!
//! The assertion is assumed to fail if the first argument is
//! evaluated to zero (false).
//! The first argument is evaluated by all macros except Standard_ASSERT_VOID
//! which does not evaluate first argument when in Release mode.
//! The mode is triggered by preprocessor macro _DEBUG: if it is defined,
//! Debug mode is assumed, Release otherwise.
//!
//! The second argument (error message) should be C string (const char*).
//!
//! The environment variable CSF_ASSERT_MODE can be set to tune behavior of assert
//! handler when assertion fails.
//! The supported values are:
//! - "abort"  print message to cerr and call exit(1)
//! - "break"  print message to cerr and raise debugger break
//! - "debug"  (default) print message to cerr and raise debugger break,
//!            but only if the process is run under debugger, otherwise continue
//! - "report" only print report message to cerr and continue
//! - "quiet"  do nothing, do not even print the report message
//!
//! The Standard_STATIC_ASSERT macro is to be used for compile time checks.
//! To use this macro, write:
//!
//!   Standard_STATIC_ASSERT(const_expression);
//!
//! If const_expression is false, a compiler error occurs.
//!
//! The macros are formed as functions and require semicolon at the end.

// Stub function used to make macros complete C++ operator 
inline void Standard_ASSERT_DO_NOTHING() {}

//! Handle assert failure.
//! Depending on the current system and environment variable CSF_ASSERT_MODE (see above),
//! print error message to std::cerr to inform about failed assert,
//! propose the user and/or just do raise debug break, or continue.
Standard_EXPORT void Standard_ASSERT_HANDLER(const char* theFile, int theLine, const char* theExpr, const char* theDesc);

#if !defined(Standard_ASSERT_ENABLE) && (defined(_DEBUG) || !defined(NDEBUG))
#define Standard_ASSERT_ENABLE
#endif

// User messages are activated in debug mode only
#ifdef Standard_ASSERT_ENABLE

  // report issue and add debug breakpoint or abort execution
  #define Standard_ASSERT_INVOKE_(theExpr, theDesc) \
    Standard_ASSERT_HANDLER(__FILE__, __LINE__, #theExpr, theDesc)

  // Basic ASSERT macros
  #define Standard_ASSERT(theExpr, theDesc, theAction)                        \
    if (!(theExpr)) { Standard_ASSERT_INVOKE_(theExpr, theDesc); theAction; } \
    else Standard_ASSERT_DO_NOTHING()
  #define Standard_ASSERT_SKIP(theExpr, theDesc) \
    Standard_ASSERT(theExpr, theDesc,)
  #define Standard_ASSERT_VOID(theExpr, theDesc) \
    Standard_ASSERT(theExpr, theDesc,)

#else

  // dummy block
  #define Standard_ASSERT_INVOKE_(theExpr, theDesc) Standard_ASSERT_DO_NOTHING()

  // Basic ASSERT macros
  #define Standard_ASSERT(theExpr, theDesc, theAction) \
    if (!(theExpr)) { theAction; }                     \
    else Standard_ASSERT_DO_NOTHING()
  #define Standard_ASSERT_SKIP(theExpr, theDesc) theExpr
  #define Standard_ASSERT_VOID(theExpr, theDesc) Standard_ASSERT_DO_NOTHING()

#endif

//! Raise exception (Standard_ProgramError) with the provided message
#define Standard_ASSERT_RAISE(theExpr, theDesc)                                  \
  Standard_ASSERT(theExpr, theDesc, throw Standard_ProgramError(                \
      "*** ERROR: ASSERT in file '" __FILE__ "': \n" theDesc " (" #theExpr ")" ) )

//! Return from the current function with specified value (empty
//! if the function returns void)
#define Standard_ASSERT_RETURN(theExpr, theDesc, theReturnValue) \
  Standard_ASSERT(theExpr, theDesc, return theReturnValue)

//! Raise debug message
#define Standard_ASSERT_INVOKE(theDesc) Standard_ASSERT_INVOKE_(false, theDesc)

//! Static assert --
//! empty default template
template <bool condition> 
struct Standard_Static_Assert { };

//! Static assert -- specialization for condition being true
template <>
struct Standard_Static_Assert<true>
{
  static void assert_ok() {}
};

//! Cause compiler error if argument is not constant expression or
//! evaluates to false
#define Standard_STATIC_ASSERT(theExpr)     \
        Standard_Static_Assert<theExpr>::assert_ok();

#endif // Standard_Assert_HeaderFile

#ifdef _MSC_VER
  #pragma once
#endif
