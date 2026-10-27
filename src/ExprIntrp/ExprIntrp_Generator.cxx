// Created on: 1992-07-20
// Created by: Arnaud BOUZY
// Copyright (c) 1992-1999 Matra Datavision
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

#include <ExprIntrp_Generator.hxx>

#include <Standard_ArrayStreamBuffer.hxx>
#include <Standard_ErrorHandler.hxx>
#include <TCollection_AsciiString.hxx>

#include "ExprIntrp_yaccintrf.pxx"
#include "ExprIntrp.tab.hxx"

IMPLEMENT_STANDARD_RTTIEXT(ExprIntrp_Generator,Standard_Transient)

ExprIntrp_Generator::ExprIntrp_Generator()
{
}

void ExprIntrp_Generator::Use(const Handle(Expr_NamedFunction)& func)
{
  myFunctions.Append(func);
}

void ExprIntrp_Generator::Use(const Handle(Expr_NamedExpression)& named)
{
  myNamed.Append(named);
}

const ExprIntrp_SequenceOfNamedFunction& ExprIntrp_Generator::GetFunctions() const
{
  return myFunctions;
}

const ExprIntrp_SequenceOfNamedExpression & ExprIntrp_Generator::GetNamed() const
{
  return myNamed;
}

Handle(Expr_NamedFunction) ExprIntrp_Generator::GetFunction (const TCollection_AsciiString& name) const
{
  for (Standard_Integer i=1; i<= myFunctions.Length(); i++) {
    if (name == myFunctions(i)->GetName()) {
      return myFunctions(i);
    }
  }
  Handle(Expr_NamedFunction) curfunc;
  return curfunc;
}

Handle(Expr_NamedExpression) ExprIntrp_Generator::GetNamed (const TCollection_AsciiString& name) const
{
  for (Standard_Integer i=1;i<=myNamed.Length();i++){
    if (name == myNamed(i)->GetName()) {
      return myNamed(i);
    }
  }
  Handle(Expr_NamedExpression) curexp;
  return curexp;
}

bool ExprIntrp_Generator::Parse(ExprIntrp_Analysis& theRecept,
                                const TCollection_AsciiString& theStr)
{
  if (theStr.IsEmpty())
    return false;

  Standard_ArrayStreamBuffer aBuffer(theStr.ToCString(), theStr.Length());
  std::istream aStream(&aBuffer);
  try
  {
    OCC_CATCH_SIGNALS

    ExprIntrp_yaccintrf aCtx(theRecept);

    ExprIntrp::scanner aScanner(aCtx, aStream);
    aScanner.yyrestart(&aStream);

    ExprIntrp::parser aParser(&aScanner);
    const int aRes = aParser.parse();
    if (aRes != 0 && !aCtx.ExprIntrp_GetLastError().IsEmpty())
    {
      Message::SendTrace() << "Expression '" << theStr << "' cannot be parsed, " << aCtx.ExprIntrp_GetLastError();
    }
    return aRes == 0;
  }
  catch (const Standard_Failure& anExcept)
  {
    Message::SendTrace() << "Expression '" << theStr << "' cannot be parsed: " << anExcept;
    return false;
  }
}

