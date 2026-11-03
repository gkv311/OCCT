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

#include <Expr_ArcCsch.hxx>

#include <Expr_NamedUnknown.hxx>
#include <Expr_NumericValue.hxx>
#include <Expr_Operators.hxx>
#include <Expr_Square.hxx>
#include <Expr_SquareRoot.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Expr_ArcCsch, Expr_UnaryExpression)

// =======================================================================
// function : ShallowSimplified
// =======================================================================
Handle(Expr_GeneralExpression) Expr_ArcCsch::ShallowSimplified() const
{
  if (Handle(Expr_NumericValue) aNumExp = Handle(Expr_NumericValue)::DownCast(myOperand))
    return new Expr_NumericValue(evaluate(aNumExp->GetValue()));

  return this;
}

// =======================================================================
// function : Derivative
// =======================================================================
Handle(Expr_GeneralExpression) Expr_ArcCsch::Derivative(const Handle(Expr_NamedUnknown)& theX) const
{
  if (!Contains(theX))
    return new Expr_NumericValue(0.0);

  // x^2
  Handle(Expr_GeneralExpression) aSq = new Expr_Square(Expr::CopyShare(myOperand));
  // sqrt(1+1/x^2)
  Handle(Expr_GeneralExpression) aSqRoot =
    new Expr_SquareRoot(1.0 + (1.0 / aSq->ShallowSimplified())->ShallowSimplified());
  // x^2*sqrt(1+1/x^2)
  Handle(Expr_GeneralExpression) aProd = aSq->ShallowSimplified() * aSqRoot->ShallowSimplified();
  // -1/(x^2*sqrt(1+1/x^2))
  Handle(Expr_GeneralExpression) aDiv = -1.0 / aProd->ShallowSimplified();

  Handle(Expr_GeneralExpression) aRes  = aDiv->ShallowSimplified() * myOperand->Derivative(theX);
  return aRes->ShallowSimplified();
}
