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

#ifndef _Geom_ExprCurve_HeaderFile
#define _Geom_ExprCurve_HeaderFile

#include <Geom_Curve.hxx>
#include <GeomEvaluator_ExprCurve.hxx>
#include <Standard_NotImplemented.hxx>

//! 3d curve defined by per-component math expressions from one unknown.
//!
//! Usage sample:
//! @code
//!   Handle(ExprIntrp_GenExp) anExprIntrp = ExprIntrp_GenExp::Create();
//!   anExprIntrp.Process("sin(u)*2");
//!   Handle(Expr_GeneralExpression) anExprX = anExprIntrp->Expression();
//!   anExprIntrp = ExprIntrp_GenExp::Create();
//!   anExprIntrp.Process("u^2");
//!   Handle(Expr_GeneralExpression) anExprY = anExprIntrp->Expression();
//!   anExprIntrp = ExprIntrp_GenExp::Create();
//!   anExprIntrp.Process("10");
//!   Handle(Expr_GeneralExpression) anExprZ = anExprIntrp->Expression();
//!   Handle(Geom_ExprCurve) aCurveExpr = new Geom_ExprCurve(anExprX, anExprY, anExprZ, 0.0, 1.0);
//!   GeomConvert_ApproxCurve anApprox = ...;
//!   Handle(Geom_BSplineCurve) aCurveRes = anApprox.Curve();
//! @endcode
//!
//! @warning This experimental class provides incomplete implementation interface
//!          (with stub definition or throwing exception) and has limited support!
//!          Some OCCT algorithms might fail or misbehavior for this geometry.
//!          Use this curve definition to create an approximated B-Spline curve.
class Geom_ExprCurve : public Geom_Curve
{
  DEFINE_STANDARD_RTTIEXT(Geom_ExprCurve, Geom_Curve)
public:

  //! Constructor from existing definition.
  //! @param[in] theFunc initialized evaluator
  //! @param[in] theFirst lower parameter value
  //! @param[in] theLast upper parameter value
  //! @param[in] theContinuity continuity of the curve
  //! @throws Standard_ConstructionError if input parameters are invald
  Standard_EXPORT Geom_ExprCurve(const GeomEvaluator_ExprCurve& theFunc,
                                 const double theFirst,
                                 const double theLast,
                                 const GeomAbs_Shape theContinuity = GeomAbs_C2);

  //! Constructor from expressions.
  //! @param[in] theExprX law for x component
  //! @param[in] theExprY law for y component
  //! @param[in] theFirst lower parameter value
  //! @param[in] theLast upper parameter value
  //! @param[in] theContinuity continuity of the curve
  //! @throws Standard_ConstructionError if input expressions do not depend from the same single unknown
  Standard_EXPORT Geom_ExprCurve(const Handle(Expr_GeneralExpression)& theExprX,
                                 const Handle(Expr_GeneralExpression)& theExprY,
                                 const Handle(Expr_GeneralExpression)& theExprZ,
                                 const double theFirst,
                                 const double theLast,
                                 const GeomAbs_Shape theContinuity = GeomAbs_C2);

  //! Return the first value of the parametric range.
  virtual double FirstParameter() const override { return myFirst; }

  //! Return the last value of the parametric range.
  virtual double LastParameter() const override { return myLast; }

  //! Return true if curve is closed within defined parametric range.
  Standard_EXPORT virtual bool IsClosed() const override;

  //! Returns true if the degree of continuity of this curve is at least @p theN.
  Standard_EXPORT virtual bool IsCN(const int theN) const override;

  //! Accessor for continuity characteristic.
  virtual GeomAbs_Shape Continuity() const override { return myContinuity; }

  //! Computes the point.
  virtual void D0(const double theU, gp_Pnt& theP) const override
  {
    myFunc(theU, 0, theP);
  }

  //! Computes the point with its first derivative.
  virtual void D1(const double theU, gp_Pnt& theP, gp_Vec& theV) const override
  {
    myFunc(theU, 0, theP);
    myFunc(theU, 1, theV);
  }

  //! Computes the point along with the first and the second derivatives.
  virtual void D2(const double theU, gp_Pnt& theP, gp_Vec& theV1, gp_Vec& theV2) const override
  {
    myFunc(theU, 0, theP);
    myFunc(theU, 1, theV1);
    myFunc(theU, 2, theV2);
  }

  //! Computes the point along with the first, second and third derivatives.
  virtual void D3(const double theU,
                  gp_Pnt& theP, gp_Vec& theV1, gp_Vec& theV2, gp_Vec& theV3) const override
  {
    myFunc(theU, 0, theP);
    myFunc(theU, 1, theV1);
    myFunc(theU, 2, theV2);
    myFunc(theU, 3, theV3);
  }

  //! Computes the derivative of specified order.
  virtual gp_Vec DN(const double theU, const int theN) const override
  {
    gp_Vec aRes;
    myFunc.Evaluate(theU, theN, aRes);
    return aRes;
  }

  //! Return a copy.
  Standard_EXPORT virtual Handle(Geom_Geometry) Copy() const override;

  //! Dump information.
  Standard_EXPORT virtual void DumpJson(Standard_OStream& theOStream, int theDepth) const override;

public: //! @name not implemented interface

  //! Return periodic flag - not implemented (consider as non-periodic).
  virtual bool IsPeriodic() const override { return false; }

  //! Reverse curve - not implemented.
  virtual void Reverse() override { throw Standard_NotImplemented(); }

  //! Reverse parameter - not implemented.
  virtual double ReversedParameter(const double) const override { throw Standard_NotImplemented(); }

  //! Transform curve - not implemented.
  virtual void Transform(const gp_Trsf&) override { throw Standard_NotImplemented(); }

  //! Transform parameter - not implemented.
  virtual double TransformedParameter(const double, const gp_Trsf&) const override
  {
    throw Standard_NotImplemented();
  }

  //! Transform curve - not implemented.
  virtual double ParametricTransformation(const gp_Trsf&) const override { throw Standard_NotImplemented(); }

private:

  GeomEvaluator_ExprCurve myFunc;

  double myFirst = -RealLast();
  double myLast = RealLast();
  GeomAbs_Shape myContinuity = GeomAbs_C2;

};

#endif // _Geom_ExprCurve_HeaderFile
