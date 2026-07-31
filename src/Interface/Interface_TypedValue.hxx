// Created on: 1998-02-23
// Created by: Christian CAILLET
// Copyright (c) 1998-1999 Matra Datavision
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

#ifndef _Interface_TypedValue_HeaderFile
#define _Interface_TypedValue_HeaderFile

#include <Standard.hxx>

#include <MoniTool_TypedValue.hxx>
#include <Interface_ParamType.hxx>
#include <MoniTool_ValueType.hxx>
class TCollection_HAsciiString;


class Interface_TypedValue;
DEFINE_STANDARD_HANDLE(Interface_TypedValue, MoniTool_TypedValue)

//! Now strictly equivalent to TypedValue from MoniTool,
//! except for ParamType which is redefined for legacy reasons,
//! and additional field Family.
//!
//! This class allows to dynamically manage .. typed values, i.e.
//! values which have an alphanumeric expression, but with
//! controls. Such as "must be an Integer" or "Enumerative Text"
//! etc
//!
//! Hence, a TypedValue brings a specification (type + constraints
//! if any) and a value. Its basic form is a string, it can be
//! specified as integer or real or enumerative string, then
//! queried as such.
//! Its string content, which is a Handle(HAsciiString) can be
//! shared by other data structures, hence gives a direct on line
//! access to its value.
class Interface_TypedValue : public MoniTool_TypedValue
{

public:

  //! Creates a new parameter with the given name.
  //! More precise specifications, titles, can be given to the object once created.
  //! @param[in] name parameter name
  //! @param[in] type gives the type of the parameter, default is free text;
  //!            also available : Integer, Real, Enum, Entity (i.e. Object)
  //! @param[in] init gives an initial value; if it is not given, the
  //!            value begins as "not set", its value is empty
  Standard_EXPORT Interface_TypedValue(const Standard_CString name,
                                       const Interface_ParamType type = Interface_ParamText,
                                       const Standard_CString init = "");

  //! Creates a new parameter with an additional parameter @p family.
  //! @param[in] family additional name for logical grouping or related parameters
  //! @param[in] name parameter name
  //! @param[in] type gives the type of the parameter, default is free text;
  //!            also available : Integer, Real, Enum, Entity (i.e. Object)
  //! @param[in] init gives an initial value; if it is not given, the
  //!            value begins as "not set", its value is empty
  Standard_EXPORT Interface_TypedValue(const Standard_CString family,
                                       const Standard_CString name,
                                       const Interface_ParamType type = Interface_ParamText,
                                       const Standard_CString init = "");

  //! Creates a new parameter with same definition as another one @p other
  //! (value is copied, except for Entity - it remains NULL).
  Standard_EXPORT Interface_TypedValue(const Standard_CString family,
                                       const Standard_CString name,
                                       const Handle(Interface_TypedValue)& other);

  //! Returns the family. It can be: a resource name for applis,
  //! an internal name between : $e (environment variables),
  //! $l (other, purely local)
  Standard_CString Family() const { return myFamily.ToCString(); }

  //! Prints definition, specification, and actual status and value.
  Standard_EXPORT void Print(Standard_OStream& S) const Standard_OVERRIDE;

  //! Returns the type; i.e. calls ValueType then makes correspondence between Interface_ParamType
  //! (which remains for compatibility reasons) and MoniTool_ValueType.
  Interface_ParamType Type() const
  {
    return ValueTypeToParamType(ValueType());
  }

public:

  //! Correspondence Interface_ParamType to MoniTool_ValueType.
  static MoniTool_ValueType ParamTypeToValueType (const Interface_ParamType typ)
  {
    return (MoniTool_ValueType)typ;
  }

  //! Correspondence Interface_ParamType to MoniTool_ValueType.
  static Interface_ParamType ValueTypeToParamType (const MoniTool_ValueType typ)
  {
    return (Interface_ParamType)typ;
  }

  DEFINE_STANDARD_RTTIEXT(Interface_TypedValue,MoniTool_TypedValue)

private:

  TCollection_AsciiString myFamily;

};

#endif // _Interface_TypedValue_HeaderFile
