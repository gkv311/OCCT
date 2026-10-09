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

#include <Geom_ExprCurve.hxx>

#include <Precision.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Geom_ExprCurve, Geom_Curve)

// =======================================================================
// function : Constructor
// =======================================================================
Geom_ExprCurve::Geom_ExprCurve(const GeomEvaluator_ExprCurve& theFunc,
                               const double theFirst,
                               const double theLast,
                               const GeomAbs_Shape theContinuity)
: myFunc(theFunc),
  myFirst(theFirst),
  myLast(theLast),
  myContinuity(theContinuity)
{
  if (!theFunc.IsValid())
    throw Standard_ConstructionError("Geom_ExprCurve invalid input");
}

// =======================================================================
// function : Constructor
// =======================================================================
Geom_ExprCurve::Geom_ExprCurve(const Handle(Expr_GeneralExpression)& theExprX,
                               const Handle(Expr_GeneralExpression)& theExprY,
                               const Handle(Expr_GeneralExpression)& theExprZ,
                               const double theFirst,
                               const double theLast,
                               const GeomAbs_Shape theContinuity)
: myFirst(theFirst),
  myLast(theLast),
  myContinuity(theContinuity)
{
  myFunc.Load(theExprX, theExprY, theExprZ);
  if (!myFunc.IsValid())
    throw Standard_ConstructionError("Geom_ExprCurve invalid input");
}

// =======================================================================
// function : Copy
// =======================================================================
Handle(Geom_Geometry) Geom_ExprCurve::Copy() const
{
  Handle(Geom_ExprCurve) aCopy = new Geom_ExprCurve(myFunc, myFirst, myLast, myContinuity);
  return aCopy;
}

// =======================================================================
// function : IsCN
// =======================================================================
bool Geom_ExprCurve::IsCN(const int theN) const
{
  switch (myContinuity)
  {
    case GeomAbs_C0: return true;
    case GeomAbs_G1:
    case GeomAbs_C1: return theN <= 1;
    case GeomAbs_G2:
    case GeomAbs_C2: return theN <= 2;
    case GeomAbs_C3: return theN <= 3;
    case GeomAbs_CN: return true;
  }
  return false;
}

// =======================================================================
// function : IsClosed
// =======================================================================
bool Geom_ExprCurve::IsClosed() const
{
  if (Precision::IsPositiveInfinite(myFirst)
   || Precision::IsNegativeInfinite(myLast))
    return false;

  NCollection_Vec3<double> aPnt1, aPnt2;
  if (!myFunc.EvaluateSafe(myFirst, 0, aPnt1.ChangeData())
   || !myFunc.EvaluateSafe(myLast,  0, aPnt2.ChangeData()))
    return false;

  return (aPnt2 - aPnt1).SquareModulus() <= Precision::SquareConfusion();
}

// =======================================================================
// function : Constructor
// =======================================================================
void Geom_ExprCurve::DumpJson(Standard_OStream& theOStream, int theDepth) const
{
  OCCT_DUMP_TRANSIENT_CLASS_BEGIN(theOStream);
  OCCT_DUMP_BASE_CLASS(theOStream, theDepth, Geom_Curve);
  OCCT_DUMP_FIELD_VALUE_NUMERICAL(theOStream, myFirst);
  OCCT_DUMP_FIELD_VALUE_NUMERICAL(theOStream, myLast);
  OCCT_DUMP_FIELD_VALUE_NUMERICAL(theOStream, myContinuity);
  myFunc.DumpJson(theOStream, theDepth);
}
