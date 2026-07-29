// Created on: 2000-10-05
// Created by: Andrey BETENEV
// Copyright (c) 2000-2014 OPEN CASCADE SAS
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

#ifndef _STEPCAFControl_Controller_HeaderFile
#define _STEPCAFControl_Controller_HeaderFile

#include <Standard.hxx>
#include <Standard_Type.hxx>

//! Provides initialization of Interface_Static parameters
//! used during translation of STEP to/from XCAF documents.
class STEPCAFControl_Controller
{
public:

  //! Initializes Interface_Static parameters used during translation of STEP to/from XCAF documents.
  Standard_EXPORT static Standard_Boolean Init();

};

#endif // _STEPCAFControl_Controller_HeaderFile
