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

#include <Geom_ExprSurface.hxx>

#include <Geom_ExprCurve.hxx>
#include <Precision.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Geom_ExprSurface, Geom_Surface)

// =======================================================================
// function : Constructor
// =======================================================================
Geom_ExprSurface::Geom_ExprSurface(const GeomEvaluator_ExprSurface& theFunc,
                                   const double theUFirst, const double theULast,
                                   const double theVFirst, const double theVLast,
                                   const GeomAbs_Shape theUContinuity, const GeomAbs_Shape theVContinuity)
  : myFunc(theFunc),
    myUFirst(theUFirst),
    myULast(theULast),
    myVFirst(theVFirst),
    myVLast(theVLast),
    myUContinuity(theUContinuity),
    myVContinuity(theVContinuity)
{
}

// =======================================================================
// function : Bounds
// =======================================================================
void Geom_ExprSurface::Bounds(double& theU1, double& theU2,
                              double& theV1, double& theV2) const
{
  theU1 = myUFirst;
  theU2 = myULast;
  theV1 = myVFirst;
  theV2 = myVLast;
}

// =======================================================================
// function : IsUClosed
// =======================================================================
bool Geom_ExprSurface::IsUClosed() const
{
  const Handle(Geom_ExprCurve) aUIsoFirst = isoCurve(myUFirst, RealLast());
  const Handle(Geom_ExprCurve) aUIsoLast  = isoCurve(myULast,  RealLast());
  if (aUIsoFirst.IsNull() || aUIsoLast.IsNull())
    return false;

  for (int aCompIter = 1; aCompIter <= 3; ++aCompIter)
  {
    // TODO - not implemented
    const TCollection_AsciiString anExpr1 = aUIsoFirst->Expression(aCompIter)->String();
    const TCollection_AsciiString anExpr2 = aUIsoLast->Expression(aCompIter)->String();
    if (anExpr1 != anExpr2)
      return false;
  }

  return true;
}

// =======================================================================
// function : IsVClosed
// =======================================================================
bool Geom_ExprSurface::IsVClosed() const
{
  const Handle(Geom_ExprCurve) aVIsoFirst = isoCurve(RealLast(), myVFirst);
  const Handle(Geom_ExprCurve) aVIsoLast  = isoCurve(RealLast(), myVLast);
  if (aVIsoFirst.IsNull() || aVIsoLast.IsNull())
    return false;

  for (int aCompIter = 1; aCompIter <= 3; ++aCompIter)
  {
    // TODO - not implemented
    const TCollection_AsciiString anExpr1 = aVIsoFirst->Expression(aCompIter)->String();
    const TCollection_AsciiString anExpr2 = aVIsoLast->Expression(aCompIter)->String();
    if (anExpr1 != anExpr2)
      return false;
  }

  return true;
}

// =======================================================================
// function : IsUPeriodic
// =======================================================================
bool Geom_ExprSurface::IsUPeriodic() const
{
  return false; // not implemented
}

// =======================================================================
// function : IsVPeriodic
// =======================================================================
bool Geom_ExprSurface::IsVPeriodic() const
{
  return false; // not implemented
}

// =======================================================================
// function : Continuity
// =======================================================================
GeomAbs_Shape Geom_ExprSurface::Continuity() const
{
  return myUContinuity < myVContinuity ? myUContinuity : myVContinuity;
}

// =======================================================================
// function : IsCNu
// =======================================================================
bool Geom_ExprSurface::IsCNu(const int theN) const
{
  switch (myUContinuity)
  {
    case GeomAbs_C0: return theN <= 0;
    case GeomAbs_G1: return theN <= 0;
    case GeomAbs_C1: return theN <= 1;
    case GeomAbs_G2: return theN <= 1;
    case GeomAbs_C2: return theN <= 2;
    case GeomAbs_C3: return theN <= 3;
    case GeomAbs_CN: return true;
  }
  return false;
}

// =======================================================================
// function : IsCNv
// =======================================================================
bool Geom_ExprSurface::IsCNv(const int theN) const
{
  switch (myVContinuity)
  {
    case GeomAbs_C0: return theN <= 0;
    case GeomAbs_G1: return theN <= 0;
    case GeomAbs_C1: return theN <= 1;
    case GeomAbs_G2: return theN <= 1;
    case GeomAbs_C2: return theN <= 2;
    case GeomAbs_C3: return theN <= 3;
    case GeomAbs_CN: return true;
  }
  return false;
}

// =======================================================================
// function : UIso
// =======================================================================
Handle(Geom_Curve) Geom_ExprSurface::UIso(const double theU) const
{
  return isoCurve(theU, RealLast());
}

// =======================================================================
// function : VIso
// =======================================================================
Handle(Geom_Curve) Geom_ExprSurface::VIso(const double theV) const
{
  return isoCurve(RealLast(), theV);
}

// =======================================================================
// function : isoCurve
// =======================================================================
Handle(Geom_ExprCurve) Geom_ExprSurface::isoCurve(const double theU, const double theV) const
{
  bool isVConst = true;
  if (theU == RealLast())
  {
    if (theV == RealLast())
      throw Standard_ProgramError("Invalid input");

    isVConst = true;
  }
  else if (theV == RealLast())
  {
    isVConst = false;
  }
  else
  {
    throw Standard_ProgramError("Invalid input");
  }

  Handle(Expr_GeneralExpression) anExpr[3] =
  {
    myFunc.Expression(1)->Copy(),
    myFunc.Expression(2)->Copy(),
    myFunc.Expression(3)->Copy(),
  };
  Handle(Geom_ExprCurve) aCurve;
  if (isVConst)
  {
    for (Handle(Expr_GeneralExpression)& anExprIter : anExpr)
      anExprIter = Expr_GeneralExpression::ReplaceConstant(anExprIter, "v", theV);

    aCurve = new Geom_ExprCurve(anExpr[0], anExpr[1], anExpr[2], myUFirst, myULast, myUContinuity);
  }
  else
  {
    for (Handle(Expr_GeneralExpression)& anExprIter : anExpr)
      anExprIter = Expr_GeneralExpression::ReplaceConstant(anExprIter, "u", theU);

    aCurve = new Geom_ExprCurve(anExpr[0], anExpr[1], anExpr[2], myVFirst, myVLast, myVContinuity);
  }

  return aCurve;
}

// =======================================================================
// function : UReverse
// =======================================================================
void Geom_ExprSurface::UReverse()
{
  throw Standard_NotImplemented();
}

// =======================================================================
// function : VReverse
// =======================================================================
void Geom_ExprSurface::VReverse()
{
  throw Standard_NotImplemented();
}

// =======================================================================
// function : UReversedParameter
// =======================================================================
double Geom_ExprSurface::UReversedParameter(const double) const
{
  throw Standard_NotImplemented();
}

// =======================================================================
// function : VReversedParameter
// =======================================================================
double Geom_ExprSurface::VReversedParameter(const double) const
{
  throw Standard_NotImplemented();
}

// =======================================================================
// function : Transform
// =======================================================================
void Geom_ExprSurface::Transform(const gp_Trsf&)
{
  throw Standard_NotImplemented();
}

// =======================================================================
// function : TransformParameters
// =======================================================================
void Geom_ExprSurface::TransformParameters(double& , double& , const gp_Trsf& ) const
{
  throw Standard_NotImplemented();
}

// =======================================================================
// function : ParametricTransformation
// =======================================================================
gp_GTrsf2d Geom_ExprSurface::ParametricTransformation(const gp_Trsf&) const
{
  throw Standard_NotImplemented();
}

// =======================================================================
// function : Copy
// =======================================================================
Handle(Geom_Geometry) Geom_ExprSurface::Copy() const
{
  Handle(Geom_ExprSurface) aCopy =
      new Geom_ExprSurface(myFunc, myUFirst, myULast, myVFirst, myVLast, myUContinuity, myVContinuity);
  return aCopy;
}

// =======================================================================
// function : DumpJson
// =======================================================================
void Geom_ExprSurface::DumpJson(Standard_OStream& theOStream, Standard_Integer theDepth) const
{
  OCCT_DUMP_TRANSIENT_CLASS_BEGIN(theOStream);
  OCCT_DUMP_BASE_CLASS(theOStream, theDepth, Geom_Surface);
  OCCT_DUMP_FIELD_VALUE_NUMERICAL(theOStream, myUFirst);
  OCCT_DUMP_FIELD_VALUE_NUMERICAL(theOStream, myULast);
  OCCT_DUMP_FIELD_VALUE_NUMERICAL(theOStream, myUContinuity);
  OCCT_DUMP_FIELD_VALUE_NUMERICAL(theOStream, myVFirst);
  OCCT_DUMP_FIELD_VALUE_NUMERICAL(theOStream, myVLast);
  OCCT_DUMP_FIELD_VALUE_NUMERICAL(theOStream, myVContinuity);
  myFunc.DumpJson(theOStream, theDepth);

}
