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

#ifndef _Expr_Cotangent_HeaderFile
#define _Expr_Cotangent_HeaderFile

#include <Expr.hxx>
#include <Expr_UnaryExpression.hxx>

//! Cotangent expression @code cot(x) = cos(x)/sin(x) @endcode
class Expr_Cotangent : public Expr_UnaryExpression
{
  DEFINE_STANDARD_RTTIEXT(Expr_Cotangent, Expr_UnaryExpression)
public:

  //! Main constructor.
  explicit Expr_Cotangent(const Handle(Expr_GeneralExpression)& theExp)
  {
    CreateOperand(theExp);
  }

  //! Returns a simplified expression (when possible) or this.
  Standard_EXPORT virtual Handle(Expr_GeneralExpression) ShallowSimplified() const override;

  //! Returns a copy of this expression having the same unknowns and functions.
  virtual Handle(Expr_GeneralExpression) Copy() const override
  {
    return new Expr_Cotangent(Expr::CopyShare(Operand()));
  }

  //! Tests if this and @p theOther define the same expression.
  virtual bool IsIdentical(const Handle(Expr_GeneralExpression)& theOther) const override
  {
    return dynamic_cast<const Expr_Cotangent*>(theOther.get()) != nullptr
        && myOperand->IsIdentical(theOther->SubExpression(1));
  }

  //! Returns TRUE if this expression is constant.
  virtual bool IsLinear() const override { return !ContainsUnknowns(); }

  //! Returns the derivative on @p theX unknown of this expression:
  //! @code cot'(x) = -csc(x)^2 @endcode
  Standard_EXPORT virtual Handle(Expr_GeneralExpression) Derivative(const Handle(Expr_NamedUnknown)& theX) const override;

  //! Evaluates expression or raises exception.
  virtual double Evaluate(const Expr_Array1OfNamedUnknown& theVars,
                          const TColStd_Array1OfReal& theVals) const override
  {
    return evaluate(myOperand->Evaluate(theVars, theVals));
  }

  //! Returns a string representing of this expression in a readable way.
  virtual TCollection_AsciiString String() const override
  {
    return TCollection_AsciiString("Cot(") + Operand()->String() + ")";
  }

private:

  //! Evaluates expression or raises exception.
  double evaluate(const double theX) const
  {
    return 1.0 / ::Tan(theX);
  }

};

#endif // _Expr_Cotangent_HeaderFile
