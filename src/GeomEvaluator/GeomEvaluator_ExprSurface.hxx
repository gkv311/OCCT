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

#ifndef _GeomEvaluator_ExprSurface_HeaderFile
#define _GeomEvaluator_ExprSurface_HeaderFile

#include <Expr_GeneralExpression.hxx>
#include <GeomEvaluator_Surface.hxx>
#include <gp_Vec.hxx>
#include <NCollection_Vec3.hxx>
#include <Standard_ConstructionError.hxx>

#include <array>

//! Function evaluator 3D surface defined by per-component
//! Expr_GeneralExpression math laws from unknowns 'u' and 'v'.
class GeomEvaluator_ExprSurface : public GeomEvaluator_Surface
{
  DEFINE_STANDARD_RTTIEXT(GeomEvaluator_ExprSurface, GeomEvaluator_Surface)
public:

  //! Empty constructor.
  Standard_EXPORT GeomEvaluator_ExprSurface();

  //! Initializer.
  //! @param[in] theExprX law for x component
  //! @param[in] theExprY law for y component
  //! @param[in] theExprZ law for z component
  Standard_EXPORT void Load(const Handle(Expr_GeneralExpression)& theExprX,
                            const Handle(Expr_GeneralExpression)& theExprY,
                            const Handle(Expr_GeneralExpression)& theExprZ);

  //! Return true if surface has been defined.
  bool IsValid() const { return myExpr.x().IsValid(); } // just check the first expression

  //! Return originating expression for specified component within 1..3 range.
  const Handle(Expr_GeneralExpression)& Expression(int theComp) const
  {
    Standard_OutOfRange_Raise_if(theComp < 1 || theComp > 3, "Invalid component index");
    return myExpr[theComp - 1].Derivatives[0][0];
  }

  //! Function evaluation.
  //! @param[in] theU U parameter value
  //! @param[in] theV V parameter value
  //! @param[in] theUOrder U derivative order to calculate
  //! @param[in] theVOrder V derivative order to calculate
  //! @param[out] theResult array of Dimension() calculated components
  //! @return TRUE if evaluation completed without error
  bool EvaluateSafe(const double theU, const double theV,
                    const int theUOrder, const int theVOrder,
                    double (&theResult)[3]) const
  {
    return evaluate(theU, theV, theUOrder, theVOrder, theResult);
  }

  //! Function evaluation.
  //! @param[in] theU U parameter value
  //! @param[in] theV V parameter value
  //! @param[in] theUOrder U derivative order to calculate
  //! @param[in] theVOrder V derivative order to calculate
  //! @param[out] theResult array of Dimension() calculated components
  //! @throw Standard_ConstructionError if evaluation has failed
  void Evaluate(const double theU, const double theV,
                const int theUOrder, const int theVOrder,
                double (&theResult)[3]) const
  {
    if (evaluate(theU, theV, theUOrder, theVOrder, theResult))
      return;

    Standard_SStream aMsg;
    aMsg << "D" << (theUOrder + theVOrder);
    for (int anOrderIter = 1; anOrderIter < theUOrder; ++anOrderIter)
      aMsg << "U";

    for (int anOrderIter = 1; anOrderIter < theVOrder; ++anOrderIter)
      aMsg << "V";

    aMsg << " evaluation failed for parameter " << theU;
    throw Standard_ConstructionError(aMsg.str().c_str());
  }

  //! Function evaluation.
  //! @param[in] theU U parameter value
  //! @param[in] theV V parameter value
  //! @param[in] theUOrder U derivative order to calculate
  //! @param[in] theVOrder V derivative order to calculate
  //! @param[out] theResult array of Dimension() calculated components
  //! @throw Standard_ConstructionError if evaluation has failed
  template<typename T>
  void Evaluate(const double theU, const double theV,
                const int theUOrder, const int theVOrder,
                T& theResult) const
  {
    NCollection_Vec3<double> aVec;
    Evaluate(theU, theV, theUOrder, theVOrder, aVec.ChangeData());
    theResult.SetCoord(aVec.x(), aVec.y(), aVec.z());
  }

  //! Function evaluation.
  //! @param[in] theU U parameter value
  //! @param[in] theV V parameter value
  //! @param[in] theUOrder U derivative order to calculate
  //! @param[in] theVOrder V derivative order to calculate
  //! @param[out] theResult array of Dimension() calculated components
  //! @throw Standard_ConstructionError if evaluation has failed
  template<typename T>
  void operator()(const double theU, const double theV,
                  const int theUOrder, const int theVOrder,
                  T& theResult) const
  {
    NCollection_Vec3<double> aVec;
    Evaluate(theU, theV, theUOrder, theVOrder, aVec.ChangeData());
    theResult.SetCoord(aVec.x(), aVec.y(), aVec.z());
  }

  //! Dump object info.
  Standard_EXPORT void DumpJson(Standard_OStream& theOStream, int theDepth) const;

public:

  //! Value of surface
  virtual void D0(const double theU, const double theV, gp_Pnt& theValue) const override
  {
    Evaluate(theU, theV, 0, 0, theValue);
  }

  //! Value and first derivatives of surface
  virtual void D1(const double theU, const double theV,
                  gp_Pnt& theValue, gp_Vec& theD1U, gp_Vec& theD1V) const override
  {
    Evaluate(theU, theV, 0, 0, theValue);
    Evaluate(theU, theV, 1, 0, theD1U);
    Evaluate(theU, theV, 0, 1, theD1V);
  }

  //! Value, first and second derivatives of surface
  virtual void D2(const double theU, const double theV,
                  gp_Pnt& theValue,
                  gp_Vec& theD1U, gp_Vec& theD1V,
                  gp_Vec& theD2U, gp_Vec& theD2V, gp_Vec& theD2UV) const override
  {
    Evaluate(theU, theV, 0, 0, theValue);
    Evaluate(theU, theV, 1, 0, theD1U);
    Evaluate(theU, theV, 0, 1, theD1V);
    Evaluate(theU, theV, 2, 0, theD2U);
    Evaluate(theU, theV, 0, 2, theD2V);
    Evaluate(theU, theV, 1, 1, theD2UV);
  }

  //! Value, first, second and third derivatives of surface
  virtual void D3(const double theU, const double theV,
                  gp_Pnt& theValue,
                  gp_Vec& theD1U, gp_Vec& theD1V,
                  gp_Vec& theD2U, gp_Vec& theD2V, gp_Vec& theD2UV,
                  gp_Vec& theD3U, gp_Vec& theD3V, gp_Vec& theD3UUV, gp_Vec& theD3UVV) const override
  {
    Evaluate(theU, theV, 0, 0, theValue);
    Evaluate(theU, theV, 1, 0, theD1U);
    Evaluate(theU, theV, 0, 1, theD1V);
    Evaluate(theU, theV, 2, 0, theD2U);
    Evaluate(theU, theV, 0, 2, theD2V);
    Evaluate(theU, theV, 1, 1, theD2UV);
    Evaluate(theU, theV, 3, 0, theD3U);
    Evaluate(theU, theV, 0, 3, theD3V);
    Evaluate(theU, theV, 2, 1, theD3UUV);
    Evaluate(theU, theV, 1, 2, theD3UVV);
  }

  //! Calculates N-th derivatives of surface, where N = theDerU + theDerV.
  virtual gp_Vec DN(const double theU, const double theV,
                    const int theDerU, const int theDerV) const override
  {
    gp_Vec aRes;
    Evaluate(theU, theV, theDerU, theDerV, aRes);
    return aRes;
  }

  //! Return a copy.
  Standard_EXPORT Handle(GeomEvaluator_Surface) ShallowCopy() const override;

private:

  struct ExprComponent
  {
    std::array<std::array<Handle(Expr_GeneralExpression), 4>,4> Derivatives; // compute up to 3rd derivative

    Handle(Expr_NamedUnknown) VarU;
    Handle(Expr_NamedUnknown) VarV;

    //! Return expression for specified derivative.
    const Handle(Expr_GeneralExpression)& operator()(int theIndexU, int theIndexV) const
    {
      return Derivatives[theIndexU][theIndexV];
    }

    //! Return expression string for specified derivative.
    TCollection_AsciiString String(int theIndexU, int theIndexV) const
    {
      return !Derivatives[theIndexU][theIndexV].IsNull()
            ? Derivatives[theIndexU][theIndexV]->String()
            : TCollection_AsciiString();
    }

    //! Return TRUE if expression has been initialized.
    bool IsValid() const { return !Derivatives[0][0].IsNull(); }

    //! Initialize with the given expression.
    Standard_EXPORT void Init(const Handle(Expr_GeneralExpression)& theExpr);
  };

private:

  //! Function evaluation.
  //! @param[in] theU U parameter value
  //! @param[in] theV V parameter value
  //! @param[in] theUOrder U derivative order to calculate
  //! @param[in] theVOrder V derivative order to calculate
  //! @param[out] theResult array of 3 calculated components
  //! @return TRUE if evaluation completed without error
  Standard_EXPORT bool evaluate(const double theU, const double theV,
                                const int theUOrder, const int theVOrder,
                                double (&theResult)[3]) const;

private:

  NCollection_Vec3<ExprComponent> myExpr;

};

#endif // _GeomEvaluator_ExprSurface_HeaderFile
