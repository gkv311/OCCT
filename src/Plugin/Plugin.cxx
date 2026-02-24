// Created on: 1997-03-06
// Created by: Mister rmi
// Copyright (c) 1997-1999 Matra Datavision
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

#include <Plugin.hxx>

#include <OSD_SharedLibrary.hxx>
#include <Plugin_Failure.hxx>
#include <Plugin_MapOfFunctions.hxx>
#include <Resource_Manager.hxx>
#include <Standard_GUID.hxx>
#include <Standard_Transient.hxx>
#include <TCollection_AsciiString.hxx>

#include <Standard_WarningDisableFunctionCast.hxx>

//=======================================================================
//function : Load
//purpose  : 
//=======================================================================
Handle(Standard_Transient) Plugin::Load (const Standard_GUID& theGUID,
                                         const Standard_Boolean theVerbose)
{
  static Plugin_MapOfFunctions theMapOfFunctions;

  char aPluginGuidRaw[Standard_GUID_SIZE_ALLOC];
  theGUID.ToCString(aPluginGuidRaw, Standard_GUID_SIZE_ALLOC);

  const TCollection_AsciiString aPluginGuid(aPluginGuidRaw);

  typedef Standard_Transient* (*FactoryType)(const Standard_GUID&);

  OSD_Function aFunction;
  if (theMapOfFunctions.Find(aPluginGuid, aFunction))
  {
    Handle(Standard_Transient) aServiceFactory = (*(FactoryType)aFunction) (theGUID);
    return aServiceFactory;
  }

  Handle(Resource_Manager) aPluginResource = new Resource_Manager("Plugin");
  const TCollection_AsciiString aResource = aPluginGuid + ".Location";

  TCollection_AsciiString aResVal;
  if (!aPluginResource->Find(aResource, aResVal))
  {
    if (theVerbose)
      std::cout << "Could not find the resource: '" << aResource << "'" << std::endl;

    const TCollection_AsciiString aMsg =
      TCollection_AsciiString("Could not find the resource: '") + aResource + "'";
    throw Plugin_Failure(aMsg.ToCString());
  }
    
  TCollection_AsciiString aPluginLibrary;
#ifndef _WIN32
  aPluginLibrary += "lib";
#endif

  aPluginLibrary += aResVal;

#ifdef _WIN32
  aPluginLibrary += ".dll";
#elif defined(__APPLE__)
  aPluginLibrary += ".dylib";
#elif defined (HPUX) || defined(_hpux)
  aPluginLibrary += ".sl";
#else
  aPluginLibrary += ".so";
#endif

  OSD_SharedLibrary aSharedLibrary(aPluginLibrary.ToCString());
  if (!aSharedLibrary.DlOpen(OSD_RTLD_LAZY))
  {
    const TCollection_AsciiString anError(aSharedLibrary.DlError());
    if (theVerbose)
      std::cout << "Could not open: '"  << aResVal << "' ; reason: "<< anError << std::endl;

    const TCollection_AsciiString aMsg =
      TCollection_AsciiString("Could not open: '") + aResVal + "'; reason: " + anError;
    throw Plugin_Failure(aMsg.ToCString());
  }

  aFunction = aSharedLibrary.DlSymb("PLUGINFACTORY");
  if (aFunction == nullptr)
  {
    const TCollection_AsciiString anError(aSharedLibrary.DlError());
    const TCollection_AsciiString aMsg =
      TCollection_AsciiString("Could not find the factory in: '") + aResVal + "'; reason: " + anError;
    throw Plugin_Failure(aMsg.ToCString());
  }

  theMapOfFunctions.Bind(aPluginGuid, aFunction);

  Handle(Standard_Transient) aServiceFactory = (*(FactoryType)aFunction) (theGUID);
  return aServiceFactory;
}
