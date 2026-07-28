// Created on: 1995-07-18
// Created by: Modelistation
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

//  Modified by skv - Thu Sep 30 15:21:07 2004 OCC593

#include <Extrema_GenExtPS.hxx>

#include <Bnd_HArray1OfSphere.hxx>
#include <Bnd_Sphere.hxx>
#include <Extrema_ExtFlag.hxx>
#include <Extrema_HUBTreeOfSphere.hxx>
#include <Extrema_POnSurf.hxx>
#include <Extrema_POnSurfParams.hxx>
#include <Geom_BezierCurve.hxx>
#include <Geom_BezierSurface.hxx>
#include <Geom_BSplineCurve.hxx>
#include <Geom_OffsetSurface.hxx>
#include <GCPnts_QuasiUniformAbscissa.hxx>
#include <gp_Pnt.hxx>
#include <math_FunctionSetRoot.hxx>
#include <math_NewtonFunctionSetRoot.hxx>
#include <math_Vector.hxx>
#include <Precision.hxx>
#include <Standard_OutOfRange.hxx>
#include <StdFail_NotDone.hxx>
#include <TColStd_Array2OfInteger.hxx>

//IMPLEMENT_HARRAY1(Extrema_HArray1OfSphere)
class Bnd_SphereUBTreeSelector : public Extrema_UBTreeOfSphere::Selector
{
 public:

  Bnd_SphereUBTreeSelector (const Handle(Bnd_HArray1OfSphere)& theSphereArray,
Bnd_Sphere& theSol)
    : myXYZ(0,0,0),
      mySphereArray(theSphereArray),
      mySol(theSol)
  {
  }

  void DefineCheckPoint( const gp_Pnt& theXYZ )
  { myXYZ = theXYZ; }

  Bnd_Sphere& Sphere() const
  { return mySol; }

  virtual Standard_Boolean Reject( const Bnd_Sphere &theBnd ) const = 0;
  
  virtual Standard_Boolean Accept(const Standard_Integer& theObj) = 0;
 protected:
  gp_Pnt                              myXYZ;
  const Handle(Bnd_HArray1OfSphere)&  mySphereArray;
  Bnd_Sphere&                         mySol;
 private:
  void operator= (const Bnd_SphereUBTreeSelector&);

};

class Bnd_SphereUBTreeSelectorMin : public Bnd_SphereUBTreeSelector
{
public:
  Bnd_SphereUBTreeSelectorMin (const Handle(Bnd_HArray1OfSphere)& theSphereArray,
		Bnd_Sphere& theSol)
		: Bnd_SphereUBTreeSelector(theSphereArray, theSol),
		  myMinDist(RealLast())
  {}
  
  void SetMinDist( const Standard_Real theMinDist )
  { myMinDist = theMinDist; }

  Standard_Real MinDist() const
  { return myMinDist; }

  Standard_Boolean Reject( const Bnd_Sphere &theBnd ) const
  { 
    Bnd_SphereUBTreeSelectorMin* me =
      const_cast<Bnd_SphereUBTreeSelectorMin*>(this);
    // myMinDist is decreased each time a nearer object is found
    return theBnd.IsOut( myXYZ.XYZ(), me->myMinDist );
  }

  Standard_Boolean Accept(const Standard_Integer&);

private:
	Standard_Real	myMinDist;
};

Standard_Boolean Bnd_SphereUBTreeSelectorMin::Accept(const Standard_Integer& theInd)
{
  const Bnd_Sphere& aSph = mySphereArray->Value(theInd);
  Standard_Real aCurDist;

//    if ( (aCurDist = aSph.SquareDistance(myXYZ.XYZ())) < mySol.SquareDistance(myXYZ.XYZ()) )
    if ( (aCurDist = aSph.Distance(myXYZ.XYZ())) < mySol.Distance(myXYZ.XYZ()) )
    {
      mySol = aSph;
      if ( aCurDist < myMinDist ) 
        myMinDist = aCurDist;

      return Standard_True;
    }

  return Standard_False;
}

class Bnd_SphereUBTreeSelectorMax : public Bnd_SphereUBTreeSelector
{
public:
  Bnd_SphereUBTreeSelectorMax (const Handle(Bnd_HArray1OfSphere)& theSphereArray,
		Bnd_Sphere& theSol)
		: Bnd_SphereUBTreeSelector(theSphereArray, theSol),
		  myMaxDist(0)
  {}

  void SetMaxDist( const Standard_Real theMaxDist )
  { myMaxDist = theMaxDist; }

  Standard_Real MaxDist() const
  { return myMaxDist; }

  Standard_Boolean Reject( const Bnd_Sphere &theBnd ) const
  { 
    Bnd_SphereUBTreeSelectorMax* me =
      const_cast<Bnd_SphereUBTreeSelectorMax*>(this);
    // myMaxDist is decreased each time a nearer object is found
    return theBnd.IsOut( myXYZ.XYZ(), me->myMaxDist );
  }

  Standard_Boolean Accept(const Standard_Integer&);

private:
	Standard_Real	myMaxDist;
};

Standard_Boolean Bnd_SphereUBTreeSelectorMax::Accept(const Standard_Integer& theInd)
{
  const Bnd_Sphere& aSph = mySphereArray->Value(theInd);
  Standard_Real aCurDist;

//    if ( (aCurDist = aSph.SquareDistance(myXYZ.XYZ())) > mySol.SquareDistance(myXYZ.XYZ()) )
    if ( (aCurDist = aSph.Distance(myXYZ.XYZ())) > mySol.Distance(myXYZ.XYZ()) )
    {
      mySol = aSph;
      if ( aCurDist > myMaxDist ) 
        myMaxDist = aCurDist;

      return Standard_True;
    }

  return Standard_False;
}

//=============================================================================

/*-----------------------------------------------------------------------------
Function:
   Find all extremum distances between point P and surface
  S using sampling (NbU,NbV).

Method:
   The algorithm bases on the hypothesis that sampling is precise enough, 
  if there exist N extreme distances between the point and the surface,
  so there also exist N extrema between the point and the grid.
  So, the algorithm consists in starting from extrema of the grid to find the 
  extrema of the surface.
  The extrema are calculated by the algorithm math_FunctionSetRoot with the
  following arguments:
  - F: Extrema_FuncExtPS created from P and S,
  - UV: math_Vector the components which of are parameters of the extremum on the 
    grid,
  - Tol: Min(TolU,TolV), (Prov.:math_FunctionSetRoot does not autorize a vector)
  - UVinf: math_Vector the components which of are lower limits of u and v,
  - UVsup: math_Vector the components which of are upper limits of u and v.

Processing:
  a- Creation of the table of distances (TbDist(0,NbU+1,0,NbV+1)):
     The table is expanded at will; lines 0 and NbU+1 and
     columns 0 and NbV+1 are initialized at RealFirst() or RealLast()
     to simplify the tests carried out at stage b
     (there is no need to test if the point is on border of the grid).
  b- Calculation of extrema:
     First the minimums and then the maximums are found. These 2 procedured 
     pass in a similar way:
  b.a- Initialization:
      - 'borders' of table  TbDist (RealLast() in case of minimums
        and  RealLast() in case of maximums),
      - table TbSel(0,NbU+1,0,NbV+1) of selection of points for 
        calculation of local extremum (0). When a point will selected,
	it will not be selectable, as well as the adjacent points
	(8 at least). The corresponding addresses will be set to 1.
  b.b- Calculation of minimums (or maximums):
       All distances from table TbDist are parsed in a loop:
      - search minimum (or maximum) in the grid,
      - calculate extremum on the surface,
      - update table TbSel.
-----------------------------------------------------------------------------*/

Extrema_GenExtPS::Extrema_GenExtPS()
{
  myGrid.reset(new PointGrid());
}

// =======================================================================
// function : ~Extrema_GenExtPS
// purpose  :
// =======================================================================
Extrema_GenExtPS::~Extrema_GenExtPS()
{
  //
}

// =======================================================================
// function : Extrema_GenExtPS
// purpose  :
// =======================================================================
Extrema_GenExtPS::Extrema_GenExtPS (const gp_Pnt&          P,
                                    const Adaptor3d_Surface& S,
                                    const Standard_Integer NbU, 
                                    const Standard_Integer NbV,
                                    const Standard_Real    TolU, 
                                    const Standard_Real    TolV,
                                    const Extrema_ExtFlag F,
                                    const Extrema_ExtAlgo A) 
: myF (P,S),
  myFlag(F),
  myAlgo(A)
{
  myGrid.reset(new PointGrid());
  Initialize(S, NbU, NbV, TolU, TolV);
  Perform(P);
}

Extrema_GenExtPS::Extrema_GenExtPS (const gp_Pnt&          P,
                                    const Adaptor3d_Surface& S,
                                    const Standard_Integer NbU, 
                                    const Standard_Integer NbV,
                                    const Standard_Real    Umin,
                                    const Standard_Real    Usup,
                                    const Standard_Real    Vmin,
                                    const Standard_Real    Vsup,
                                    const Standard_Real    TolU, 
                                    const Standard_Real    TolV,
                                    const Extrema_ExtFlag F,
                                    const Extrema_ExtAlgo A) 
: myF (P,S),
  myFlag(F),
  myAlgo(A)
{
  myGrid.reset(new PointGrid());
  Initialize(S, NbU, NbV, Umin, Usup, Vmin, Vsup, TolU, TolV);
  Perform(P);
}


void Extrema_GenExtPS::Initialize(const Adaptor3d_Surface& S,
                                  const Standard_Integer NbU, 
                                  const Standard_Integer NbV,
                                  const Standard_Real    TolU, 
                                  const Standard_Real    TolV)
{
  myumin = S.FirstUParameter();
  myusup = S.LastUParameter();
  myvmin = S.FirstVParameter();
  myvsup = S.LastVParameter();
  Initialize(S,NbU,NbV,myumin,myusup,myvmin,myvsup,TolU,TolV);
}


void Extrema_GenExtPS::Initialize(const Adaptor3d_Surface& S,
                                  const Standard_Integer NbU, 
                                  const Standard_Integer NbV,
                                  const Standard_Real    Umin,
                                  const Standard_Real    Usup,
                                  const Standard_Real    Vmin,
                                  const Standard_Real    Vsup,
                                  const Standard_Real    TolU, 
                                  const Standard_Real    TolV)
{
  myS = &S;
  myusample = NbU;
  myvsample = NbV;
  mytolu = TolU;
  mytolv = TolV;
  myumin = Umin;
  myusup = Usup;
  myvmin = Vmin;
  myvsup = Vsup;

  if ((myusample < 2) ||
      (myvsample < 2)) { throw Standard_OutOfRange(); }

  myF.Initialize(S);

  mySphereUBTree.Nullify();
  myGrid->UParams.clear();
  myGrid->VParams.clear();
  myInit = Standard_False;
}

static void fillParams (const TColStd_Array1OfReal& theKnots,
                        Standard_Integer theDegree,
                        Standard_Real theParMin,
                        Standard_Real theParMax,
                        std::vector<Standard_Real, NCollection_StdAllocator<Standard_Real>>& theParams,
                        Standard_Integer theSample)
{
  theParams.clear();
  theParams.reserve(theKnots.Length() * theDegree);

  Standard_Real aPrevPar = theParMin;
  theParams.push_back(theParMin);
  //calculation the array of parametric points depending on the knots array variation and degree of given surface
  for (Standard_Integer i = 1; i < theKnots.Length() && theKnots[i] < (theParMax - Precision::PConfusion()); ++i)
  {
    if (  theKnots(i+1) < theParMin + Precision::PConfusion())
      continue;

    Standard_Real aStep = (theKnots(i+1) - theKnots(i))/Max(theDegree,2);
    for (Standard_Integer k = 1; k <= theDegree; ++k)
    {
      Standard_Real aPar = theKnots(i) + k * aStep;
      if(aPar > theParMax - Precision::PConfusion())
        break;
      if(aPar > aPrevPar + Precision::PConfusion() )
      {
        theParams.push_back(aPar);
        aPrevPar = aPar;
      }
    }

  }
  theParams.push_back(theParMax);

  //in case of an insufficient number of points the grid will be built later
  if (theParams.size() < (size_t)theSample)
    theParams.clear();
}

//! Build equi-distant distribution of points along the curve with the help of GCPnts_QuasiUniformAbscissa.
static void buildEquiDistantGrid(const Handle(Adaptor3d_Curve)& theCurve,
                                 const Standard_Real theParMin,
                                 const Standard_Real theParMax,
                                 const Standard_Integer theSample,
                                 std::vector<Standard_Real, NCollection_StdAllocator<Standard_Real>>& theParams)
{
  GCPnts_QuasiUniformAbscissa anAbscissa(*theCurve, theSample, theParMin, theParMax);
  if (!anAbscissa.IsDone() ||anAbscissa.NbPoints() < theSample)
    return;

  const Standard_Integer nbPoints = anAbscissa.NbPoints();
  theParams.resize(nbPoints);
  for (Standard_Integer i = 1; i <= nbPoints; ++i)
    theParams[i - 1] = anAbscissa.Parameter(i);
}

void Extrema_GenExtPS::GetGridPoints( const Adaptor3d_Surface& theSurf)
{
  //creation parametric points for BSpline and Bezier surfaces
  //with taking into account of Degree and NbKnots of BSpline or Bezier geometry
  if (theSurf.GetType() == GeomAbs_OffsetSurface)
  {
    GetGridPoints (*theSurf.BasisSurface());
  }
  //parametric points for BSpline surfaces
  else if( theSurf.GetType() == GeomAbs_BSplineSurface) 
  {
    Handle(Geom_BSplineSurface) aBspl = theSurf.BSpline();
    if(!aBspl.IsNull())
    {
      fillParams(aBspl->UKnots(), aBspl->UDegree(), myumin, myusup, myGrid->UParams, myusample);
      fillParams(aBspl->VKnots(), aBspl->VDegree(), myvmin, myvsup, myGrid->VParams, myvsample);
    }
  }
  //calculation parametric points for Bezier surfaces
  else if(theSurf.GetType() == GeomAbs_BezierSurface)
  {
    Handle(Geom_BezierSurface) aBezier = theSurf.Bezier();
    if(aBezier.IsNull())
      return;

    Standard_Real aKnotsU[2] = {};
    Standard_Real aKnotsV[2] = {};
    aBezier->Bounds(aKnotsU[0], aKnotsU[1], aKnotsV[0], aKnotsV[1]);
    fillParams(TColStd_Array1OfReal(aKnotsU[0], 1, 2), aBezier->UDegree(), myumin, myusup, myGrid->UParams, myusample);
    fillParams(TColStd_Array1OfReal(aKnotsV[0], 1, 2), aBezier->VDegree(), myvmin, myvsup, myGrid->VParams, myvsample);
  }
  //creation points for surfaces based on BSpline or Bezier curves
  else if(theSurf.GetType() == GeomAbs_SurfaceOfRevolution || 
    theSurf.GetType() == GeomAbs_SurfaceOfExtrusion)
  {
    if (theSurf.BasisCurve()->GetType() == GeomAbs_BSplineCurve)
    {
      Handle(Geom_BSplineCurve) aBspl = theSurf.BasisCurve()->BSpline();
      if (!aBspl.IsNull())
      {
        if (theSurf.GetType() == GeomAbs_SurfaceOfRevolution)
          fillParams(aBspl->Knots(), aBspl->Degree(), myvmin, myvsup, myGrid->VParams, myvsample);
        else
          fillParams(aBspl->Knots(), aBspl->Degree(), myumin, myusup, myGrid->UParams, myusample);
      }
    }
    else if (theSurf.BasisCurve()->GetType() == GeomAbs_BezierCurve)
    {
      Handle(Geom_BezierCurve) aBez = theSurf.BasisCurve()->Bezier();
      if (!aBez.IsNull())
      {
        Standard_Real aParams[2] = { aBez->FirstParameter(), aBez->LastParameter() };
        TColStd_Array1OfReal aKnots(aParams[0], 1, 2);
        if (theSurf.GetType() == GeomAbs_SurfaceOfRevolution)
          fillParams(aKnots, aBez->Degree(), myvmin, myvsup, myGrid->VParams, myvsample);
        else
          fillParams(aKnots, aBez->Degree(), myumin, myusup, myGrid->UParams, myusample);
      }
    }

    if (theSurf.GetType() == GeomAbs_SurfaceOfExtrusion && myGrid->UParams.empty())
      buildEquiDistantGrid (theSurf.BasisCurve(), myumin, myusup, myusample, myGrid->UParams);
    else if (theSurf.GetType() == GeomAbs_SurfaceOfRevolution && myGrid->VParams.empty())
      buildEquiDistantGrid (theSurf.BasisCurve(), myvmin, myvsup, myvsample, myGrid->VParams);
  }

  if (!myGrid->UParams.empty())
    myusample = static_cast<Standard_Integer>(myGrid->UParams.size());
  if (!myGrid->VParams.empty())
    myvsample = static_cast<Standard_Integer>(myGrid->VParams.size());
}

/*
 * This function computes the point on surface parameters on edge.
 * if it coincides with theParam0 or theParam1, it is returned.
 */
void Extrema_GenExtPS::ComputeEdgeParameters (Extrema_POnSurfParams&       theOutParam,
                                              const Standard_Boolean       IsUEdge,
                                              const Extrema_POnSurfParams &theParam0,
                                              const Extrema_POnSurfParams &theParam1,
                                              const gp_Pnt                &thePoint,
                                              const Standard_Real          theDiffTol)
{
  const Standard_Real aSqrDist01 =
    theParam0.Value().SquareDistance(theParam1.Value());

  if (aSqrDist01 <= theDiffTol)
  {
    // The points are confused. Get the first point and change its type.
    theOutParam = theParam0;
    return;
  }
  else
  {
    const Standard_Real aDiffDist =
      Abs(theParam0.GetSqrDistance() - theParam1.GetSqrDistance());

    if (aDiffDist >= aSqrDist01 - theDiffTol)
    {
      // The shortest distance is one of the nodes.
      if (theParam0.GetSqrDistance() > theParam1.GetSqrDistance())
      {
        // The shortest distance is the point 1.
        theOutParam = theParam1;
        return;
      }
      else
      {
        // The shortest distance is the point 0.
        theOutParam = theParam0;
        return;
      }
    }
    else
    {
      // The shortest distance is inside the edge.
      gp_XYZ aPoP(thePoint.XYZ().Subtracted(theParam0.Value().XYZ()));
      gp_XYZ aPoP1(theParam1.Value().XYZ().Subtracted(theParam0.Value().XYZ()));
      Standard_Real aRatio = aPoP.Dot(aPoP1)/aSqrDist01;
      // clamp the region explicitly to protect from FPE in case of floating point inaccuracy
      aRatio = Max(0.0, Min(1.0, aRatio));

      Standard_Real aU[2];
      Standard_Real aV[2];

      theParam0.Parameter(aU[0], aV[0]);
      theParam1.Parameter(aU[1], aV[1]);

      Standard_Real aUPar = aU[0];
      Standard_Real aVPar = aV[0];

      if (IsUEdge)
      {
        aUPar += aRatio*(aU[1] - aU[0]);
      }
      else
      {
        aVPar += aRatio*(aV[1] - aV[0]);
      }

      theOutParam.SetParameters(aUPar, aVPar, myS->Value(aUPar, aVPar));
      Standard_Integer anIndices[2];

      theParam0.GetIndices(anIndices[0], anIndices[1]);
      theOutParam.SetElementType(IsUEdge ? Extrema_UIsoEdge : Extrema_VIsoEdge);
      theOutParam.SetSqrDistance(thePoint.SquareDistance(theOutParam.Value()));
      theOutParam.SetIndices(anIndices[0], anIndices[1]);
    }
  }
}

void Extrema_GenExtPS::BuildGrid(const gp_Pnt &thePoint)
{
  Standard_Integer NoU, NoV;

  //if grid was already built skip its creation
  if (!myInit) {
    //build parametric grid in case of a complex surface geometry (BSpline and Bezier surfaces)
    GetGridPoints(*myS);

    //build grid in other cases
    if (myGrid->UParams.empty())
      myGrid->BuildParamGrid (myGrid->UParams, myumin, myusup, myusample);

    if (myGrid->VParams.empty())
      myGrid->BuildParamGrid (myGrid->VParams, myvmin, myvsup, myvsample);

    myGrid->Resize (myGrid->Points,         myusample + 2, myvsample + 2);
    myGrid->Resize (myGrid->FacePntParams,  myusample + 1, myvsample + 1);
    myGrid->Resize (myGrid->UEdgePntParams, myusample - 1, myvsample);
    myGrid->Resize (myGrid->VEdgePntParams, myusample,     myvsample - 1);

    // Calculation of distances
    for ( NoU = 1 ; NoU <= myusample; NoU++ ) {
      for ( NoV = 1 ; NoV <= myvsample; NoV++) {
        const gp_Pnt aP1 = myS->Value(myGrid->UParams[NoU - 1], myGrid->VParams[NoV - 1]);
        Extrema_POnSurfParams& aParam = myGrid->Points[NoU][NoV];
        aParam.SetParameters(myGrid->UParams[NoU - 1], myGrid->VParams[NoV - 1], aP1);
        aParam.SetElementType(Extrema_Node);
        aParam.SetIndices(NoU, NoV);
      }
    }

    // Fill boundary with negative square distance.
    // It is used for computation of Maximum.
    for (NoV = 0; NoV <= myvsample + 1; NoV++) {
      myGrid->Points[0][NoV].SetSqrDistance(-1.);
      myGrid->Points[myusample + 1][NoV].SetSqrDistance(-1.);
    }

    for (NoU = 1; NoU <= myusample; NoU++) {
      myGrid->Points[NoU][0].SetSqrDistance(-1.);
      myGrid->Points[NoU][myvsample + 1].SetSqrDistance(-1.);
    }

    // Fill boundary with RealLast square distance.
    for (NoV = 0; NoV <= myvsample; NoV++) {
      myGrid->FacePntParams[0][NoV].SetSqrDistance(RealLast());
      myGrid->FacePntParams[myusample][NoV].SetSqrDistance(RealLast());
    }

    for (NoU = 1; NoU < myusample; NoU++) {
      myGrid->FacePntParams[NoU][0].SetSqrDistance(RealLast());
      myGrid->FacePntParams[NoU][myvsample].SetSqrDistance(RealLast());
    }

    myInit = Standard_True;
  }

  // Compute distances to mesh.
  // Step 1. Compute distances to nodes.
  for ( NoU = 1 ; NoU <= myusample; NoU++ ) {
    for ( NoV = 1 ; NoV <= myvsample; NoV++) {
      Extrema_POnSurfParams& aParam = myGrid->Points[NoU][NoV];

      aParam.SetSqrDistance(thePoint.SquareDistance(aParam.Value()));
    }
  }

  // For search of minimum compute distances to mesh.
  if(myFlag == Extrema_ExtFlag_MIN || myFlag == Extrema_ExtFlag_MINMAX) 
  {
    // This is the tolerance of difference of squared values.
    // No need to set it too small.
    const Standard_Real aDiffTol = mytolu + mytolv;

    // Step 2. Compute distances to edges.
    // Assume UEdge(i, j) = { Point(i, j); Point(i + 1, j    ) }
    // Assume VEdge(i, j) = { Point(i, j); Point(i,     j + 1) }
    for ( NoU = 1 ; NoU <= myusample; NoU++ ) 
    {
      for ( NoV = 1 ; NoV <= myvsample; NoV++)
      {
        const Extrema_POnSurfParams& aParam0 = myGrid->Points[NoU][NoV];

        if (NoU < myusample)
        {
          // Compute parameters to UEdge.
          const Extrema_POnSurfParams& aParam1 = myGrid->Points[NoU + 1][NoV];
          ComputeEdgeParameters(myGrid->UEdgePntParams[NoU - 1][NoV - 1], Standard_True, aParam0, aParam1, thePoint,
                                Precision::SquareConfusion());
        }

        if (NoV < myvsample)
        {
          // Compute parameters to VEdge.
          const Extrema_POnSurfParams& aParam1 = myGrid->Points[NoU][NoV + 1];
          ComputeEdgeParameters(myGrid->VEdgePntParams[NoU - 1][NoV - 1], Standard_False, aParam0, aParam1, thePoint,
                                Precision::SquareConfusion());
        }
      }
    }

    // Step 3. Compute distances to faces.
    // Assume myFacePntParams(i, j) =
    // { Point(i, j); Point(i + 1, j); Point(i + 1, j + 1); Point(i, j + 1) }
    //   Or
    // { UEdge(i, j); VEdge(i + 1, j); UEdge(i, j + 1); VEdge(i, j) }
    Standard_Real aSqrDist01;
    Standard_Real aDiffDist;
    Standard_Boolean isOut;

    for ( NoU = 1 ; NoU < myusample; NoU++ ) {
      for ( NoV = 1 ; NoV < myvsample; NoV++) {
        const Extrema_POnSurfParams& aUE0 = myGrid->UEdgePntParams[NoU - 1][NoV - 1];
        const Extrema_POnSurfParams& aUE1 = myGrid->UEdgePntParams[NoU - 1][NoV];
        const Extrema_POnSurfParams& aVE0 = myGrid->VEdgePntParams[NoU - 1][NoV - 1];
        const Extrema_POnSurfParams& aVE1 = myGrid->VEdgePntParams[NoU][NoV - 1];

        aSqrDist01 = aUE0.Value().SquareDistance(aUE1.Value());
        aDiffDist = Abs(aUE0.GetSqrDistance() - aUE1.GetSqrDistance());
        isOut = Standard_False;

        if (aDiffDist >= aSqrDist01 - aDiffTol) {
          // The projection is outside the face.
          isOut = Standard_True;
        } else {
          aSqrDist01 = aVE0.Value().SquareDistance(aVE1.Value());
          aDiffDist = Abs(aVE0.GetSqrDistance() - aVE1.GetSqrDistance());

          if (aDiffDist >= aSqrDist01 - aDiffTol) {
            // The projection is outside the face.
            isOut = Standard_True;
          }
        }

        if (isOut) {
          // Get the closest point on an edge.
          const Extrema_POnSurfParams &aUEMin =
            aUE0.GetSqrDistance() < aUE1.GetSqrDistance() ? aUE0 : aUE1;
          const Extrema_POnSurfParams &aVEMin =
            aVE0.GetSqrDistance() < aVE1.GetSqrDistance() ? aVE0 : aVE1;
          const Extrema_POnSurfParams &aEMin =
            aUEMin.GetSqrDistance() < aVEMin.GetSqrDistance() ? aUEMin : aVEMin;

          myGrid->FacePntParams[NoU][NoV] = aEMin;
        } else {
          // Find closest point inside the face.
          Standard_Real aU[2];
          Standard_Real aV[2];
          Standard_Real aUPar;
          Standard_Real aVPar;

          // Compute U parameter.
          aUE0.Parameter(aU[0], aV[0]);
          aUE1.Parameter(aU[1], aV[1]);
          aUPar = 0.5*(aU[0] + aU[1]);
          // Compute V parameter.
          aVE0.Parameter(aU[0], aV[0]);
          aVE1.Parameter(aU[1], aV[1]);
          aVPar = 0.5*(aV[0] + aV[1]);

          Extrema_POnSurfParams& aParam = myGrid->FacePntParams[NoU][NoV];
          aParam.SetParameters(aUPar, aVPar, myS->Value(aUPar, aVPar));
          aParam.SetElementType(Extrema_Face);
          aParam.SetSqrDistance(thePoint.SquareDistance(aParam.Value()));
          aParam.SetIndices(NoU, NoV);
        }
      }
    }
  }
}

static Standard_Real LengthOfIso(const Adaptor3d_Surface& theS, const GeomAbs_IsoType theIso,
  const Standard_Real thePar1, const Standard_Real thePar2,
  const Standard_Integer theNbPnts,  const Standard_Real thePar)
{
  Standard_Real aLen = 0.;
  Standard_Integer i;
  Standard_Real dPar = (thePar2 - thePar1) / (theNbPnts - 1);
  gp_Pnt aP1, aP2;
  Standard_Real aPar = thePar1 + dPar;
  if(theIso == GeomAbs_IsoU)
  {
    aP1 = theS.Value(thePar, thePar1);
  }
  else
  {
    aP1 = theS.Value(thePar1, thePar);
  }

  for (i = 2; i <= theNbPnts; ++i)
  {
    if (theIso == GeomAbs_IsoU)
    {
      aP2 = theS.Value(thePar, aPar);
    }
    else
    {
      aP2 = theS.Value(aPar, thePar);
    }
    aLen += aP1.Distance(aP2);
    aP1 = aP2;
    aPar += dPar;
  }
  return aLen;
}
static void CorrectNbSamples(const Adaptor3d_Surface& theS, 
  const Standard_Real theU1, const Standard_Real theU2, Standard_Integer& theNbU, 
  const Standard_Real theV1, const Standard_Real theV2, Standard_Integer& theNbV)
{
  Standard_Real aMinLen = 1.e-3;
  Standard_Integer nbp = Min(23, theNbV);
  Standard_Real aLenU1 = LengthOfIso(theS, GeomAbs_IsoU, theV1, theV2, nbp, theU1);
  if (aLenU1 <= aMinLen)
  {
    Standard_Real aL = LengthOfIso(theS, GeomAbs_IsoU, theV1, theV2, nbp, .7*theU1 + 0.3*theU2);
    aLenU1 = Max(aL, aLenU1);
  }
  Standard_Real aLenU2 = LengthOfIso(theS, GeomAbs_IsoU, theV1, theV2, nbp, theU2);
  if (aLenU2 <= aMinLen)
  {
    Standard_Real aL = LengthOfIso(theS, GeomAbs_IsoU, theV1, theV2, nbp, .3*theU1 + 0.7*theU2);
    aLenU2 = Max(aL, aLenU2);
  }
  nbp = Min(23, theNbV);
  Standard_Real aLenV1 = LengthOfIso(theS, GeomAbs_IsoV, theU1, theU2, nbp, theV1);
  if (aLenV1 <= aMinLen)
  {
    Standard_Real aL = LengthOfIso(theS, GeomAbs_IsoV, theU1, theU2, nbp, .7*theV1 + 0.3*theV2);
    aLenV1 = Max(aL, aLenV1);
  }
  Standard_Real aLenV2 = LengthOfIso(theS, GeomAbs_IsoV, theU1, theU2, nbp, theV2);
  if (aLenV2 <= aMinLen)
  {
    Standard_Real aL = LengthOfIso(theS, GeomAbs_IsoV, theU1, theU2, nbp, .3*theV1 + 0.7*theV2);
    aLenV2 = Max(aL, aLenV2);
  }
  //
  Standard_Real aStepV1 = aLenU1 / theNbV;
  Standard_Real aStepV2 = aLenU2 / theNbV;
  Standard_Real aStepU1 = aLenV1 / theNbU;
  Standard_Real aStepU2 = aLenV2 / theNbU;

  Standard_Real aMaxStepV = Max(aStepV1, aStepV2);
  Standard_Real aMaxStepU = Max(aStepU1, aStepU2);
  //
  Standard_Real aRatio = aMaxStepV / aMaxStepU;
  if (aRatio > 10.)
  {
    Standard_Integer aMult = RealToInt(Log(aRatio) );
    if(aMult > 1)
      theNbV *= aMult;
  }
  else if (aRatio < 0.1)
  {
    Standard_Integer aMult = RealToInt( - Log(aRatio));
    if(aMult > 1)
      theNbV *= aMult;
  }

}
void Extrema_GenExtPS::BuildTree()
{
  // if tree already exists, assume it is already correctly filled
  if (!mySphereUBTree.IsNull())
    return;

  if (myS->GetType() == GeomAbs_BSplineSurface) {
    Handle(Geom_BSplineSurface) aBspl = myS->BSpline();
    Standard_Integer aUValue = aBspl->UDegree() * aBspl->NbUKnots();
    Standard_Integer aVValue = aBspl->VDegree() * aBspl->NbVKnots();
    // 300 is value, which is used for singular points (see Extrema_ExtPS.cxx::Initialize(...))
    if (aUValue > myusample)
      myusample = Min(aUValue, 300);
    if (aVValue > myvsample)
      myvsample = Min(aVValue, 300);
  }
  //
  CorrectNbSamples(*myS, myumin, myusup, myusample, myvmin, myvsup, myvsample);
  //
  gp_Pnt P1;

  //build grid of parametric points
  myGrid->BuildParamGrid (myGrid->UParams, myumin, myusup, myusample);
  myGrid->BuildParamGrid (myGrid->VParams, myvmin, myvsup, myvsample);

  // Calculation of distances
  mySphereUBTree = new Extrema_UBTreeOfSphere;
  Extrema_UBTreeFillerOfSphere aFiller(*mySphereUBTree);
  Standard_Integer i = 0;
  
  mySphereArray = new Bnd_HArray1OfSphere(0, myusample * myvsample);
 
  for (Standard_Integer NoU = 1; NoU <= myusample; ++NoU) {
    for (Standard_Integer NoV = 1; NoV <= myvsample; ++NoV) {
      P1 = myS->Value(myGrid->UParams[NoU - 1], myGrid->VParams[NoV - 1]);
      Bnd_Sphere aSph(P1.XYZ(), 0/*mytolu < mytolv ? mytolu : mytolv*/, NoU, NoV);
      aFiller.Add(i, aSph);
      mySphereArray->SetValue( i, aSph );
      i++;
    }
  }
  aFiller.Fill();
}

void Extrema_GenExtPS::FindSolution(const gp_Pnt& /*P*/, 
                                    const Extrema_POnSurfParams &theParams)
{
  math_Vector Tol(1,2);
  Tol(1) = mytolu;
  Tol(2) = mytolv;

  math_Vector UV(1, 2);
  theParams.Parameter(UV(1), UV(2));

  math_Vector UVinf(1,2), UVsup(1,2);
  UVinf(1) = myumin;
  UVinf(2) = myvmin;
  UVsup(1) = myusup;
  UVsup(2) = myvsup;

  math_FunctionSetRoot S(myF, Tol);
  S.Perform(myF, UV, UVinf, UVsup);

  myDone = Standard_True;
}

void Extrema_GenExtPS::SetFlag(const Extrema_ExtFlag F)
{
  myFlag = F;
}

void Extrema_GenExtPS::SetAlgo(const Extrema_ExtAlgo A)
{
  if(myAlgo != A)
     myInit = Standard_False;
  myAlgo = A;
 
}

void Extrema_GenExtPS::Perform(const gp_Pnt& P) 
{  
  myDone = Standard_False;
  myF.SetPoint(P);
  
  if(myAlgo == Extrema_ExtAlgo_Grad)
  {
    BuildGrid(P);
    Standard_Integer NoU,NoV;

    if(myFlag == Extrema_ExtFlag_MIN || myFlag == Extrema_ExtFlag_MINMAX) 
    {
      Extrema_ElementType anElemType;
      Standard_Integer iU;
      Standard_Integer iV;
      Standard_Integer iU2;
      Standard_Integer iV2;
      Standard_Boolean isMin;
      Standard_Integer i;

      for (NoU = 1; NoU < myusample; NoU++) {
        for (NoV = 1; NoV < myvsample; NoV++) {
          const Extrema_POnSurfParams& aParam = myGrid->FacePntParams[NoU][NoV];

          isMin = Standard_False;
          anElemType = aParam.GetElementType();

          if (anElemType == Extrema_Face) {
            isMin = Standard_True;
          } else {
            // Check if it is a boundary edge or corner vertex.
            aParam.GetIndices(iU, iV);

            if (anElemType == Extrema_UIsoEdge) {
              isMin = (iV == 1 || iV == myvsample);
            } else if (anElemType == Extrema_VIsoEdge) {
              isMin = (iU == 1 || iU == myusample);
            } else if (anElemType == Extrema_Node) {
              isMin = (iU == 1 || iU == myusample) &&
                      (iV == 1 || iV == myvsample);
            }

            if (!isMin) {
              // This is a middle element.
              if (anElemType == Extrema_UIsoEdge ||
                (anElemType == Extrema_Node && (iU == 1 || iU == myusample))) {
                // Check the down face.
                const Extrema_POnSurfParams& aDownParam = myGrid->FacePntParams[NoU][NoV - 1];

                if (aDownParam.GetElementType() == anElemType) {
                  aDownParam.GetIndices(iU2, iV2);
                  isMin = (iU == iU2 && iV == iV2);
                }
              } else if (anElemType == Extrema_VIsoEdge ||
                (anElemType == Extrema_Node && (iV == 1 || iV == myvsample))) {
                // Check the right face.
                const Extrema_POnSurfParams& aRightParam = myGrid->FacePntParams[NoU - 1][NoV];

                if (aRightParam.GetElementType() == anElemType) {
                  aRightParam.GetIndices(iU2, iV2);
                  isMin = (iU == iU2 && iV == iV2);
                }
              } else if (iU == NoU && iV == NoV) {
                // Check the lower-left node. For this purpose it is necessary
                // to check lower-left, lower and left faces.
                isMin = Standard_True;

                const Extrema_POnSurfParams *anOtherParam[3] =
                {
                  &myGrid->FacePntParams[NoU][NoV - 1],     // Down
                  &myGrid->FacePntParams[NoU - 1][NoV - 1], // Lower-left
                  &myGrid->FacePntParams[NoU - 1][NoV]      // Left
                };

                for (i = 0; i < 3 && isMin; i++) {
                  if (anOtherParam[i]->GetElementType() == Extrema_Node) {
                    anOtherParam[i]->GetIndices(iU2, iV2);
                    isMin = (iU == iU2 && iV == iV2);
                  } else {
                    isMin = Standard_False;
                  }
                }
              }
            }
          }

          if (isMin) {
            FindSolution(P, aParam);
          }
        }
      }
    }
    
    if(myFlag == Extrema_ExtFlag_MAX || myFlag == Extrema_ExtFlag_MINMAX)
    {
      Standard_Real Dist;

      for (NoU = 1; NoU <= myusample; NoU++)
      {
        for (NoV = 1; NoV <= myvsample; NoV++)
        {
          const Extrema_POnSurfParams& aParamMain = myGrid->Points[NoU][NoV];
          const Extrema_POnSurfParams& aParam1 = myGrid->Points[NoU - 1][NoV - 1];
          const Extrema_POnSurfParams& aParam2 = myGrid->Points[NoU - 1][NoV];
          const Extrema_POnSurfParams& aParam3 = myGrid->Points[NoU - 1][NoV + 1];
          const Extrema_POnSurfParams& aParam4 = myGrid->Points[NoU][NoV - 1];
          const Extrema_POnSurfParams& aParam5 = myGrid->Points[NoU][NoV + 1];
          const Extrema_POnSurfParams& aParam6 = myGrid->Points[NoU + 1][NoV - 1];
          const Extrema_POnSurfParams& aParam7 = myGrid->Points[NoU + 1][NoV];
          const Extrema_POnSurfParams& aParam8 = myGrid->Points[NoU + 1][NoV + 1];

          Dist = aParamMain.GetSqrDistance();

          if ((aParam1.GetSqrDistance() <= Dist) &&
              (aParam2.GetSqrDistance() <= Dist) &&
              (aParam3.GetSqrDistance() <= Dist) &&
              (aParam4.GetSqrDistance() <= Dist) &&
              (aParam5.GetSqrDistance() <= Dist) &&
              (aParam6.GetSqrDistance() <= Dist) &&
              (aParam7.GetSqrDistance() <= Dist) &&
              (aParam8.GetSqrDistance() <= Dist))
          {
            // Find maximum.
            FindSolution(P, myGrid->Points[NoU][NoV]);
          }
        }
      }
    }
  }
  else
  {
    BuildTree();
    if(myFlag == Extrema_ExtFlag_MIN || myFlag == Extrema_ExtFlag_MINMAX)
    {
      Bnd_Sphere aSol = mySphereArray->Value(0);
      Bnd_SphereUBTreeSelectorMin aSelector(mySphereArray, aSol);
      //aSelector.SetMaxDist( RealLast() );
      aSelector.DefineCheckPoint( P );
      mySphereUBTree->Select( aSelector );
      //TODO: check if no solution in binary tree
      Bnd_Sphere& aSph = aSelector.Sphere();
      Standard_Real aU = myGrid->UParams[aSph.U() - 1];
      Standard_Real aV = myGrid->VParams[aSph.V() - 1];
      Extrema_POnSurfParams aParams(aU, aV, myS->Value(aU, aV));

      aParams.SetSqrDistance(P.SquareDistance(aParams.Value()));
      aParams.SetIndices(aSph.U(), aSph.V());
      FindSolution(P, aParams);
    }
    if(myFlag == Extrema_ExtFlag_MAX || myFlag == Extrema_ExtFlag_MINMAX)
    {
      Bnd_Sphere aSol = mySphereArray->Value(0);
      Bnd_SphereUBTreeSelectorMax aSelector(mySphereArray, aSol);
      //aSelector.SetMaxDist( RealLast() );
      aSelector.DefineCheckPoint( P );
      mySphereUBTree->Select( aSelector );
      //TODO: check if no solution in binary tree
      Bnd_Sphere& aSph = aSelector.Sphere();
      Standard_Real aU = myGrid->UParams[aSph.U() - 1];
      Standard_Real aV = myGrid->VParams[aSph.V() - 1];
      Extrema_POnSurfParams aParams(aU, aV, myS->Value(aU, aV));
      aParams.SetSqrDistance(P.SquareDistance(aParams.Value()));
      aParams.SetIndices(aSph.U(), aSph.V());

      FindSolution(P, aParams);
    }
  }
}
//=============================================================================

Standard_Boolean Extrema_GenExtPS::IsDone () const { return myDone; }
//=============================================================================

Standard_Integer Extrema_GenExtPS::NbExt () const
{
  if (!IsDone()) { throw StdFail_NotDone(); }
  return myF.NbExt();
}
//=============================================================================

Standard_Real Extrema_GenExtPS::SquareDistance (const Standard_Integer N) const
{
  if ((N < 1) || (N > NbExt()))
  {
    throw Standard_OutOfRange();
  }

  return myF.SquareDistance(N);
}
//=============================================================================

const Extrema_POnSurf& Extrema_GenExtPS::Point (const Standard_Integer N) const
{
  if ((N < 1) || (N > NbExt()))
  {
    throw Standard_OutOfRange();
  }

  return myF.Point(N);
}
//=============================================================================
