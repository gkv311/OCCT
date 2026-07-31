// Created on: 1995-12-08
// Created by: Christian CAILLET
// Copyright (c) 1995-1999 Matra Datavision
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

#ifndef _Interface_Static_HeaderFile
#define _Interface_Static_HeaderFile

#include <Interface_TypedValue.hxx>
#include <TColStd_HSequenceOfHAsciiString.hxx>

//! This class maintains a registry of named static variables (parameters)
//! used as global parameters in various procedures.
//! Each parameter is identified by a name and a value as Interface_TypedValue.
class Interface_Static
{
public:

  //! Declares a new Static (by calling its constructor)
  //! If this name is already taken, does nothing and returns False
  //! Else, creates it and returns True
  //! For additional definitions, get the Static then edit it
  Standard_EXPORT static Standard_Boolean Init (const Standard_CString family, const Standard_CString name, const Interface_ParamType type, const Standard_CString init = "");
  
  //! As Init with ParamType, but type is given as a character
  //! This allows a simpler call
  //! Types : 'i' Integer, 'r' Real, 't' Text, 'e' Enum, 'o' Object
  //! '=' for same definition as, <init> gives the initial Static
  //! Returns False if <type> does not match this list
  Standard_EXPORT static Standard_Boolean Init (const Standard_CString family, const Standard_CString name, const Standard_Character type, const Standard_CString init = "");

  //! Declares a new Static and stores it in the global map as a copy of the given parameter @p theStatic.
  //! Does nothing if the corresponding Name is already stored in the Map.
  Standard_EXPORT static void Init (const Handle(Interface_TypedValue)& theStatic);

  //! Edit current @p theStatic with some parameter @p init
  Standard_EXPORT static Standard_Boolean InitValues (const Handle(Interface_TypedValue)& theStatic, const Standard_CString init);

  //! Returns a Static from its name. Null Handle if not present
  Standard_EXPORT static Handle(Interface_TypedValue) Static (const Standard_CString name);
  
  //! Returns True if a Static named <name> is present, False else
  Standard_EXPORT static Standard_Boolean IsPresent (const Standard_CString name);
  
  //! Returns a part of the definition of a Static, as a CString
  //! The part is designated by its name, as a CString
  //! If the required value is not a string, it is converted to a
  //! CString then returned
  //! If <name> is not present, or <part> not defined for <name>,
  //! this function returns an empty string
  //!
  //! Allowed parts for CDef :
  //! family : the family
  //! type  : the type ("integer","real","text","enum")
  //! label : the label
  //! satis : satisfy function name if any
  //! rmin : minimum real value
  //! rmax : maximum real value
  //! imin : minimum integer value
  //! imax : maximum integer value
  //! enum nn (nn : value of an integer) : enum value for nn
  //! unit : unit definition for a real
  Standard_EXPORT static Standard_CString CDef (const Standard_CString name, const Standard_CString part);
  
  //! Returns a part of the definition of a Static, as an Integer
  //! The part is designated by its name, as a CString
  //! If the required value is not a string, returns zero
  //! For a Boolean, 0 for false, 1 for true
  //! If <name> is not present, or <part> not defined for <name>,
  //! this function returns zero
  //!
  //! Allowed parts for IDef :
  //! imin, imax : minimum or maximum integer value
  //! estart : starting number for enum
  //! ecount : count of enum values (starting from estart)
  //! ematch : exact match status
  //! eval val : case determined from a string
  Standard_EXPORT static Standard_Integer IDef (const Standard_CString name, const Standard_CString part);
  
  //! Returns True if @p name is present AND set.
  Standard_EXPORT static Standard_Boolean IsSet (const Standard_CString name);
  
  //! Returns the value of the
  //! parameter identified by the string name.
  //! If the specified parameter does not exist, an empty
  //! string is returned.
  //! Example
  //! Interface_Static::CVal("write.step.schema");
  //! which could return:
  //! "AP214"
  Standard_EXPORT static Standard_CString CVal (const Standard_CString name);
  
  //! Returns the integer value of
  //! the translation parameter identified by the string name.
  //! Returns the value 0 if the parameter does not exist.
  //! Example
  //! Interface_Static::IVal("write.step.schema");
  //! which could return: 3
  Standard_EXPORT static Standard_Integer IVal (const Standard_CString name);
  
  //! Returns the value of a static
  //! translation parameter identified by the string name.
  //! Returns the value 0.0 if the parameter does not exist.
  Standard_EXPORT static Standard_Real RVal (const Standard_CString name);
  
  //! Modifies the value of the
  //! parameter identified by name. The modification is specified
  //! by the string val. false is returned if the parameter does not exist.
  //! Example
  //! Interface_Static::SetCVal
  //! ("write.step.schema","AP203")
  //! This syntax specifies a switch from the default STEP 214 mode to STEP 203 mode.
  Standard_EXPORT static Standard_Boolean SetCVal (const Standard_CString name, const Standard_CString val);
  
  //! Modifies the value of the
  //! parameter identified by name. The modification is specified
  //! by the integer value val. false is returned if the
  //! parameter does not exist.
  //! Example
  //! Interface_Static::SetIVal
  //! ("write.step.schema", 3)
  //! This syntax specifies a switch from the default STEP 214 mode to STEP 203 mode.S
  Standard_EXPORT static Standard_Boolean SetIVal (const Standard_CString name, const Standard_Integer val);
  
  //! Modifies the value of a
  //! translation parameter. false is returned if the
  //! parameter does not exist. The modification is specified
  //! by the real number value val.
  Standard_EXPORT static Standard_Boolean SetRVal (const Standard_CString name, const Standard_Real val);
  
  //! Sets a Static to be "uptodate"
  //! Returns False if <name> is not present
  //! This status can be used by a reinitialisation procedure to
  //! rerun if a value has been changed
  Standard_EXPORT static Standard_Boolean Update (const Standard_CString name);

  //! Returns a list of names of statics.
  //! @param[in] theFamily if not NULL, return only items with the same family
  Standard_EXPORT static Handle(TColStd_HSequenceOfHAsciiString) Items (const Standard_CString theFamily = nullptr);

  //! Initializes all standard static parameters, which can be used
  //! by every function. statics specific of a norm or a function
  //! must be defined around it
  Standard_EXPORT static void Standards();

  //! Fills given map of named values with copy of current static data.
  Standard_EXPORT static void FillMap(NCollection_DataMap<TCollection_AsciiString, Handle(Interface_TypedValue)>& theMap);

  //! Fills given string-to-string map with all static data
  Standard_EXPORT static void FillMap(NCollection_DataMap<TCollection_AsciiString, TCollection_AsciiString, TCollection_AsciiString>& theMap);

private:

  Interface_Static() = delete;

};

#endif // _Interface_Static_HeaderFile
