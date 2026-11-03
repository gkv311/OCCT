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

#include <Expr_Secant.hxx>

#include <Expr_NamedUnknown.hxx>
#include <Expr_NumericValue.hxx>
#include <Expr_Operators.hxx>
#include <Expr_Tangent.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Expr_Secant, Expr_UnaryExpression)

// =======================================================================
// function : ShallowSimplified
// =======================================================================
Handle(Expr_GeneralExpression) Expr_Secant::ShallowSimplified() const
{
  if (Handle(Expr_NumericValue) aNumExp = Handle(Expr_NumericValue)::DownCast(myOperand))
    return new Expr_NumericValue(evaluate(aNumExp->GetValue()));

  return this;
}

// =======================================================================
// function : Derivative
// =======================================================================
Handle(Expr_GeneralExpression) Expr_Secant::Derivative(const Handle(Expr_NamedUnknown)& theX) const
{
  if (!Contains(theX))
    return new Expr_NumericValue(0.0);

  Handle(Expr_GeneralExpression) aSec  = new Expr_Secant(Expr::CopyShare(myOperand));
  Handle(Expr_GeneralExpression) aTan  = new Expr_Tangent(Expr::CopyShare(myOperand));
  // sec(x)*tan(x)
  Handle(Expr_GeneralExpression) aProd = aSec->ShallowSimplified() * aTan->ShallowSimplified();
  Handle(Expr_GeneralExpression) aRes  = aProd->ShallowSimplified() * myOperand->Derivative(theX);
  return aRes->ShallowSimplified();
}
