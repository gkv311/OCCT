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

#include <RWHeaderSection.hxx>

#include <HeaderSection.hxx>
#include <HeaderSection_Protocol.hxx>
#include <Interface_GeneralLib.hxx>
#include <Interface_ReaderLib.hxx>
#include <RWHeaderSection_GeneralModule.hxx>
#include <RWHeaderSection_ReadWriteModule.hxx>
#include <StepData.hxx>
#include <StepData_WriterLib.hxx>

static bool initOnce()
{
  static Handle(RWHeaderSection_ReadWriteModule) aRWM = new RWHeaderSection_ReadWriteModule();
  static Handle(RWHeaderSection_GeneralModule) aGM = new RWHeaderSection_GeneralModule();

  Handle(HeaderSection_Protocol) aProtocol = HeaderSection::Protocol();
  StepData::AddHeaderProtocol(aProtocol);
  StepData_WriterLib::SetGlobal(aRWM, aProtocol);
  Interface_ReaderLib::SetGlobal(aRWM, aProtocol);
  Interface_GeneralLib::SetGlobal(aGM, aProtocol);
  return true;
}

void RWHeaderSection::Init()
{
  static const bool wasInitialized = initOnce();
  (void)wasInitialized;
}
