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

#include <Geom2dEvaluator_ExprCurve.hxx>

#include <Expr_UnknownIterator.hxx>
#include <Standard_NumericError.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Geom2dEvaluator_ExprCurve, Geom2dEvaluator_Curve)

//! Return an empty unknown.
static const Handle(Expr_NamedUnknown)& GetEmptyUnknown()
{
  static const Handle(Expr_NamedUnknown) anEmptyVar = new Expr_NamedUnknown("");
  return anEmptyVar;
}

// =======================================================================
// function : Constructor
// =======================================================================
Geom2dEvaluator_ExprCurve::Geom2dEvaluator_ExprCurve()
{
  //
}

// =======================================================================
// function : ShallowCopy
// =======================================================================
Handle(Geom2dEvaluator_Curve) Geom2dEvaluator_ExprCurve::ShallowCopy() const
{
  Handle(Geom2dEvaluator_ExprCurve) aCopy = new Geom2dEvaluator_ExprCurve(*this);
  return aCopy;
}

// =======================================================================
// function : Load
// =======================================================================
void Geom2dEvaluator_ExprCurve::Load(const Handle(Expr_GeneralExpression)& theExprX,
                                     const Handle(Expr_GeneralExpression)& theExprY)
{
  myExpr = NCollection_Vec2<ExprComponent>();
  if (theExprX.IsNull() || theExprY.IsNull())
    throw Standard_NoSuchObject("Null expression");

  NCollection_Vec2<ExprComponent> aNewExpr;
  aNewExpr.x().Init(theExprX);
  aNewExpr.y().Init(theExprY);

  Handle(Expr_NamedUnknown) aTestUnknown;
  for (int aDimIter = 0; aDimIter < aNewExpr.Length(); ++aDimIter)
  {
    Handle(Expr_NamedUnknown)& aDimUnknown = aNewExpr[aDimIter].Variable;
    if (aDimUnknown.IsNull())
      continue;

    if (aTestUnknown.IsNull())
      aTestUnknown = aDimUnknown;
    else if (aDimUnknown->GetName() != aTestUnknown->GetName())
      throw Standard_ConstructionError("Expressions should use the same unknown");
  }

  if (aTestUnknown.IsNull())
    throw Standard_ConstructionError("At least one unknown is expected in expression");

  myExpr = aNewExpr;
}

// =======================================================================
// function : Init
// =======================================================================
void Geom2dEvaluator_ExprCurve::ExprComponent::Init(const Handle(Expr_GeneralExpression)& theExpr)
{
  for (Expr_UnknownIterator anUnknownIter(theExpr); anUnknownIter.More(); anUnknownIter.Next())
  {
    const Handle(Expr_NamedUnknown) aNewUnknown = anUnknownIter.Value();
    if (!Variable.IsNull())
      throw Standard_ConstructionError("Only one unknown is expected in expression");

    Variable = aNewUnknown;
  }

  const Handle(Expr_NamedUnknown) aVar = !Variable.IsNull() ? Variable : GetEmptyUnknown();

  Derivatives[0] = theExpr;
  for (size_t aDerIter = 1; aDerIter < Derivatives.size(); ++aDerIter)
    Derivatives[aDerIter] = Derivatives[aDerIter - 1]->Derivative(aVar);
}

// =======================================================================
// function : evaluate
// =======================================================================
bool Geom2dEvaluator_ExprCurve::evaluate(const double theU,
                                         const int theOrder,
                                         double (&theResult)[2]) const
{
  if (!IsValid() || theOrder < 0 || size_t(theOrder) >= myExpr.x().Derivatives.size())
    return false;

  Handle(Expr_NamedUnknown) aVarCArr[1] = {};
  const double aValCArr[1] = {theU};
  Expr_Array1OfNamedUnknown  aVarArr(aVarCArr[0], 1, 1);
  const TColStd_Array1OfReal aValArr(aValCArr[0], 1, 1);
  for (int aDimIter = 0; aDimIter < myExpr.Length(); ++aDimIter)
  {
    const ExprComponent& aComp = myExpr[aDimIter];

    double& aRes = theResult[aDimIter];
    aVarArr[1] = !aComp.Variable.IsNull() ? aComp.Variable : GetEmptyUnknown();
    try
    {
      aRes = aComp.Derivatives[theOrder]->Evaluate(aVarArr, aValArr);
    }
    catch (const Standard_NumericError&)
    {
      // Cos(), Sqrt(), etc.;
      // also Standard_DivideByZero with FPE enabled
      return false;
    }
    catch (const Standard_DomainError&)
    {
      // ASin() throws Standard_OutOfRange
      return false;
    }

    if (std::isnan(aRes))
      return false;
    else if (std::isinf(aRes))
      return false;
  }
  return true;
}

// =======================================================================
// function : DumpJson
// =======================================================================
void Geom2dEvaluator_ExprCurve::DumpJson(Standard_OStream& theOStream, int ) const
{
  const TCollection_AsciiString anExprXD0 = myExpr.x().String(0);
  const TCollection_AsciiString anExprYD0 = myExpr.y().String(0);
  OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprXD0);
  OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprYD0);

  const TCollection_AsciiString anExprXD1 = myExpr.x().String(1);
  const TCollection_AsciiString anExprYD1 = myExpr.y().String(1);
  OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprXD1);
  OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprYD1);

  const TCollection_AsciiString anExprXD2 = myExpr.x().String(2);
  const TCollection_AsciiString anExprYD2 = myExpr.y().String(2);
  OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprXD2);
  OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprYD2);
}
