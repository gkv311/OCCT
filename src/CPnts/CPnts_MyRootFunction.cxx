// Copyright (c) 1995-1999 Matra Datavision
// Copyright (c) 1999-2014 OPEN CASCADE SAS
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


#include <CPnts_MyRootFunction.hxx>
#include <math_GaussSingleIntegration.hxx>
#include <Standard_DomainError.hxx>

void CPnts_MyRootFunction::Init(const CPnts_RealFunction& F,
				const Standard_Address D,
				const Standard_Integer Order)
{
  myFunction.Init(F,D);
  myOrder = Order;
}

void CPnts_MyRootFunction::Init(const Standard_Real X0,
				const Standard_Real L) 
{
  myX0 = X0;
  myL = L;
  myTol = -1; //to suppress the tolerance
}

void CPnts_MyRootFunction::Init(const Standard_Real X0,
				const Standard_Real L,
				const Standard_Real Tol) 
{
  myX0 = X0;
  myL = L;
  myTol = Tol;
}

Standard_Boolean CPnts_MyRootFunction::Value(const Standard_Real X,
					     Standard_Real& F)
{
  // Subdivided like CPnts_AbscissaPoint::Length, whose integral this inverts: on a single rule the
  // parameter returned would not agree with the length it was given.
  Standard_Real aLength = 0.0;
  if (!AdaptiveIntegrate(myFunction, myX0, X, myOrder, myTol > 0.0 ? &myTol : nullptr, aLength))
    return false;

  F = aLength - myL;
  return true;
} 

Standard_Boolean CPnts_MyRootFunction::Derivative(const Standard_Real X,  
						  Standard_Real& Df)
{
  return myFunction.Value(X,Df);
}

Standard_Boolean CPnts_MyRootFunction::Values(const Standard_Real X, 
					      Standard_Real& F, 
					      Standard_Real& Df)
{
  Standard_Real aLength = 0.0;
  if (!AdaptiveIntegrate(myFunction, myX0, X, myOrder, myTol > 0 ? &myTol : nullptr, aLength))
    return false;

  F = aLength - myL;
  return myFunction.Value(X, Df);
}

//! One theOrder-point Gauss quadrature of theF over [theU1, theU2].
static bool gaussIntegrate(math_Function& theF,
                           const Standard_Real theU1,
                           const Standard_Real theU2,
                           const Standard_Integer theOrder,
                           const Standard_Real* theTol,
                           Standard_Real& theValue)
{
  math_GaussSingleIntegration anIntegral =
    theTol != nullptr ? math_GaussSingleIntegration(theF, theU1, theU2, theOrder, *theTol)
                      : math_GaussSingleIntegration(theF, theU1, theU2, theOrder);
  if (!anIntegral.IsDone())
    return false;

  theValue = anIntegral.Value();
  return true;
}

bool CPnts_MyRootFunction::AdaptiveIntegrate(math_Function& theF,
                                             const Standard_Real theU1,
                                             const Standard_Real theU2,
                                             const Standard_Integer theOrder,
                                             const Standard_Real* theTol,
                                             Standard_Real& theValue)
{
  //! Relative agreement for two successive subdivision levels must reach.
  static constexpr double CPnts_IntegrationTolerance = 1.e-9;
  //! Ceiling on the equal parts the range may be split into.
  static constexpr int CPnts_IntegrationMaxParts = 512;

  Standard_Real aPrevious = 0.0;
  if (!gaussIntegrate(theF, theU1, theU2, theOrder, theTol, aPrevious))
  {
    return false;
  }

  for (Standard_Integer aNbParts = 2; aNbParts <= CPnts_IntegrationMaxParts; aNbParts *= 2)
  {
    const Standard_Real aStep  = (theU2 - theU1) / aNbParts;
    Standard_Real       aTotal = 0.;
    for (Standard_Integer aPart = 0; aPart < aNbParts; ++aPart)
    {
      Standard_Real aPartValue = 0.;
      if (!gaussIntegrate(theF,
                          theU1 + aPart * aStep,
                          aPart + 1 == aNbParts ? theU2 : theU1 + (aPart + 1) * aStep,
                          theOrder,
                          theTol,
                          aPartValue))
      {
        return false;
      }

      aTotal += aPartValue;
    }

    if (Abs(aTotal - aPrevious) <= CPnts_IntegrationTolerance * Abs(aTotal))
    {
      theValue = aTotal;
      return true;
    }

    aPrevious = aTotal;
  }

  theValue = aPrevious;
  return true;
}
