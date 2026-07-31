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


#include <Interface_TypedValue.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Interface_TypedValue,MoniTool_TypedValue)

Interface_TypedValue::Interface_TypedValue(const Standard_CString    family,
                                           const Standard_CString    name,
                                           const Interface_ParamType type,
                                           const Standard_CString    init)
: MoniTool_TypedValue(name, Interface_TypedValue::ParamTypeToValueType(type), init),
  myFamily(family)
{
  //
}

Interface_TypedValue::Interface_TypedValue(const Standard_CString    name,
                                           const Interface_ParamType type,
                                           const Standard_CString    init)
: Interface_TypedValue("", name, type, init)
{
  //
}

Interface_TypedValue::Interface_TypedValue(const Standard_CString family,
                                           const Standard_CString name,
                                           const Handle(Interface_TypedValue)& other)
: MoniTool_TypedValue(name, other->ValueType(), ""),
  myFamily(family)
{
  switch (Type())
  {
    case Interface_ParamInteger:
    {
      Standard_Integer lim = 0;
      if (other->IntegerLimit(true, lim))
        SetIntegerLimit(true, lim);

      if (other->IntegerLimit(false, lim))
        SetIntegerLimit(false, lim);

      break;
    }
    case Interface_ParamReal:
    {
      Standard_Real lim = 0.0;
      if (other->RealLimit(true, lim))
        SetRealLimit(true, lim);

      if (other->RealLimit(false, lim))
        SetRealLimit(false, lim);

      SetUnitDef(other->UnitDef());
      break;
    }
    case Interface_ParamEnum:
    {
      Standard_Boolean match = false;
      Standard_Integer e0 = 0, e1 = 0;
      other->EnumDef(e0, e1, match);
      StartEnum(e0, match);
      //if (e1 >= e0) theenums = new TColStd_HArray1OfAsciiString(e0, e1);
      for (Standard_Integer i = e0; i <= e1; i++)
        AddEnum(other->EnumVal(i));

      break;
    }
    case Interface_ParamIdent:
      SetObjectType(other->ObjectType());
      break;
    default:
      break;
  }

  if (other->IsSetValue())
    SetCStringValue(other->CStringValue());
}

void Interface_TypedValue::Print(Standard_OStream& S) const
{
  S << "--- Static Value : " << Name() << " Family:" << Family();
  MoniTool_TypedValue::Print (S);
}
