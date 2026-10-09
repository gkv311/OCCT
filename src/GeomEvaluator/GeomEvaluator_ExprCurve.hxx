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

#ifndef _GeomEvaluator_ExprCurve_HeaderFile
#define _GeomEvaluator_ExprCurve_HeaderFile

#include <Expr_GeneralExpression.hxx>
#include <GeomEvaluator_Curve.hxx>
#include <gp_Vec.hxx>
#include <NCollection_Vec3.hxx>
#include <Standard_ConstructionError.hxx>

#include <array>

//! Function evaluator 3d curve defined by per-component
//! Expr_GeneralExpression math laws from one unknown.
class GeomEvaluator_ExprCurve : public GeomEvaluator_Curve
{
  DEFINE_STANDARD_RTTIEXT(GeomEvaluator_ExprCurve, GeomEvaluator_Curve)
public:

  //! Empty constructor.
  Standard_EXPORT GeomEvaluator_ExprCurve();

  //! Initializer.
  //! @param[in] theExprX law for x component
  //! @param[in] theExprY law for y component
  //! @param[in] theExprZ law for z component
  //! @throws Standard_ConstructionError if input expressions do not depend from the same single unknown
  Standard_EXPORT void Load(const Handle(Expr_GeneralExpression)& theExprX,
                            const Handle(Expr_GeneralExpression)& theExprY,
                            const Handle(Expr_GeneralExpression)& theExprZ);

  //! Return true if curve has been defined.
  bool IsValid() const { return myExpr.x().IsValid(); } // just check the first expression

  //! Return originating expression for specified component within 1..3 range.
  const Handle(Expr_GeneralExpression)& Expression(int theComp) const
  {
    Standard_OutOfRange_Raise_if(theComp < 1 || theComp > 3, "Invalid component index");
    return myExpr[theComp - 1].Derivatives[0];
  }

  //! Function evaluation that doesn't throw exceptions for invalid input.
  //! @param[in] theU parameter value
  //! @param[in] theOrder derivative order to calculate
  //! @param[out] theResult array of 3 calculated components
  //! @return TRUE if evaluation completed without error
  bool EvaluateSafe(const double theU, const int theOrder, double (&theResult)[3]) const
  {
    return evaluate(theU, theOrder, theResult);
  }

  //! Function evaluation that throws exception if value cannot be computed.
  //! @param[in] theU parameter value
  //! @param[in] theOrder derivative order to calculate
  //! @param[out] theResult array of 3 calculated components
  //! @throws Standard_ConstructionError if evaluation has failed
  void Evaluate(const double theU, const int theOrder, double (&theResult)[3]) const
  {
    if (evaluate(theU, theOrder, theResult))
      return;

    Standard_SStream aMsg;
    aMsg << "D" << theOrder << " evaluation failed for parameter " << theU;
    throw Standard_ConstructionError(aMsg.str().c_str());
  }

  //! Function evaluation.
  //! @param[in] theU parameter value
  //! @param[in] theOrder derivative order to calculate
  //! @param[out] theResult array of 3 calculated components
  //! @throws Standard_ConstructionError if evaluation has failed
  template<typename T>
  void Evaluate(const double theU, const int theOrder, T& theResult) const
  {
    NCollection_Vec3<double> aVec;
    Evaluate(theU, theOrder, aVec.ChangeData());
    theResult.SetCoord(aVec.x(), aVec.y(), aVec.z());
  }

  //! Function evaluation.
  //! @param[in] theU parameter value
  //! @param[in] theOrder derivative order to calculate
  //! @param[out] theResult array of 3 calculated components
  //! @throws Standard_ConstructionError if evaluation has failed
  template<typename T>
  void operator()(const double theU, const int theOrder, T& theResult) const
  {
    NCollection_Vec3<double> aVec;
    Evaluate(theU, theOrder, aVec.ChangeData());
    theResult.SetCoord(aVec.x(), aVec.y(), aVec.z());
  }

  //! Dump object info.
  Standard_EXPORT void DumpJson(Standard_OStream& theOStream, int theDepth) const;

public: //! @name Geom2dEvaluator_Curve interface implementation

  //! Value of curve
  virtual void D0(const double theU, gp_Pnt& theValue) const override { Evaluate(theU, 0, theValue); }

  //! Value and first derivatives of curve
  virtual void D1(const double theU, gp_Pnt& theValue, gp_Vec& theD1) const override
  {
    Evaluate(theU, 0, theValue);
    Evaluate(theU, 1, theD1);
  }

  //! Value, first and second derivatives of curve
  virtual void D2(const double theU,
                  gp_Pnt& theValue, gp_Vec& theD1, gp_Vec& theD2) const override
  {
    Evaluate(theU, 0, theValue);
    Evaluate(theU, 1, theD1);
    Evaluate(theU, 2, theD2);
  }

  //! Value, first, second and third derivatives of curve
  virtual void D3(const double theU,
                  gp_Pnt& theValue, gp_Vec& theD1, gp_Vec& theD2, gp_Vec& theD3) const override
  {
    Evaluate(theU, 0, theValue);
    Evaluate(theU, 1, theD1);
    Evaluate(theU, 2, theD2);
    Evaluate(theU, 3, theD3);
  }

  //! Calculates N-th derivatives of curve.
  virtual gp_Vec DN(const double theU, const int theDeriv) const override
  {
    gp_Vec aRes;
    Evaluate(theU, theDeriv, aRes);
    return aRes;
  }

  //! Return a copy.
  Standard_EXPORT Handle(GeomEvaluator_Curve) ShallowCopy() const override;

private:

  struct ExprComponent
  {
    std::array<Handle(Expr_GeneralExpression), 4> Derivatives; // from 0th to 3rd derivative
    Handle(Expr_NamedUnknown) Variable;

    //! Return expression string for specified derivative.
    TCollection_AsciiString String(int theIndex) const
    {
      return !Derivatives[theIndex].IsNull() ? Derivatives[theIndex]->String() : TCollection_AsciiString();
    }

    //! Return TRUE if expression has been initialized.
    bool IsValid() const { return !Derivatives[0].IsNull(); }

    //! Initialize with the given expression.
    Standard_EXPORT void Init(const Handle(Expr_GeneralExpression)& theExpr);
  };

private:

  //! Function evaluation.
  //! @param[in] theParam parameter value
  //! @param[in] theOrder derivative order to calculate
  //! @param[out] theResult array of 3 calculated components
  //! @return TRUE if evaluation completed without error
  Standard_EXPORT bool evaluate(const double theParam,
                                const int theOrder,
                                double (&theResult)[3]) const;

private:

  NCollection_Vec3<ExprComponent> myExpr;

};

#endif // _Geom2dEvaluator_ExprCurve_HeaderFile
