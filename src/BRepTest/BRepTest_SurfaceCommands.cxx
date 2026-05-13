// Created on: 1993-07-22
// Created by: Remi LEQUETTE
// Copyright (c) 1993-1999 Matra Datavision
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

#include <stdio.h>
#include <BRepTest.hxx>
#include <GeometryTest.hxx>

#include <DrawTrSurf.hxx>
#include <DBRep.hxx>
#include <Draw_Interpretor.hxx>
#include <Draw_Appli.hxx>

#include <BRepLib.hxx>
#include <BRepTools_Quilt.hxx>
#include <BRepAdaptor_Curve.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>
#include <BRepBuilderAPI_MakeShell.hxx>
#include <BRepBuilderAPI_Sewing.hxx>
#include <BRepOffsetAPI_FindContigousEdges.hxx>
#include <TopExp.hxx>
#include <TopExp_Explorer.hxx>
#include <TopoDS.hxx>
#include <TCollection_AsciiString.hxx>
#include <Geom_Plane.hxx>
#include <Geom2d_TrimmedCurve.hxx>
#include <TopTools_SequenceOfShape.hxx>
#include <Precision.hxx>
#include <Draw_ProgressIndicator.hxx>
#include <BRepBuilderAPI_FastSewing.hxx>

#include <GeomAPI_ProjectPointOnSurf.hxx>
#include <Message.hxx>

//=======================================================================
// mkface
//=======================================================================

static Standard_Integer mkface(Draw_Interpretor& , Standard_Integer n, const char** a)
{
  if (n < 3) return 1;
  
  Handle(Geom_Surface) S = DrawTrSurf::GetSurface(a[2]);
  if (S.IsNull()) {
    Message::SendFail() << a[2] << " is not a surface";
    return 1;
  }
  
  Standard_Boolean mkface = a[0][2] == 'f';
  TopoDS_Shape res;

  Standard_Boolean Segment = Standard_False;
  if ( !mkface && (n == 4 || n == 8)) {
    Segment = !strcmp(a[n-1],"1");
    n--;
  }

  if (n == 3) {
    if (mkface)
      res = BRepBuilderAPI_MakeFace(S, Precision::Confusion());
    else
      res = BRepBuilderAPI_MakeShell(S,Segment);
  }
  else if (n <= 5) {
    if (!mkface) return 1;
    Standard_Boolean orient = (n  == 4);
    TopoDS_Shape W = DBRep::Get(a[3],TopAbs_WIRE);
    if (W.IsNull()) return 1;
    res = BRepBuilderAPI_MakeFace(S,TopoDS::Wire(W),orient);
  }
  else {
    if (mkface)
      res = BRepBuilderAPI_MakeFace(S,Draw::Atof(a[3]),Draw::Atof(a[4]),Draw::Atof(a[5]),Draw::Atof(a[6]),Precision::Confusion());
    else
      res = BRepBuilderAPI_MakeShell(S,Draw::Atof(a[3]),Draw::Atof(a[4]),Draw::Atof(a[5]),Draw::Atof(a[6]),
			      Segment);
  }
  
  DBRep::Set(a[1],res);
  return 0;
}

//=======================================================================
// quilt
//=======================================================================

static Standard_Integer quilt(Draw_Interpretor& , Standard_Integer n, const char** a)
{
  if (n < 4) return 1;
  BRepTools_Quilt Q;

  Standard_Integer i = 2;
  while (i < n) {
    TopoDS_Shape S = DBRep::Get(a[i]);
    if (!S.IsNull()) {
      if (S.ShapeType() == TopAbs_EDGE) {
	if (i+1 < n) {
	  TopoDS_Shape E = DBRep::Get(a[i+1]);
	  if (!E.IsNull()) {
	    if (E.ShapeType() == TopAbs_EDGE) {
	      i++;
	      Q.Bind(TopoDS::Edge(S),TopoDS::Edge(E));
	    }
	  }
	}
      }
      if (S.ShapeType() == TopAbs_VERTEX) {
	if (i+1 < n) {
	  TopoDS_Shape E = DBRep::Get(a[i+1]);
	  if (!E.IsNull()) {
	    if (E.ShapeType() == TopAbs_VERTEX) {
	      i++;
	      Q.Bind(TopoDS::Vertex(S),TopoDS::Vertex(E));
	    }
	  }
	}
      }
      else {
	Q.Add(S);
      }
    }
    i++;
  }

  DBRep::Set(a[1],Q.Shells());
  return 0;
}


//=======================================================================
// mksurface
//=======================================================================

static Standard_Integer mksurface(Draw_Interpretor& , Standard_Integer n, const char** a)
{
  if (n < 3) return 1;

  TopoDS_Shape S = DBRep::Get(a[2],TopAbs_FACE);
  if (S.IsNull()) return 1;
  TopLoc_Location L;
  Handle(Geom_Surface) C = BRep_Tool::Surface(TopoDS::Face(S),L);


  DrawTrSurf::Set(a[1],C->Transformed(L.Transformation()));
  return 0;
}

//=======================================================================
// mkplane
//=======================================================================

static Standard_Integer mkplane(Draw_Interpretor& theDI, Standard_Integer n, const char** a)
{
  if (n < 3) return 1;

  TopoDS_Shape S = DBRep::Get(a[2],TopAbs_WIRE);
  if (S.IsNull()) return 1;

  Standard_Boolean OnlyPlane = Standard_False;
  if ( n == 4) {
    OnlyPlane =  !strcmp(a[3],"1");
  }

  BRepBuilderAPI_MakeFace aMF(TopoDS::Wire(S), OnlyPlane);

  switch(aMF.Error())
  {
  case BRepBuilderAPI_FaceDone:
    DBRep::Set(a[1],aMF.Face());
    break;
  case BRepBuilderAPI_NoFace:
    theDI << "Error. mkplane has been finished with \"No Face\" status.\n";
    break;
  case BRepBuilderAPI_NotPlanar:
    theDI << "Error. mkplane has been finished with \"Not Planar\" status.\n";
    break;
  case BRepBuilderAPI_CurveProjectionFailed:
    theDI << "Error. mkplane has been finished with \"Fail in projection curve\" status.\n";
    break;
  case BRepBuilderAPI_ParametersOutOfRange:
    theDI << "Error. mkplane has been finished with \"Parameters are out of range\" status.\n";
    break;
  default:
    theDI << "Error. Undefined status. Please check the code.\n";
    break;
  }

  return 0;
}

//=======================================================================
// pcurve
//=======================================================================
Standard_IMPORT Draw_Color DrawTrSurf_CurveColor(const Draw_Color col);
Standard_IMPORT void DBRep_WriteColorOrientation ();
Standard_IMPORT Draw_Color DBRep_ColorOrientation (const TopAbs_Orientation Or);

//! Display a new 3D edge as curve on surface on XOY plane.
//! @param[in] theEdge original edge
//! @param[in] theFace face to find parametric curve
//! @param[in] theName result drawable name
//! @param[in] theNewFace a face to create new edge on
static bool showPCurveForEdge3d(const TopoDS_Edge& theEdge,
                                const TopoDS_Face& theFace,
                                const TCollection_AsciiString& theName,
                                const TopoDS_Face& theNewFace)
{
  Standard_Real anEdgeRange[2] = {};
  const Handle(Geom2d_Curve) aPCurve = BRep_Tool::CurveOnSurface(theEdge, theFace, anEdgeRange[0], anEdgeRange[1]);
  if (aPCurve.IsNull())
  {
    const Handle(Poly_Polygon2D) aPol = BRep_Tool::PolygonOnSurface(theEdge, theFace);
    if (aPol.IsNull())
      return false;

    TopoDS_Edge aNewEdge;
    BRep_Builder().MakeEdge(aNewEdge);
    BRep_Builder().UpdateEdge(aNewEdge, aPol, theNewFace);
    BRep_Builder().UpdateEdge(aNewEdge, BRep_Tool::Tolerance(theEdge));
    BRep_Builder().Range(aNewEdge, theNewFace, anEdgeRange[0], anEdgeRange[1]);
    aNewEdge.Orientation(theEdge.Orientation());

    DBRep::Set(theName.ToCString(), aNewEdge);
    return true;
  }

  TopoDS_Edge aNewEdge;
  BRep_Builder().MakeEdge(aNewEdge);
  BRep_Builder().UpdateEdge(aNewEdge, aPCurve, theNewFace, BRep_Tool::Tolerance(theEdge));
  BRep_Builder().Range(aNewEdge, theNewFace, anEdgeRange[0], anEdgeRange[1]);
  aNewEdge.Orientation(theEdge.Orientation());

  DBRep::Set(theName.ToCString(), aNewEdge);
  return true;
}

//! Display a 2d curve from the given edge and face.
//! @param[in] theEdge original edge
//! @param[in] theFace face to find parametric curve
//! @param[in] theName result drawable name
static bool showPCurveForEdge2d(const TopoDS_Edge& theEdge,
                                const TopoDS_Face& theFace,
                                const TCollection_AsciiString& theName)
{
  Standard_Real anEdgeRange[2] = {};
  Handle(Geom2d_Curve) aPCurve = BRep_Tool::CurveOnSurface(theEdge, theFace, anEdgeRange[0], anEdgeRange[1]);
  if (aPCurve.IsNull())
  {
    const Handle(Poly_Polygon2D) aPol = BRep_Tool::PolygonOnSurface(theEdge, theFace);
    if (aPol.IsNull())
      return false;

    const Draw_Color aColorBack = DrawTrSurf_CurveColor(DBRep_ColorOrientation(theEdge.Orientation()));
    DrawTrSurf::Set(theName.ToCString(), aPol);
    DrawTrSurf_CurveColor(aColorBack);
    return true;
  }

  Standard_Real    aCFirst    = aPCurve->FirstParameter();
  Standard_Real    aCLast     = aPCurve->LastParameter();
  Standard_Boolean isPeriodic = aPCurve->IsPeriodic();
  if (Handle(Geom2d_TrimmedCurve) aTrimCurve = Handle(Geom2d_TrimmedCurve)::DownCast(aPCurve))
  {
    const Handle(Geom2d_Curve) aBCurve = aTrimCurve->BasisCurve();
    isPeriodic = aBCurve->IsPeriodic();
    aCFirst = aBCurve->FirstParameter();
    aCLast  = aBCurve->LastParameter();
  }

  if (isPeriodic || ((aCFirst - anEdgeRange[0] <= Precision::PConfusion())
                    && anEdgeRange[1] - aCLast <= Precision::PConfusion()))
  {
    aPCurve = new Geom2d_TrimmedCurve(aPCurve, anEdgeRange[0], anEdgeRange[1]);
  }

  const Draw_Color aColorBack = DrawTrSurf_CurveColor(DBRep_ColorOrientation(theEdge.Orientation()));
  DrawTrSurf::Set(theName.ToCString(), aPCurve);
  DrawTrSurf_CurveColor(aColorBack);
  return true;
}

static Standard_Integer pcurve(Draw_Interpretor& theDI,
                               Standard_Integer theNbArgs,
                               const char** theArgVec)
{
  TopoDS_Edge anEdge;
  TopoDS_Face aFace;
  TopoDS_Face aNewFace;
  Standard_CString aName = nullptr;
  bool toPrintColorLegend = true;
  for (Standard_Integer anArgIter = 1; anArgIter < theNbArgs; ++anArgIter)
  {
    TCollection_AsciiString anArgCase(theArgVec[anArgIter]);
    anArgCase.LowerCase();
    if (anArgCase == "-mute")
    {
      toPrintColorLegend = false;
    }
    else if (anArgCase == "-3d")
    {
      Handle(Geom_Plane) aSurf = new Geom_Plane(gp_Pln(gp::XOY()));
      BRep_Builder().MakeFace(aNewFace, aSurf, Precision::Confusion());
    }
    else if (aName == nullptr
          && anEdge.IsNull()
          && anArgIter + 1 < theNbArgs
          && !DBRep::Get(theArgVec[anArgIter + 1], TopAbs_EDGE).IsNull())
    {
      aName  = theArgVec[anArgIter];
      anEdge = TopoDS::Edge(DBRep::Get(theArgVec[++anArgIter]));
    }
    else if (aFace.IsNull()
          && !DBRep::Get(theArgVec[anArgIter], TopAbs_FACE).IsNull())
    {
      if (aName == nullptr)
        aName = theArgVec[anArgIter];

      aFace = TopoDS::Face(DBRep::Get(theArgVec[anArgIter]));
    }
    else
    {
      theDI << "Syntax error at '" << theArgVec[anArgIter] << "'\n";
      return 1;
    }
  }
  if (aFace.IsNull())
  {
    theDI << "Syntax error: wrong number of arguments";
    return 1;
  }

  if (!anEdge.IsNull())
  {
    const bool isDone = !aNewFace.IsNull()
                      ? showPCurveForEdge3d(anEdge, aFace, aName, aNewFace)
                      : showPCurveForEdge2d(anEdge, aFace, aName);
    if (!isDone)
      theDI << "Error: Edge " << aName << " does not have pcurve";

    return isDone ? 0 : 1;
  }

  if (toPrintColorLegend)
    DBRep_WriteColorOrientation();

  // pcurves of a face
  aFace.Orientation(TopAbs_FORWARD);
  TopExp_Explorer anEdgeIter(aFace, TopAbs_EDGE);
  int aNbDone = 0;
  for (Standard_Integer anEdgeIndex = 1; anEdgeIter.More(); anEdgeIter.Next(), ++anEdgeIndex)
  {
    TCollection_AsciiString anEdgeName = TCollection_AsciiString(aName) + "_" + anEdgeIndex;
    const bool isDone = !aNewFace.IsNull()
                      ? showPCurveForEdge3d(TopoDS::Edge(*anEdgeIter), aFace, anEdgeName, aNewFace)
                      : showPCurveForEdge2d(TopoDS::Edge(*anEdgeIter), aFace, anEdgeName);
    if (isDone)
    {
      ++aNbDone;
      theDI << anEdgeName << " ";
    }
    else
    {
      theDI << "Error: Edge " << anEdgeName << " does not have pcurve";
    }
  }
  return aNbDone != 0 ? 0 : 1;
}

//=======================================================================
// sewing
//=======================================================================

static Standard_Integer sewing (Draw_Interpretor& theDi, 
				Standard_Integer theArgc, const char** theArgv)
{
  BRepBuilderAPI_Sewing aSewing;
  Standard_Integer aPar = 1;
  TopTools_SequenceOfShape aSeq;

  Standard_Real aTol = 1.0e-06;
  Standard_Boolean aSewingMode = Standard_True;
  Standard_Boolean anAnalysisMode = Standard_True;
  Standard_Boolean aCuttingMode = Standard_True;
  Standard_Boolean aNonManifoldMode = Standard_False;
  Standard_Boolean aSameParameterMode = Standard_True;
  Standard_Boolean aFloatingEdgesMode = Standard_False;
  Standard_Boolean aFaceMode = Standard_True;
  Standard_Boolean aSetMinTol = Standard_False;
  Standard_Real aMinTol = 0.;
  Standard_Real aMaxTol = Precision::Infinite();

  for (Standard_Integer i = 2; i < theArgc; i++)
  {
    if (theArgv[i][0] == '-' || theArgv[i][0] == '+')
    {
      Standard_Boolean aVal = (theArgv[i][0] == '+' ? Standard_True : Standard_False);
      switch (tolower(theArgv[i][1]))
      {
      case 'm':
        {
          if (tolower(theArgv[i][2]) == 'i' && i+1 < theArgc)
          {
            if (Draw::Atof (theArgv[i+1]))
            {
              aMinTol = Draw::Atof (theArgv[++i]);
              aSetMinTol = Standard_True;
            }
            else
            {
              theDi << "Error! min tolerance can't possess the null value\n";
              return (1);
            }
          }
          if (tolower(theArgv[i][2]) == 'a' && i+1 < theArgc)
          {
            if (Draw::Atof (theArgv[i+1]))
              aMaxTol = Draw::Atof (theArgv[++i]);
            else
            {
              theDi << "Error! max tolerance can't possess the null value\n";
              return (1);
            }
          }
        }
        break;
      case 's': aSewingMode = aVal; break;
      case 'a': anAnalysisMode = aVal; break;
      case 'c': aCuttingMode = aVal; break;
      case 'n': aNonManifoldMode = aVal; break;
      case 'p': aSameParameterMode = aVal; break;
      case 'e': aFloatingEdgesMode = aVal; break;
      case 'f': aFaceMode = aVal; break;
      }
    }
    else
    {
      TopoDS_Shape aShape = DBRep::Get (theArgv[i]);
      if (!aShape.IsNull())
      {
        aSeq.Append (aShape);
        aPar++;
      }
      else
      {
        if (Draw::Atof (theArgv[i]))
          aTol = Draw::Atof (theArgv[i]);
      }
    }
  }
   
  if (aPar < 2)
  {
    theDi << "Use: " << theArgv[0] << " result [tolerance] shape1 shape2 ... [min tolerance] [max tolerance] [switches]\n";
    theDi << "To set user's value of min/max tolerances the following syntax is used: +<parameter> <value>\n";
    theDi << "- parameters are identified by letters:\n";
    theDi << "  mint - min tolerance\n";
    theDi << "  maxt - max tolerance\n";
    theDi << "Switches allow to tune other parameters of Sewing\n";
    theDi << "The following syntax is used: <symbol><parameter>\n";
    theDi << "- symbol may be - to set parameter off, + to set on\n";
    theDi << "- parameters are identified by letters:\n";
    theDi << "  s - mode for creating sewed shape\n";
    theDi << "  a - mode for analysis of input shapes\n";
    theDi << "  c - mode for cutting of free edges\n";
    theDi << "  n - mode for non manifold processing\n";
    theDi << "  p - mode for same parameter processing for edges\n";
    theDi << "  e - mode for sewing floating edges\n";
    theDi << "  f - mode for sewing faces\n";
    return (1);
  }
    
  if (!aSetMinTol)
    aMinTol = aTol*1e-4;
  if (aTol < Precision::Confusion())
    aTol = Precision::Confusion();
  if (aMinTol < Precision::Confusion())
    aMinTol = Precision::Confusion();
  if (aMinTol > aTol)
  {
    theDi << "Error! min tolerance can't exceed working tolerance\n";
    return (1);
  }
  if (aMaxTol < aTol)
  {
    theDi << "Error! max tolerance can't be less than working tolerance\n";
    return (1);
  }

  aSewing.Init (aTol, aSewingMode, anAnalysisMode, aCuttingMode, aNonManifoldMode);
  aSewing.SetSameParameterMode (aSameParameterMode);
  aSewing.SetFloatingEdgesMode (aFloatingEdgesMode);
  aSewing.SetFaceMode (aFaceMode);
  aSewing.SetMinTolerance (aMinTol);
  aSewing.SetMaxTolerance (aMaxTol);

  for (Standard_Integer i = 1; i <= aSeq.Length(); i++)
    aSewing.Add(aSeq.Value(i));
  
  Handle(Draw_ProgressIndicator) aProgress = new Draw_ProgressIndicator (theDi, 1);
  aSewing.Perform (aProgress->Start());
  aSewing.Dump();

  const TopoDS_Shape& aRes = aSewing.SewedShape();
  if (!aRes.IsNull())
    DBRep::Set(theArgv[1], aRes);
  return 0;
}

//=======================================================================
//function : fastsewing
//purpose  : 
//=======================================================================
Standard_Integer fastsewing (Draw_Interpretor& theDI, 
                            Standard_Integer theNArg, 
                            const char** theArgVal)
{
  if(theNArg < 3)
  {
    //                0         1       2     3         4
    theDI << "Use: fastsewing result [-tol <value>] <list_of_faces>\n";
    return 1;
  }

  BRepBuilderAPI_FastSewing aFS;

  Standard_Integer aStartIndex = 2;

  if(!strcmp(theArgVal[aStartIndex], "-tol"))
  {
    aFS.SetTolerance(Draw::Atof (theArgVal[aStartIndex+1]));
    aStartIndex = 4;
  }

  for(Standard_Integer i = aStartIndex; i < theNArg; i++)
  {
    TopoDS_Shape aS = DBRep::Get(theArgVal[i]);
    
    if(!aFS.Add(aS))
    {
      theDI << "Face is not added. See statuses.\n";
    }
  }

  BRepBuilderAPI_FastSewing::FS_VARStatuses aStatus = aFS.GetStatuses();

  if(aStatus)
  {
    theDI << "Error: There are some problems while adding (" <<
                        (static_cast<Standard_Integer>(aStatus)) << ")\n";
    aFS.GetStatuses(&std::cout);
  }

  aFS.Perform();

  aStatus = aFS.GetStatuses();

  if(aStatus)
  {
    theDI << "Error: There are some problems while performing (" <<
                        (static_cast<Standard_Integer>(aStatus)) << ")\n";
    aFS.GetStatuses(&std::cout);
  }

  DBRep::Set(theArgVal[1], aFS.GetResult());

  return 0;
}

//=======================================================================
// continuity
//=======================================================================

static Standard_Integer continuity (Draw_Interpretor& , 
				    Standard_Integer n, const char** a)
{
  if (n < 2) return (1);

  BRepOffsetAPI_FindContigousEdges aFind;

  TopoDS_Shape sh = DBRep::Get(a[1]);
  Standard_Integer i=1;
  if (sh.IsNull()) {
    if (n < 3) return (1);
    Standard_Real tol = Draw::Atof(a[1]);
    aFind.Init(tol, Standard_False);
    i = 2;
  }
  
  while (i < n) {
    sh = DBRep::Get(a[i]);
    aFind.Add(sh);
    i++;
  }

  aFind.Perform();
  aFind.Dump();

  return 0;
}

//=======================================================================
// encoderegularity
//=======================================================================
static Standard_Integer encoderegularity (Draw_Interpretor& , 
					  Standard_Integer n, const char** a)

{
  if (n < 2) return 1;
  TopoDS_Shape sh = DBRep::Get(a[1]);
  if (sh.IsNull()) return 1;
  if (n==2) 
    BRepLib::EncodeRegularity(sh);
  else {
    Standard_Real Tol = Draw::Atof(a[2]);
    Tol *= M_PI/180.;
    BRepLib::EncodeRegularity(sh, Tol);
  }
  return 0;
}

static Standard_Integer getedgeregul
  (Draw_Interpretor& di, Standard_Integer argc, const char** argv)
{
  if( argc < 3)
  {
    Message::SendFail() << "Invalid number of arguments. Should be: checkedgeregularity edge face1 [face2]";
    return 1;
  }
  
  TopoDS_Shape anEdge =  DBRep::Get(argv[1],TopAbs_EDGE);
  TopoDS_Shape aFace1 = DBRep::Get(argv[2],TopAbs_FACE);
  TopoDS_Shape aFace2 = (argc > 3  ? DBRep::Get(argv[3],TopAbs_FACE) : aFace1);
  if( anEdge.IsNull() || aFace1.IsNull() || aFace2.IsNull())
  {
    Message::SendFail() << "Invalid number of arguments. Should be: getedgeregularity edge face1 [face2]";
    return 1;
  }
 
  GeomAbs_Shape aRegularity = BRep_Tool::Continuity(TopoDS::Edge(anEdge), TopoDS::Face(aFace1),  TopoDS::Face(aFace2));
  TCollection_AsciiString aStrReg("Regularity of edge : ");
  switch( aRegularity)
  {
    default:
    case GeomAbs_C0 : aStrReg += "C0"; break;
    case GeomAbs_G1 : aStrReg += "G1"; break;
    case GeomAbs_C1 : aStrReg += "C1"; break;
    case GeomAbs_G2 : aStrReg += "G2"; break;
    case GeomAbs_C2 : aStrReg += "C2"; break;
    case GeomAbs_C3 : aStrReg += "C3"; break;
    case GeomAbs_CN : aStrReg += "CN"; break;
  };

  di<<aStrReg.ToCString()<<"\n";
  return 0; // Done
}

//=======================================================================
//function : projponf
//purpose  : 
//=======================================================================
static Standard_Integer projponf(Draw_Interpretor& di, Standard_Integer n, const char** a)
{
  if (n < 3 || n > 5) {
    di << "Project point on the face.\n";
    di << "Usage: projponf face pnt [extrema flag: -min/-max/-minmax] [extrema algo: -g(grad)/-t(tree)]\n";
    return 1;
  }
  // get face
  TopoDS_Shape aS = DBRep::Get(a[1]);
  if (aS.IsNull()) {
    di << "the face is a null shape\n";
    return 0;
  }
  //
  if (aS.ShapeType() != TopAbs_FACE) {
    di << "not a face\n";
    return 0;
  }
  //
  const TopoDS_Face& aFace = *(TopoDS_Face*)&aS;
  //
  // get point
  gp_Pnt aP;
  DrawTrSurf::GetPoint(a[2], aP);
  //
  // get projection options
  // default values;
  Extrema_ExtAlgo anExtAlgo = Extrema_ExtAlgo_Grad;
  Extrema_ExtFlag anExtFlag = Extrema_ExtFlag_MINMAX;
  //
  for (Standard_Integer i = 3; i < n; ++i) {
    if (!strcasecmp(a[i], "-min")) {
      anExtFlag = Extrema_ExtFlag_MIN;
    }
    else if (!strcasecmp(a[i], "-max")) {
      anExtFlag = Extrema_ExtFlag_MAX;
    }
    else if (!strcasecmp(a[i], "-minmax")) {
      anExtFlag = Extrema_ExtFlag_MINMAX;
    }
    else if (!strcasecmp(a[i], "-t")) {
      anExtAlgo = Extrema_ExtAlgo_Tree;
    }
    else if (!strcasecmp(a[i], "-g")) {
      anExtAlgo = Extrema_ExtAlgo_Grad;
    }
  }
  //
  // get surface
  TopLoc_Location aLoc;
  const Handle(Geom_Surface)& aSurf = BRep_Tool::Surface(aFace, aLoc);
  // move point to surface location
  aP.Transform(aLoc.Transformation().Inverted());
  //
  // get bounds of the surface
  Standard_Real aUMin, aUMax, aVMin, aVMax;
  aSurf->Bounds(aUMin, aUMax, aVMin, aVMax);
  //
  // initialize projector
  GeomAPI_ProjectPointOnSurf aProjPS;
  aProjPS.Init(aSurf, aUMin, aUMax, aVMin, aVMax);
  // set the options
  aProjPS.SetExtremaAlgo(anExtAlgo);
  aProjPS.SetExtremaFlag(anExtFlag);
  // perform projection
  aProjPS.Perform(aP);
  //
  if (aProjPS.NbPoints()) {
    // lower distance
    Standard_Real aDist = aProjPS.LowerDistance();
    // lower distance parameters
    Standard_Real U, V;
    aProjPS.LowerDistanceParameters(U, V);
    // nearest point
    gp_Pnt aPProj = aProjPS.NearestPoint();
    // translate projection point to face location
    aPProj.Transform(aLoc.Transformation());
    //
    // print the projection values
    di << "proj dist = " << aDist << "\n";
    di << "uvproj = " << U << " " << V << "\n";
    di << "pproj = " << aPProj.X() << " " << aPProj.Y() << " " << aPProj.Z() << "\n";
  }
  else {
    if (!aProjPS.IsDone()) {
      di << "projection failed\n";
    }
    else {
      di << "no projection found\n";
    }
  }
  return 0;
}

//=======================================================================
//function : SurfaceCommands
//purpose  : 
//=======================================================================

void  BRepTest::SurfaceCommands(Draw_Interpretor& theCommands)
{
  static Standard_Boolean done = Standard_False;
  if (done) return;
  done = Standard_True;

  DBRep::BasicCommands(theCommands);
  GeometryTest::SurfaceCommands(theCommands);

  const char* g = "Surface topology commands";

  theCommands.Add("mkface",
		  "mkface facename surfacename [ufirst ulast vfirst vlast] [wire [norient]]",
		  __FILE__,mkface,g);

  theCommands.Add("mkshell",
		  "mkshell shellname surfacename [ufirst ulast vfirst vlast] [segment 0/1]",
		  __FILE__,mkface,g);

  theCommands.Add("quilt",
		  "quilt compoundname shape1 edgeshape2  edgeshape1... shape2  edgeshape3 edgeshape1or2 ... shape3 ...",
		  __FILE__,quilt,g);
  
  theCommands.Add("mksurface",
		  "mksurface surfacename facename",
		  __FILE__,mksurface,g);

  theCommands.Add("mkplane",
		  "mkplane facename wirename [OnlyPlane 0/1]",
		  __FILE__,mkplane,g);

  theCommands.Add("pcurve", R"(
pcurve [name edgename] facename [-mute] [-3d]
Display 2D parametric curve for specified edge and face in 2D viewer.
When edge is not specified, all edges will be displayed for specified face,
and output variables will be generated from face name.
 -mute suppress output of color legend;
 -3d   display a new 3D edge as curve on surface on XOY plane.
)", __FILE__, pcurve, g);

  theCommands.Add("sewing",
		  "sewing result [tolerance] shape1 shape2 ... [min tolerance] [max tolerance] [switches]",
		  __FILE__,sewing, g);

  theCommands.Add("continuity", 
		  "continuity [tolerance] shape1 shape2 ...",
		  __FILE__,continuity, g);

  theCommands.Add("encoderegularity", 
		  "encoderegularity shape [tolerance (in degree)]",
		  __FILE__,encoderegularity, g);

  theCommands.Add ("fastsewing", "fastsewing result [-tol <value>] <list_of_faces>", 
                                                __FILE__, fastsewing, g);
  theCommands.Add ("getedgeregularity", "getedgeregularity edge face1 [face2]",  __FILE__,getedgeregul,g);

  theCommands.Add ("projponf",
                   "projponf face pnt [extrema flag: -min/-max/-minmax] [extrema algo: -g(grad)/-t(tree)]\n"
                   "\t\tProject point on the face.",
                   __FILE__, projponf, g);
}

