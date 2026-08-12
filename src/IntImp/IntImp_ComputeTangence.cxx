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

#include <IntImp_ComputeTangence.hxx>

#include <algorithm>

//=======================================================================
//function : IntImp_ComputeTangence
//purpose  :
//=======================================================================
Standard_Boolean IntImp_ComputeTangence(const gp_Vec (&theDPuv)[4],
                                        const Standard_Real (&theEpsUV)[4],
                                        Standard_Real (&theTgduv)[4],
                                        IntImp_ConstIsoparametric (&theTabIso)[4])
{
  constexpr Standard_Real aTol2 = 1.e-32;
  Standard_Real aNormDuv[4];
  for (Standard_Integer i = 0; i < 4; ++i)
  {
    aNormDuv[i] = theDPuv[i].SquareMagnitude();
    if (aNormDuv[i] <= aTol2)
      return true;
  }

  gp_Vec aN1 = theDPuv[0] ^ theDPuv[1];
  const Standard_Real aM1 = aN1.SquareMagnitude();
  if (aM1 < aTol2)
    return true;

  gp_Vec aN2 = theDPuv[2] ^ theDPuv[3];
  const Standard_Real aM2 = aN2.SquareMagnitude();
  if (aM2 < aTol2)
    return true;

  aN1 /= Sqrt(aM1);
  aN2 /= Sqrt(aM2);
  for (Standard_Integer i = 0; i < 4; ++i)
    aNormDuv[i] = Sqrt(aNormDuv[i]);

  theTgduv[0] = -theDPuv[1].Dot(aN2);
  theTgduv[1] =  theDPuv[0].Dot(aN2);
  theTgduv[2] =  theDPuv[3].Dot(aN1);
  theTgduv[3] = -theDPuv[2].Dot(aN1);
  const bool isTangent = (Abs(theTgduv[0]) <= theEpsUV[0] * aNormDuv[1]
                       && Abs(theTgduv[1]) <= theEpsUV[1] * aNormDuv[0]
                       && Abs(theTgduv[2]) <= theEpsUV[2] * aNormDuv[3]
                       && Abs(theTgduv[3]) <= theEpsUV[3] * aNormDuv[2]);
  if (isTangent)
    return true;

  constexpr Standard_Real aTangDot = 0.999999999;
  const Standard_Real aCosAng = Abs(aN1.Dot(aN2));
  if (aCosAng > aTangDot)
    return true;

  Standard_Real aNormTgduv[4] =
  {
    Abs(theTgduv[1]) / aNormDuv[0], // iso u along surface 1
    Abs(theTgduv[0]) / aNormDuv[1], // iso v along surface 1
    Abs(theTgduv[3]) / aNormDuv[2], // iso u along surface 2
    Abs(theTgduv[2]) / aNormDuv[3]  // iso v along surface 2
  };

  // Sort aNormTgduv (in parallel with theTabIso)
  theTabIso[0] = IntImp_UIsoparametricOnCaro1;
  theTabIso[1] = IntImp_VIsoparametricOnCaro1;
  theTabIso[2] = IntImp_UIsoparametricOnCaro2;
  theTabIso[3] = IntImp_VIsoparametricOnCaro2;
  bool isSorted = false;
  do
  {
    isSorted = true;
    for (Standard_Integer i = 1; i < 4; ++i)
    {
      if (aNormTgduv[i - 1] > aNormTgduv[i])
      {
        isSorted = false;
        std::swap(aNormTgduv[i], aNormTgduv[i - 1]);
        std::swap(theTabIso[i], theTabIso[i - 1]);
      }
    }
  } while (!isSorted);

  return false;
}
