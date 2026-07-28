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

#ifndef _Extrema_GenExtPS_HeaderFile
#define _Extrema_GenExtPS_HeaderFile

#include <Bnd_HArray1OfSphere.hxx>
#include <NCollection_StdAllocator.hxx>
#include <Extrema_Array2OfPOnSurfParams.hxx>
#include <Extrema_POnSurfParams.hxx>
#include <Extrema_HUBTreeOfSphere.hxx>
#include <Extrema_FuncPSNorm.hxx>
#include <Extrema_ExtFlag.hxx>
#include <Extrema_ExtAlgo.hxx>
#include <TColStd_HArray1OfReal.hxx>

#include <memory>
#include <vector>

class Adaptor3d_Surface;

//! It calculates all the extremum distances
//! between a point and a surface.
//! These distances can be minimum or maximum.
class Extrema_GenExtPS 
{
public:

  DEFINE_STANDARD_ALLOC

  //! Empty constructor.
  Standard_EXPORT Extrema_GenExtPS();

  //! Destructor.
  Standard_EXPORT ~Extrema_GenExtPS();

  //! It calculates all the distances.
  //! The function F(u,v)=distance(P,S(u,v)) has an
  //! extremum when gradient(F)=0. The algorithm searches
  //! all the zeros inside the definition ranges of the
  //! surface.
  //! NbU and NbV are used to locate the close points
  //! to find the zeros. They must be great enough
  //! such that if there is N extrema, there will
  //! be N extrema between P and the grid.
  //! TolU et TolV are used to determine the conditions
  //! to stop the iterations; at the iteration number n:
  //! (Un - Un-1) < TolU and (Vn - Vn-1) < TolV .
  Standard_EXPORT Extrema_GenExtPS(const gp_Pnt& P, const Adaptor3d_Surface& S, const Standard_Integer NbU, const Standard_Integer NbV, const Standard_Real TolU, const Standard_Real TolV, const Extrema_ExtFlag F = Extrema_ExtFlag_MINMAX, const Extrema_ExtAlgo A = Extrema_ExtAlgo_Grad);
  
  //! It calculates all the distances.
  //! The function F(u,v)=distance(P,S(u,v)) has an
  //! extremum when gradient(F)=0. The algorithm searches
  //! all the zeros inside the definition ranges of the
  //! surface.
  //! NbU and NbV are used to locate the close points
  //! to find the zeros. They must be great enough
  //! such that if there is N extrema, there will
  //! be N extrema between P and the grid.
  //! TolU et TolV are used to determine the conditions
  //! to stop the iterations; at the iteration number n:
  //! (Un - Un-1) < TolU and (Vn - Vn-1) < TolV .
  Standard_EXPORT Extrema_GenExtPS(const gp_Pnt& P, const Adaptor3d_Surface& S, const Standard_Integer NbU, const Standard_Integer NbV, const Standard_Real Umin, const Standard_Real Usup, const Standard_Real Vmin, const Standard_Real Vsup, const Standard_Real TolU, const Standard_Real TolV, const Extrema_ExtFlag F = Extrema_ExtFlag_MINMAX, const Extrema_ExtAlgo A = Extrema_ExtAlgo_Grad);
  
  Standard_EXPORT void Initialize (const Adaptor3d_Surface& S, const Standard_Integer NbU, const Standard_Integer NbV, const Standard_Real TolU, const Standard_Real TolV);
  
  Standard_EXPORT void Initialize (const Adaptor3d_Surface& S, const Standard_Integer NbU, const Standard_Integer NbV, const Standard_Real Umin, const Standard_Real Usup, const Standard_Real Vmin, const Standard_Real Vsup, const Standard_Real TolU, const Standard_Real TolV);
  
  //! the algorithm is done with the point P.
  //! An exception is raised if the fields have not
  //! been initialized.
  Standard_EXPORT void Perform (const gp_Pnt& P);
  
  Standard_EXPORT void SetFlag (const Extrema_ExtFlag F);
  
  Standard_EXPORT void SetAlgo (const Extrema_ExtAlgo A);
  
  //! Returns True if the distances are found.
  Standard_EXPORT Standard_Boolean IsDone() const;
  
  //! Returns the number of extremum distances.
  Standard_EXPORT Standard_Integer NbExt() const;
  
  //! Returns the value of the Nth resulting square distance.
  Standard_EXPORT Standard_Real SquareDistance (const Standard_Integer N) const;
  
  //! Returns the point of the Nth resulting distance.
  Standard_EXPORT const Extrema_POnSurf& Point (const Standard_Integer N) const;

private:
  
  Standard_EXPORT void BuildTree();
  
  Standard_EXPORT void FindSolution (const gp_Pnt& P, const Extrema_POnSurfParams& theParams);
  
  //! Selection of points to build grid, depending on the type of surface
  Standard_EXPORT void GetGridPoints (const Adaptor3d_Surface& theSurf);
  
  //! Creation of grid of parametric points
  Standard_EXPORT void BuildGrid (const gp_Pnt& thePoint);
  
  //! Compute new edge parameters.
  Standard_EXPORT void ComputeEdgeParameters (Extrema_POnSurfParams& theOutParam,
                                              const Standard_Boolean IsUEdge,
                                              const Extrema_POnSurfParams& theParam0,
                                              const Extrema_POnSurfParams& theParam1,
                                              const gp_Pnt& thePoints,
                                              const Standard_Real theDiffTol);

private:

  // disallow copies
  Extrema_GenExtPS            (const Extrema_GenExtPS& ) Standard_DELETE;
  Extrema_GenExtPS& operator= (const Extrema_GenExtPS& ) Standard_DELETE;

private:

  typedef std::vector<Standard_Real, NCollection_StdAllocator<Standard_Real>> VectorOfParams;
  typedef std::vector<Extrema_POnSurfParams, NCollection_StdAllocator<Extrema_POnSurfParams>> VectorOfSurfParams;
  typedef std::vector<VectorOfSurfParams, NCollection_StdAllocator<VectorOfSurfParams>> VectorOfVectorOfSurfParams;

  struct PointGrid
  {
    VectorOfParams UParams;
    VectorOfParams VParams;

    // avoid using NCollection_Array2 to avoid memory allocation issues on Windows
    VectorOfVectorOfSurfParams Points;
    VectorOfVectorOfSurfParams FacePntParams;
    VectorOfVectorOfSurfParams UEdgePntParams;
    VectorOfVectorOfSurfParams VEdgePntParams;

    //! Resize vector of vectors.
    static void Resize (VectorOfVectorOfSurfParams& theVec,
                        Standard_Integer theNbRows, Standard_Integer theNbCols)
    {
      theVec.resize(theNbRows);
      for (VectorOfSurfParams& aRow : theVec)
        aRow.resize(theNbCols);
    }

    //! Build grid of parametric points.
    static void BuildParamGrid (VectorOfParams& theParams,
                                Standard_Real theMin,
                                Standard_Real theSup,
                                Standard_Integer theNbSamples)
    {
      Standard_Real PasU = theSup - theMin;
      Standard_Real U0   = PasU / theNbSamples / 100.;
      PasU = (PasU - U0) / (theNbSamples - 1);
      U0 = U0/2. + theMin;
      theParams.resize(theNbSamples);
      Standard_Real U = U0;
      for (Standard_Integer aSampleIter = 0; aSampleIter < theNbSamples; ++aSampleIter, U += PasU)
        theParams[aSampleIter] = U;
    }
  };

private:

  Standard_Boolean myDone = Standard_False;
  Standard_Boolean myInit = Standard_False;
  Standard_Real myumin = 0.0;
  Standard_Real myusup = 0.0;
  Standard_Real myvmin = 0.0;
  Standard_Real myvsup = 0.0;
  Standard_Integer myusample = 0;
  Standard_Integer myvsample = 0;
  Standard_Real mytolu = 0.0;
  Standard_Real mytolv = 0.0;

  Extrema_HUBTreeOfSphere mySphereUBTree;
  Handle(Bnd_HArray1OfSphere) mySphereArray;
  Extrema_FuncPSNorm myF;
  const Adaptor3d_Surface* myS = nullptr;
  Extrema_ExtFlag myFlag = Extrema_ExtFlag_MINMAX;
  Extrema_ExtAlgo myAlgo = Extrema_ExtAlgo_Grad;

  std::unique_ptr<PointGrid> myGrid;
};

#endif // _Extrema_GenExtPS_HeaderFile
