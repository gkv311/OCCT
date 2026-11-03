// Copyright (c) 2015 OPEN CASCADE SAS
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

#include <Expr_Sech.hxx>

#include <Expr_NamedUnknown.hxx>
#include <Expr_NumericValue.hxx>
#include <Expr_Operators.hxx>
#include <Expr_Tanh.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Expr_Sech, Expr_UnaryExpression)

// =======================================================================
// function : ShallowSimplified
// =======================================================================
Handle(Expr_GeneralExpression) Expr_Sech::ShallowSimplified() const
{
  if (Handle(Expr_NumericValue) aNumExp = Handle(Expr_NumericValue)::DownCast(myOperand))
    return new Expr_NumericValue(evaluate(aNumExp->GetValue()));

  return this;
}

// =======================================================================
// function : Derivative
// =======================================================================
Handle(Expr_GeneralExpression) Expr_Sech::Derivative(const Handle(Expr_NamedUnknown)& theX) const
{
  if (!Contains(theX))
    return new Expr_NumericValue(0.0);

  Handle(Expr_GeneralExpression) aSech = new Expr_Sech(Expr::CopyShare(myOperand));
  Handle(Expr_GeneralExpression) aTanh = new Expr_Tanh(Expr::CopyShare(myOperand));
  // -sech(x)*tanh(x)
  Handle(Expr_GeneralExpression) aProd = (-aSech)->ShallowSimplified() * aTanh->ShallowSimplified();
  Handle(Expr_GeneralExpression) aRes  = aProd->ShallowSimplified() * myOperand->Derivative(theX);
  return aRes->ShallowSimplified();
}
