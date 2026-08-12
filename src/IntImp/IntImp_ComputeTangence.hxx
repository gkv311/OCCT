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

#ifndef IntImp_ComputeTangence_HeaderFile
#define IntImp_ComputeTangence_HeaderFile

#include <gp_Vec.hxx>
#include <IntImp_ConstIsoparametric.hxx>
#include <Standard_OutOfRange.hxx>

inline IntImp_ConstIsoparametric ChoixRef (Standard_Integer theIndex)
{
  static constexpr IntImp_ConstIsoparametric staticChoixRef[4] =
  {
    IntImp_UIsoparametricOnCaro1,
    IntImp_VIsoparametricOnCaro1,
    IntImp_UIsoparametricOnCaro2,
    IntImp_VIsoparametricOnCaro2,
  };

  Standard_OutOfRange_Raise_if (theIndex < 0 || theIndex > 3, "ChoixRef() out of range")
  return staticChoixRef[theIndex];
}

//! The method detects if the given point is a tangential one or not.
//!
//! Algorithm:
//! - calculates the tangent at the intersection;
//!   using the following property from the scalar product @code a^(b^c)=b(ac)-c(ab) @endcode
//!   algorithm obtains the components of the tangent at the intersection in the 2 tangent planes
//!   ( @code t=n1^n2 @endcode, n1 normal to the first tile, n2 to the 2nd);
//! - makes sure that the tangent planes of the 2 tiles are not parallel;
//! - the components of the intersection in the tangent planes make it possible
//!   to determine the angle between the isoparametriques of a tile with the reciprocal tile;
//! - sorts the cosines in ascending order: the smallest cosine determines
//!   the best angle so the best iso to choose to find the intersection.
//!
//! @param[in]  theDPuv  U and V derivatives on surface 1, then on surface 2
//! @param[in]  theEpsUV U and V tolerances along isoparametric directions on surface 1,
//!                      then on surface 2
//! @param[out] theTgduv dp/du and dp/dv components of the tangent vector
//!                      of intersection line along U/V on surface 1, the on surface 2
//! @param[out] theTabIso iso directions sorted in descending order
//!                       to be candidate for advancement of intersection.
Standard_EXPORT Standard_Boolean IntImp_ComputeTangence(const gp_Vec (&theDPuv)[4],
                                                        const Standard_Real (&theEpsUV)[4],
                                                        Standard_Real (&theTgduv)[4],
                                                        IntImp_ConstIsoparametric (&theTabIso)[4]);

#endif
