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

#include <Interface_Static.hxx>

#include <NCollection_StdAllocator.hxx>
#include <TCollection_HAsciiString.hxx>
#include <Standard_Mutex.hxx>

#include <algorithm>
#include <vector>

// global registry of values and mutex to protect it from concurrent access
static NCollection_DataMap<TCollection_AsciiString, Handle(Interface_TypedValue)> theValuesMap;
static Standard_Mutex theMutex;

//  #######################################################################
//  #########    DICTIONNAIRE DES STATICS (static sur Static)    ##########

Standard_Boolean  Interface_Static::Init
  (const Standard_CString family,  const Standard_CString name,
   const Interface_ParamType type, const Standard_CString init)
{
  if (name[0] == '\0') return Standard_False;

  Standard_Mutex::Sentry aSentry(theMutex);
  if (theValuesMap.IsBound(name))
    return Standard_False;

  Handle(Interface_TypedValue) item;
  if (type == Interface_ParamMisc)
  {
    Handle(Interface_TypedValue) other;
    if (!theValuesMap.Find(init, other))
      return Standard_False;

    item = new Interface_TypedValue(family, name, other);
  }
  else
  {
    item = new Interface_TypedValue(family, name, type, init);
  }

  theValuesMap.Bind(name, item);
  return Standard_True;
}


Standard_Boolean  Interface_Static::Init
  (const Standard_CString family,  const Standard_CString name,
   const Standard_Character type, const Standard_CString init)
{
  Interface_ParamType epyt;
  switch (type) {
    case 'e' : epyt = Interface_ParamEnum;     break;
    case 'i' : epyt = Interface_ParamInteger;  break;
    case 'o' : epyt = Interface_ParamIdent;    break;
    case 'p' : epyt = Interface_ParamText;     break;
    case 'r' : epyt = Interface_ParamReal;     break;
    case 't' : epyt = Interface_ParamText;     break;
    case '=' : epyt = Interface_ParamMisc;     break;
    case '&' : {
      Handle(Interface_TypedValue) unstat = Interface_Static::Static(name);
      return !unstat.IsNull() && Interface_Static::InitValues(unstat, init);
    }
    default  : return Standard_False;
  }
  if (!Interface_Static::Init (family,name,epyt,init)) return Standard_False;
  if (type != 'p') return Standard_True;

  Handle(Interface_TypedValue) stat = Interface_Static::Static(name);
//NT  stat->SetSatisfies (StaticPath,"Path");
  if (!stat->Satisfies(stat->HStringValue())) stat->SetCStringValue("");
  return Standard_True;
}

Standard_Boolean Interface_Static::InitValues (const Handle(Interface_TypedValue)& theStatic,
                                               const Standard_CString init)
{
  //  Editions : init donne un petit texte d edition, en 2 termes "cmd var" :
  //  imin <ival> imax <ival>  rmin <rval>  rmax <rval> unit <def>
  //  enum <from> ematch <from>  eval <cval>
  Standard_Integer i, iblc = 0;
  for (i = 0; init[i] != '\0'; i++) if (init[i] == ' ') iblc = i + 1;
  //  Reconnaissance du sous-cas et aiguillage
  if (init[0] == 'i' && init[2] == 'i')
    theStatic->SetIntegerLimit(Standard_False, atoi(&init[iblc]));
  else if (init[0] == 'i' && init[2] == 'a')
    theStatic->SetIntegerLimit(Standard_True, atoi(&init[iblc]));
  else if (init[0] == 'r' && init[2] == 'i')
    theStatic->SetRealLimit(Standard_False, Atof(&init[iblc]));
  else if (init[0] == 'r' && init[2] == 'a')
    theStatic->SetRealLimit(Standard_True, Atof(&init[iblc]));
  else if (init[0] == 'u')
    theStatic->SetUnitDef(&init[iblc]);
  else if (init[0] == 'e' && init[1] == 'm')
    theStatic->StartEnum(atoi(&init[iblc]), Standard_True);
  else if (init[0] == 'e' && init[1] == 'n')
    theStatic->StartEnum(atoi(&init[iblc]), Standard_False);
  else if (init[0] == 'e' && init[1] == 'v')
    theStatic->AddEnum(&init[iblc]);
  else
    return Standard_False;

  return Standard_True;
}

void Interface_Static::Init (const Handle(Interface_TypedValue)& theStatic)
{
  if (theStatic.IsNull())
    return;

  const Standard_CString aName = theStatic->Name();
  Standard_Mutex::Sentry aSentry(theMutex);
  if (theValuesMap.IsBound(aName))
    return;

  Handle(Interface_TypedValue) aNewItem = new Interface_TypedValue(theStatic->Family(), aName, theStatic);
  theValuesMap.Bind(aName, aNewItem);
}

Handle(Interface_TypedValue) Interface_Static::Static (const Standard_CString name)
{
  Standard_Mutex::Sentry aSentry(theMutex);
  Handle(Interface_TypedValue) result;
  theValuesMap.Find(name, result);
  return result;
}

Standard_Boolean Interface_Static::IsPresent (const Standard_CString name)
{
  Standard_Mutex::Sentry aSentry(theMutex);
  return theValuesMap.IsBound (name);
}

//  ##########  VALEUR COURANTE  ###########

Standard_Boolean Interface_Static::IsSet (const Standard_CString name)
{
  Standard_Mutex::Sentry aSentry(theMutex);
  const Handle(Interface_TypedValue)* item = theValuesMap.Seek(name);
  return item != nullptr && !item->IsNull() && (*item)->IsSetValue();
}


Standard_CString  Interface_Static::CVal  (const Standard_CString name)
{
  Standard_Mutex::Sentry aSentry(theMutex);
  const Handle(Interface_TypedValue)* item = theValuesMap.Seek(name);
  if (item == nullptr || item->IsNull())
  {
#ifdef OCCT_DEBUG
    std::cout << "Warning: Interface_Static::CVal: incorrect parameter " << name << std::endl;
#endif
    return "";
  }
  return (*item)->CStringValue();
}


Standard_Integer  Interface_Static::IVal  (const Standard_CString name)
{
  Standard_Mutex::Sentry aSentry(theMutex);
  const Handle(Interface_TypedValue)* item = theValuesMap.Seek(name);
  if (item == nullptr || item->IsNull())
  {
#ifdef OCCT_DEBUG
    std::cout << "Warning: Interface_Static::IVal: incorrect parameter " << name << std::endl;
#endif
    return 0;
  }
  return (*item)->IntegerValue();
}


Standard_Real Interface_Static::RVal (const Standard_CString name)
{
  Standard_Mutex::Sentry aSentry(theMutex);
  const Handle(Interface_TypedValue)* item = theValuesMap.Seek(name);
  if (item == nullptr || item->IsNull())
  {
#ifdef OCCT_DEBUG
    std::cout << "Warning: Interface_Static::RVal: incorrect parameter " << name << std::endl;
#endif
    return 0.0;
  }
  return (*item)->RealValue();
}


Standard_Boolean  Interface_Static::SetCVal
  (const Standard_CString name, const Standard_CString val)
{
  Standard_Mutex::Sentry aSentry(theMutex);
  const Handle(Interface_TypedValue)* anItem = theValuesMap.Seek(name);
  if (anItem == nullptr || anItem->IsNull())
    return Standard_False;

  return (*anItem)->SetCStringValue(val);
}


Standard_Boolean  Interface_Static::SetIVal
  (const Standard_CString name, const Standard_Integer val)
{
  Standard_Mutex::Sentry aSentry(theMutex);
  const Handle(Interface_TypedValue)* anItem = theValuesMap.Seek(name);
  if (anItem == nullptr || anItem->IsNull())
    return Standard_False;

  return (*anItem)->SetIntegerValue(val);
}


Standard_Boolean  Interface_Static::SetRVal
  (const Standard_CString name, const Standard_Real val)
{
  Standard_Mutex::Sentry aSentry(theMutex);
  const Handle(Interface_TypedValue)* anItem = theValuesMap.Seek(name);
  if (anItem == nullptr || anItem->IsNull())
    return Standard_False;

  return (*anItem)->SetRealValue(val);
}

Handle(TColStd_HSequenceOfHAsciiString) Interface_Static::Items (const Standard_CString theFamily)
{
  Standard_Mutex::Sentry aSentry(theMutex);

  std::vector<Handle(TCollection_HAsciiString), NCollection_StdAllocator<Handle(TCollection_HAsciiString)>> aKeys;
  for (const std::pair<const TCollection_AsciiString, Handle(Interface_TypedValue)>& iter : theValuesMap)
  {
    Handle(Interface_TypedValue) item = iter.second;
    if (!item.IsNull() && (theFamily == nullptr || strcmp(theFamily, item->Family()) == 0))
      aKeys.push_back(new TCollection_HAsciiString(iter.first));
  }

  // sort alphabetically
  const auto isLess = [](const Handle(TCollection_HAsciiString)& aStr1, const Handle(TCollection_HAsciiString)& aStr2)
  {
    return aStr1->IsLess(aStr2);
  };
  std::sort(aKeys.begin(),aKeys.end(), isLess);

  Handle(TColStd_HSequenceOfHAsciiString) aList = new TColStd_HSequenceOfHAsciiString();
  for (const Handle(TCollection_HAsciiString)& aKey : aKeys)
    aList->Append(aKey);

  return aList;
}

//=======================================================================
// function : FillMap
// purpose  :
//=======================================================================
void Interface_Static::FillMap (NCollection_DataMap<TCollection_AsciiString, Handle(Interface_TypedValue)>& theMap)
{
  theMap.Clear();

  Standard_Mutex::Sentry aSentry(theMutex);
  for (const std::pair<const TCollection_AsciiString, Handle(Interface_TypedValue)>& iter : theValuesMap)
  {
    if (!iter.second.IsNull())
      theMap.Bind(iter.first, new Interface_TypedValue(*iter.second));
  }
}

//=======================================================================
// function : FillMap
// purpose  : Fills given string-to-string map with all static data
//=======================================================================
void Interface_Static::FillMap (NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString, TCollection_AsciiString>& theMap)
{
  theMap.Clear();

  Standard_Mutex::Sentry aSentry(theMutex);
  for (const std::pair<const TCollection_AsciiString, Handle(Interface_TypedValue)>& iter : theValuesMap)
  {
    if (!iter.second.IsNull() && !iter.second->HStringValue().IsNull())
      theMap.Bind(iter.first, iter.second->HStringValue()->String());
  }
}
