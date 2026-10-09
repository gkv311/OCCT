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

#include <GeomEvaluator_ExprSurface.hxx>

#include <Expr_UnknownIterator.hxx>
#include <Standard_NumericError.hxx>

IMPLEMENT_STANDARD_RTTIEXT(GeomEvaluator_ExprSurface, GeomEvaluator_Surface)

//! Return an empty unknown.
static const Handle(Expr_NamedUnknown)& GetEmptyUnknown()
{
  static const Handle(Expr_NamedUnknown) anEmptyVar = new Expr_NamedUnknown("");
  return anEmptyVar;
}

// =======================================================================
// function : Constructor
// =======================================================================
GeomEvaluator_ExprSurface::GeomEvaluator_ExprSurface()
{
  //
}

// =======================================================================
// function : ShallowCopy
// =======================================================================
Handle(GeomEvaluator_Surface) GeomEvaluator_ExprSurface::ShallowCopy() const
{
  Handle(GeomEvaluator_ExprSurface) aCopy = new GeomEvaluator_ExprSurface(*this);
  return aCopy;
}

// =======================================================================
// function : Load
// =======================================================================
void GeomEvaluator_ExprSurface::Load(const Handle(Expr_GeneralExpression)& theExprX,
                                     const Handle(Expr_GeneralExpression)& theExprY,
                                     const Handle(Expr_GeneralExpression)& theExprZ)
{
  myExpr = NCollection_Vec3<ExprComponent>();
  if (theExprX.IsNull() || theExprY.IsNull() || theExprZ.IsNull())
    throw Standard_NoSuchObject("Null expression");

  NCollection_Vec3<ExprComponent> aNewExpr;
  aNewExpr.x().Init(theExprX);
  aNewExpr.y().Init(theExprY);
  aNewExpr.z().Init(theExprZ);

  Handle(Expr_NamedUnknown) aTestU, aTestV;
  for (int aDimIter = 0; aDimIter < aNewExpr.Length(); ++aDimIter)
  {
    if (!aNewExpr[aDimIter].VarU.IsNull())
      aTestU = aNewExpr[aDimIter].VarU;

    if (!aNewExpr[aDimIter].VarV.IsNull())
      aTestV = aNewExpr[aDimIter].VarV;
  }

  if (aTestU.IsNull() || aTestV.IsNull())
    throw Standard_ConstructionError("Unknowns 'u' and 'v' are expected in expression");

  myExpr = aNewExpr;
}

// =======================================================================
// function : Init
// =======================================================================
void GeomEvaluator_ExprSurface::ExprComponent::Init(const Handle(Expr_GeneralExpression)& theExpr)
{
  for (Expr_UnknownIterator anUnknownIter(theExpr); anUnknownIter.More(); anUnknownIter.Next())
  {
    const Handle(Expr_NamedUnknown) aNewUnknown = anUnknownIter.Value();
    if (aNewUnknown->GetName() == "u")
      VarU = aNewUnknown;
    else if (aNewUnknown->GetName() == "v")
      VarV = aNewUnknown;
    else
      throw Standard_RangeError("Only 'u' and 'v' unknowns are expected in expression");
  }

  const Handle(Expr_NamedUnknown) aVarU = !VarU.IsNull() ? VarU : GetEmptyUnknown();
  const Handle(Expr_NamedUnknown) aVarV = !VarV.IsNull() ? VarV : GetEmptyUnknown();

  Derivatives[0][0] = theExpr;
  for (size_t aDerUIter = 0; aDerUIter < Derivatives.size(); ++aDerUIter)
  {
    const size_t aNbDer = Derivatives.size() - aDerUIter;
    for (size_t aDerVIter = 0; aDerVIter < aNbDer; ++aDerVIter)
    {
      if (aDerUIter > 0)
        Derivatives[aDerUIter][aDerVIter] = Derivatives[aDerUIter - 1][aDerVIter]->Derivative(aVarU);
      else if (aDerVIter > 0)
        Derivatives[aDerUIter][aDerVIter] = Derivatives[aDerUIter][aDerVIter - 1]->Derivative(aVarV);
    }
  }
}

// =======================================================================
// function : evaluate
// =======================================================================
bool GeomEvaluator_ExprSurface::evaluate(const double theU, const double theV,
                                         const int theUOrder, const int theVOrder,
                                         double (&theResult)[3]) const
{
  const int anOrder = theUOrder + theVOrder;
  if (!IsValid() || theUOrder < 0 || theVOrder < 0)
    return false;
  else if (size_t(anOrder) >= myExpr.x().Derivatives.size())
    return false;

  Handle(Expr_NamedUnknown) aVarCArr[2] = {};
  const double              aValCArr[2] = {theU, theV};

  Expr_Array1OfNamedUnknown  aVarArr(aVarCArr[0], 1, 2);
  const TColStd_Array1OfReal aValArr(aValCArr[0], 1, 2);
  for (int aDimIter = 0; aDimIter < 3; ++aDimIter)
  {
    double& aRes = theResult[aDimIter];
    const ExprComponent& anExprComp = myExpr[aDimIter];
    aVarArr[1] = !anExprComp.VarU.IsNull() ? anExprComp.VarU : GetEmptyUnknown();
    aVarArr[2] = !anExprComp.VarV.IsNull() ? anExprComp.VarV : GetEmptyUnknown();
    try
    {
      aRes = anExprComp(theUOrder, theVOrder)->Evaluate(aVarArr, aValArr);
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
void GeomEvaluator_ExprSurface::DumpJson(Standard_OStream& theOStream, int ) const
{
  {
    const TCollection_AsciiString anExprXD0 = myExpr.x().String(0, 0);
    const TCollection_AsciiString anExprYD0 = myExpr.y().String(0, 0);
    const TCollection_AsciiString anExprZD0 = myExpr.z().String(0, 0);
    OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprXD0);
    OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprYD0);
    OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprZD0);
  }
  {
    const TCollection_AsciiString anExprXD1U = myExpr.x().String(1, 0);
    const TCollection_AsciiString anExprYD1U = myExpr.y().String(1, 0);
    const TCollection_AsciiString anExprZD1U = myExpr.z().String(1, 0);
    OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprXD1U);
    OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprYD1U);
    OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprZD1U);
  }
  {
    const TCollection_AsciiString anExprXD1V = myExpr.x().String(0, 1);
    const TCollection_AsciiString anExprYD1V = myExpr.y().String(0, 1);
    const TCollection_AsciiString anExprZD1V = myExpr.z().String(0, 1);
    OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprXD1V);
    OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprYD1V);
    OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprZD1V);
  }
  {
    const TCollection_AsciiString anExprXD2U = myExpr.x().String(2, 0);
    const TCollection_AsciiString anExprYD2U = myExpr.y().String(2, 0);
    const TCollection_AsciiString anExprZD2U = myExpr.z().String(2, 0);
    OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprXD2U);
    OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprYD2U);
    OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprZD2U);
  }
  {
    const TCollection_AsciiString anExprXD2V = myExpr.x().String(0, 2);
    const TCollection_AsciiString anExprYD2V = myExpr.y().String(0, 2);
    const TCollection_AsciiString anExprZD2V = myExpr.z().String(0, 2);
    OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprXD2V);
    OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprYD2V);
    OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprZD2V);
  }
  {
    const TCollection_AsciiString anExprXD2UV = myExpr.x().String(1, 1);
    const TCollection_AsciiString anExprYD2UV = myExpr.y().String(1, 1);
    const TCollection_AsciiString anExprZD2UV = myExpr.z().String(1, 1);
    OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprXD2UV);
    OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprYD2UV);
    OCCT_DUMP_FIELD_VALUE_STRING(theOStream, anExprZD2UV);
  }
}
