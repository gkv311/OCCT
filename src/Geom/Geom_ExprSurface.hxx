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

#ifndef _Geom_ExprSurface_HeaderFile
#define _Geom_ExprSurface_HeaderFile

#include <Geom_Surface.hxx>
#include <GeomEvaluator_ExprSurface.hxx>
#include <gp_GTrsf2d.hxx>
#include <Standard_NotImplemented.hxx>

class Geom_ExprCurve;

//! 3D surface defined by per-component math expressions from two unknowns.
class Geom_ExprSurface : public Geom_Surface
{
  DEFINE_STANDARD_RTTIEXT(Geom_ExprSurface, Geom_Surface)
public:

  //! Empty constructor.
  //! @param[in] theFunc initialized evaluator
  //! @param[in] theUFirst lower U parameter value
  //! @param[in] theULast  upper U parameter value
  //! @param[in] theVFirst lower V parameter value
  //! @param[in] theVLast  upper V parameter value
  //! @param[in] theUContinuity continuity along U
  //! @param[in] theVContinuity continuity along V
  //! @throws Standard_ConstructionError if input parameters are invalid
  Standard_EXPORT Geom_ExprSurface(const GeomEvaluator_ExprSurface& theFunc,
                                   const double theUFirst, const double theULast,
                                   const double theVFirst, const double theVLast,
                                   const GeomAbs_Shape theUContinuity, const GeomAbs_Shape theVContinuity);

  //! Returns the parametric bounds U1, U2, V1 and V2 of this surface.
  Standard_EXPORT virtual void Bounds(double& theU1, double& theU2,
                                      double& theV1, double& theV2) const override;

  //! Return U closed flag - not implemented (consider as not closed).
  Standard_EXPORT virtual bool IsUClosed() const override;

  //! Return V closed flag - not implemented (consider as not closed).
  Standard_EXPORT virtual bool IsVClosed() const override;

  //! Return U periodic flag - not implemented (consider as non-periodic).
  Standard_EXPORT virtual bool IsUPeriodic() const override;

  //! Return V periodic flag - not implemented (consider as non-periodic).
  Standard_EXPORT virtual bool IsVPeriodic() const override;

  //! Accessor for continuity characteristic.
  Standard_EXPORT virtual GeomAbs_Shape Continuity() const override;

  //! Returns the order of continuity of the surface in the U parametric direction.
  Standard_EXPORT virtual bool IsCNu(const int theN) const override;

  //! Returns the order of continuity of the surface in the V parametric direction.
  Standard_EXPORT virtual bool IsCNv(const int theN) const override;

  //! Computes the U isoparametric curve.
  Standard_EXPORT virtual Handle(Geom_Curve) UIso(const double theU) const override;

  //! Computes the V isoparametric curve.
  Standard_EXPORT virtual Handle(Geom_Curve) VIso(const double theV) const override;

  //! Computes the point.
  virtual void D0(const double theU, const double theV, gp_Pnt& theP) const override
  {
    myFunc(theU, theV, 0, 0, theP);
  }

  //! Computes the point with its first derivative.
  virtual void D1(const double theU, const double theV,
                  gp_Pnt& theP,
                  gp_Vec& theD1U, gp_Vec& theD1V) const override
  {
    myFunc(theU, theV, 0, 0, theP);
    myFunc(theU, theV, 1, 0, theD1U);
    myFunc(theU, theV, 0, 1, theD1V);
  }

  //! Computes the point along with the first and the second derivatives.
  virtual void D2(const double theU, const double theV,
                  gp_Pnt& theP,
                  gp_Vec& theD1U, gp_Vec& theD1V,
                  gp_Vec& theD2U, gp_Vec& theD2V, gp_Vec& theD2UV) const override
  {
    myFunc(theU, theV, 0, 0, theP);
    myFunc(theU, theV, 1, 0, theD1U);
    myFunc(theU, theV, 0, 1, theD1V);
    myFunc(theU, theV, 2, 0, theD2U);
    myFunc(theU, theV, 0, 2, theD2V);
    myFunc(theU, theV, 1, 1, theD2UV);
  }

  //! Computes the point along with the first, second and third derivatives.
  virtual void D3(const double theU, const double theV,
                  gp_Pnt& theP,
                  gp_Vec& theD1U, gp_Vec& theD1V,
                  gp_Vec& theD2U, gp_Vec& theD2V, gp_Vec& theD2UV,
                  gp_Vec& theD3U, gp_Vec& theD3V, gp_Vec& theD3UUV, gp_Vec& theD3UVV) const override
  {
    myFunc(theU, theV, 0, 0, theP);
    myFunc(theU, theV, 1, 0, theD1U);
    myFunc(theU, theV, 0, 1, theD1V);
    myFunc(theU, theV, 2, 0, theD2U);
    myFunc(theU, theV, 0, 2, theD2V);
    myFunc(theU, theV, 1, 1, theD2UV);
    myFunc(theU, theV, 3, 0, theD3U);
    myFunc(theU, theV, 0, 3, theD3V);
    myFunc(theU, theV, 2, 1, theD3UUV);
    myFunc(theU, theV, 1, 2, theD3UVV);
  }

  //! Computes the derivative of specified order.
  virtual gp_Vec DN(const double theU, const double theV,
                    const int theNu, const int theNv) const override
  {
    gp_Vec aVec;
    myFunc(theU, theV, theNu, theNv, aVec);
    return aVec;
  }

  //! Return a copy.
  Standard_EXPORT virtual Handle(Geom_Geometry) Copy() const override;

  //! Dump information.
  Standard_EXPORT virtual void DumpJson(Standard_OStream& theOStream, int theDepth) const override;

public:

  //! Reverse U - not implemented.
  Standard_EXPORT virtual void UReverse() override;

  //! Reverse V - not implemented.
  Standard_EXPORT virtual void VReverse() override;

  //! Reverse U parameter - not implemented.
  Standard_EXPORT virtual double UReversedParameter(const double) const override;

  //! Reverse V parameter - not implemented.
  Standard_EXPORT virtual double VReversedParameter(const double) const override;

  //! Transform surface - not implemented.
  Standard_EXPORT virtual void Transform(const gp_Trsf&) override;

  //! Transform parameters - not implemented.
  Standard_EXPORT virtual void TransformParameters(double& , double& , const gp_Trsf& ) const override;

  //! Transform surface - not implemented.
  Standard_EXPORT virtual gp_GTrsf2d ParametricTransformation(const gp_Trsf&) const override;

private:

  //! Computes the isoparametric curve.
  Standard_EXPORT Handle(Geom_ExprCurve) isoCurve(const double theU, const double theV) const;

private:

  GeomEvaluator_ExprSurface myFunc;

  double myUFirst = -RealLast();
  double myULast = RealLast();
  double myVFirst = -RealLast();
  double myVLast = RealLast();

  GeomAbs_Shape myUContinuity = GeomAbs_C2;
  GeomAbs_Shape myVContinuity = GeomAbs_C2;
};

#endif // _Geom_ExprSurface_HeaderFile
