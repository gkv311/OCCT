// Copyright (c) 1999-2014 OPEN CASCADE SAS
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

#include <StepData.hxx>

#include <Interface_Macros.hxx>
#include <Standard_Mutex.hxx>
#include <StepData_Protocol.hxx>

static Handle(StepData_Protocol) theHeader;
static Standard_Mutex theMutex;

Handle(StepData_Protocol) StepData::Protocol()
{
  static Handle(StepData_Protocol) aProto = new StepData_Protocol();
  return aProto;
}

void StepData::AddHeaderProtocol (const Handle(StepData_Protocol)& header)
{
  Standard_Mutex::Sentry aSentry(theMutex);
  theHeader = header;
}

Handle(StepData_Protocol) StepData::HeaderProtocol()
{
  Standard_Mutex::Sentry aSentry(theMutex);
  return theHeader;
}
