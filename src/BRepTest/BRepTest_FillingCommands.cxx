// Created on: 1996-07-10
// Created by: Joelle CHAUVET
// Copyright (c) 1996-1999 Matra Datavision
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

// Modified:	Wed Mar  5 09:45:42 1997
//    by:	Joelle CHAUVET
//              G1134 : new command "approxplate"
// Modified:	Thu Jun 12 16:51:36 1997
//    by:	Jerome LEMONIER
//              Mise a jour suite a la modification des methodes Curves2d 
//		et Sense GeomPlate_BuildPlateSurface.
// Modified:	Mon Nov  3 10:24:07 1997
//		utilisation de BRepFill_CurveConstraint



#include <GeometryTest.hxx>
#include <Draw_Interpretor.hxx>
#include <Draw_Appli.hxx>
#include <Draw_ProgressIndicator.hxx>
#include <DrawTrSurf.hxx>
#include <BRepAdaptor_Surface.hxx>
#include <BRepAdaptor_Curve2d.hxx>
#include <BRepTest.hxx>
#include <DBRep.hxx>
#include <TColStd_HArray1OfInteger.hxx>

#include <BRepLib.hxx>
#include <BRep_Builder.hxx>
#include <GeomPlate_Surface.hxx>

#include <GeomPlate_MakeApprox.hxx>
#include <GeomPlate_PlateG0Criterion.hxx>
#include <GeomPlate_PlateG1Criterion.hxx>
#include <BRepFill_CurveConstraint.hxx>
#include <Geom_Surface.hxx>

#include <TopoDS_Wire.hxx>
#include <BRepBuilderAPI_MakeEdge.hxx>
#include <BRepBuilderAPI_MakeWire.hxx>
#include <BRepBuilderAPI_MakeFace.hxx>

#include <AdvApp2Var_ApproxAFunc2Var.hxx>

#include <Geom_BSplineSurface.hxx>

#include <TColgp_SequenceOfXY.hxx>
#include <TColgp_SequenceOfXYZ.hxx>

#include <BRepAdaptor_Curve.hxx>

#include <BRepOffsetAPI_MakeFilling.hxx>
#include <TCollection_AsciiString.hxx>

#include <BRepTest_Objects.hxx>

#include <stdio.h>
#include <gp_Pnt.hxx>

// pour mes tests
#ifdef OCCT_DEBUG
#include <OSD_Chronometer.hxx>
#include <Geom_Line.hxx>
#endif

const Standard_Integer defDegree = 3;
const Standard_Integer defNbPtsOnCur = 10;
const Standard_Integer defNbIter = 3;
const Standard_Boolean defAnisotropie = Standard_False;
const Standard_Real defTol2d = 0.00001;
const Standard_Real defTol3d = 0.0001;
const Standard_Real defTolAng = 0.01;
const Standard_Real defTolCurv = 0.1;
const Standard_Integer defMaxDeg = 8;
const Standard_Integer defMaxSegments = 9;

Standard_Integer Degree = defDegree;
Standard_Integer NbPtsOnCur = defNbPtsOnCur ;
Standard_Integer NbIter = defNbIter;
Standard_Boolean Anisotropie = defAnisotropie ;
Standard_Real Tol2d = defTol2d;
Standard_Real Tol3d = defTol3d;
Standard_Real TolAng = defTolAng;
Standard_Real TolCurv = defTolCurv;
Standard_Integer MaxDeg = defMaxDeg;
Standard_Integer MaxSegments = defMaxSegments;

//////////////////////////////////////////////////////////////////////////////// 
//  commande plate : resultat face sur surface plate
//////////////////////////////////////////////////////////////////////////////// 

static Standard_Integer plate (Draw_Interpretor & di,Standard_Integer n,const char** a)
{
  if (n < 8 ) return 1;
  Standard_Integer NbCurFront=Draw::Atoi(a[3]);
  Handle(GeomPlate_HArray1OfHCurve) Fronts = new GeomPlate_HArray1OfHCurve(1,NbCurFront);
  Handle(TColStd_HArray1OfInteger) Tang = new TColStd_HArray1OfInteger(1,NbCurFront);
  Handle(TColStd_HArray1OfInteger) NbPtsCur = new TColStd_HArray1OfInteger(1,NbCurFront);
  BRep_Builder B;
  
  GeomPlate_BuildPlateSurface Henri(3,15,2);

  Standard_Integer i;
  for (i=1; i<=NbCurFront ; i++) { 
    TopoDS_Shape aLocalEdge(DBRep::Get(a[3*i+1],TopAbs_EDGE));
    TopoDS_Edge E = TopoDS::Edge(aLocalEdge);
//    TopoDS_Edge E = TopoDS::Edge(DBRep::Get(a[3*i+1],TopAbs_EDGE));
    if(E.IsNull()) return 1;
    TopoDS_Shape aLocalFace(DBRep::Get(a[3*i+2],TopAbs_FACE));
    TopoDS_Face F = TopoDS::Face(aLocalFace);
//    TopoDS_Face F = TopoDS::Face(DBRep::Get(a[3*i+2],TopAbs_FACE));
    if(F.IsNull()) return 1;
    Standard_Integer T = Draw::Atoi(a[3*i+3]);
    Tang->SetValue(i,T);
    NbPtsCur->SetValue(i,Draw::Atoi(a[2]));
    Handle(BRepAdaptor_Surface) S = new BRepAdaptor_Surface();
    S->Initialize(F);
    Handle(BRepAdaptor_Curve2d) C = new BRepAdaptor_Curve2d();
    C->Initialize(E,F);
    Adaptor3d_CurveOnSurface ConS(C,S);
    Handle (Adaptor3d_CurveOnSurface) HConS = new Adaptor3d_CurveOnSurface(ConS);
    Fronts->SetValue(i,HConS);
    Handle(GeomPlate_CurveConstraint) Cont
      = new BRepFill_CurveConstraint(HConS,
				     Tang->Value(i),
				     NbPtsCur->Value(i));
    Henri.Add(Cont);
  }
  Handle(Draw_ProgressIndicator) aProgress = new Draw_ProgressIndicator(di, 1);
  Henri.Perform(aProgress->Start());
  if (aProgress->UserBreak())
  {
    di << "Error: UserBreak\n";
    return 0;
  }

  Standard_Real ErrG0 = 1.1*Henri.G0Error();
  //std::cout<<" dist. max = "<<Henri.G0Error()<<" ; angle max = "<<Henri.G1Error()<<std::endl;
  di<<" dist. max = "<<Henri.G0Error()<<" ; angle max = "<<Henri.G1Error()<<"\n";

  BRepBuilderAPI_MakeWire MW;
  for (i=1 ; i<=NbCurFront ; i++) {
    Standard_Integer iInOrder=Henri.Order()->Value(i);
    TopoDS_Edge E;
    if (Henri.Sense()->Value(iInOrder)==1) { 
      BRepBuilderAPI_MakeEdge ME(Henri.Curves2d()->Value(iInOrder),
                          Henri.Surface(),
                          Fronts->Value(iInOrder)->LastParameter(),
                          Fronts->Value(iInOrder)->FirstParameter());
      E = ME.Edge();
      
    }
    else {
      BRepBuilderAPI_MakeEdge ME(Henri.Curves2d()->Value(iInOrder),
                          Henri.Surface(),
                          Fronts->Value(iInOrder)->FirstParameter(),
                          Fronts->Value(iInOrder)->LastParameter());
      E = ME.Edge();
    }
    B.UpdateVertex(TopExp::FirstVertex(E), ErrG0);
    B.UpdateVertex(TopExp::LastVertex(E), ErrG0);
    BRepLib::BuildCurve3d(E);
    char name[100];
    Snprintf(name,"Edge_%d", i);
    DBRep::Set(name, E);
    MW.Add(E);
    if (MW.IsDone()==Standard_False) {
      throw Standard_Failure("mkWire is over ");
    }
      
  }
  TopoDS_Wire W;
  W=MW.Wire();
  if (!(W.Closed())) throw Standard_Failure("Wire is not closed");
  BRepBuilderAPI_MakeFace MF(Henri.Surface(),W,Standard_True);
  DBRep::Set(a[1],MF.Face());
  return 0;
}

//////////////////////////////////////////////////////////////////////////////// 
//  commande gplate : resultat face egale a la surface approchee
////////////////////////////////////////////////////////////////////////////////
 
static Standard_Integer gplate(Draw_Interpretor& theDI,
                               Standard_Integer theNbArgs,
                               const char** theArgVec)
{
  if (theNbArgs < 6)
  {
    theDI << "Syntax error: wrong number of arguments";
    return 1;
  }

  GeomPlate_BuildPlateSurface Henri(3, 15, 2);

  Standard_Integer anArgIter = 2;

  Standard_Integer aNbCurFront = 0;
  if (!Draw::ParseInteger(theArgVec[anArgIter++], aNbCurFront))
  {
    theDI << "Syntax error at '" << theArgVec[anArgIter - 1] << "'";
    return 1;
  }

  Standard_Integer aNbPointConstraint = 0;
  if (!Draw::ParseInteger(theArgVec[anArgIter++], aNbPointConstraint))
  {
    theDI << "Syntax error at '" << theArgVec[anArgIter - 1] <<"'";
    return 1;
  }

  // optional initial surface
  const TopoDS_Shape anInitFace = DBRep::GetExisting(theArgVec[anArgIter]);
  if (!anInitFace.IsNull() && anInitFace.ShapeType() == TopAbs_FACE)
  {
    ++anArgIter;
    const Handle(BRepAdaptor_Surface) aSurfAdaptor = new BRepAdaptor_Surface(TopoDS::Face(anInitFace));
    Henri.LoadInitSurface(BRep_Tool::Surface(aSurfAdaptor->Face()));
  }

  const auto parseContinuity = [](const char* theStr, Standard_Integer& theCont) -> bool
  {
    const char* aStr = theStr;
    if (LowerCase(*aStr) == 'g')
      ++aStr;

    return Draw::ParseInteger(aStr, theCont) && theCont >= -1 && theCont <= 2;
  };

  for (Standard_Integer anEdgeIter = 1; anEdgeIter <= aNbCurFront; ++anEdgeIter)
  {
    const TopoDS_Shape anEdge = DBRep::GetExisting(theArgVec[anArgIter++]);
    if (anEdge.IsNull() || anEdge.ShapeType() != TopAbs_EDGE)
    {
      theDI << "Syntax error: '" << theArgVec[anArgIter - 1] << "' is not an Edge";
      return 1;
    }

    Standard_Integer aConti = 0;
    if (!parseContinuity(theArgVec[anArgIter++], aConti))
    {
      theDI << "Syntax error at '" << theArgVec[anArgIter - 1] << "'";
      return 1;
    }

    if ((aConti == 0) || (aConti == -1))
    {
      const Handle(BRepAdaptor_Curve)         aCurvAdaptor = new BRepAdaptor_Curve(TopoDS::Edge(anEdge));
      const Handle(GeomPlate_CurveConstraint) aConstr      = new BRepFill_CurveConstraint(aCurvAdaptor, aConti);
      Henri.Add(aConstr);
      continue;
    }

    const TopoDS_Shape aFace = DBRep::GetExisting(theArgVec[anArgIter++]);
    if (aFace.IsNull() || aFace.ShapeType() != TopAbs_FACE)
    {
      theDI << "Syntax error: '" << theArgVec[anArgIter - 1] << "' is not a Face";
      return 1;
    }

    const Handle(BRepAdaptor_Surface)       aSurfAdaptor    = new BRepAdaptor_Surface(TopoDS::Face(aFace));
    const Handle(BRepAdaptor_Curve2d)       aCurve2dAdaptor = new BRepAdaptor_Curve2d(TopoDS::Edge(anEdge), TopoDS::Face(aFace));
    const Handle(Adaptor3d_CurveOnSurface)  aCurveOnSurf    = new Adaptor3d_CurveOnSurface(aCurve2dAdaptor, aSurfAdaptor);
    const Handle(GeomPlate_CurveConstraint) aConstr         = new BRepFill_CurveConstraint(aCurveOnSurf, aConti);
    Henri.Add(aConstr);
  }

  for (Standard_Integer aPntIter = 1; aPntIter <= aNbPointConstraint; ++aPntIter)
  {
    gp_Pnt aPnt;
    if (DrawTrSurf::GetPoint(theArgVec[anArgIter], aPnt))
    {
      const Handle(GeomPlate_PointConstraint) PCont = new GeomPlate_PointConstraint(aPnt, 0);
      Henri.Add(PCont);
      ++anArgIter;
      continue;
    }

    Standard_Real aUV[2] = {};
    if (!Draw::ParseReal(theArgVec[anArgIter + 0], aUV[0])
     || !Draw::ParseReal(theArgVec[anArgIter + 1], aUV[1]))
    {
      theDI << "Syntax error at '" << theArgVec[anArgIter] << "'";
      return 1;
    }
    anArgIter += 2;

    Standard_Integer aConti = 0;
    if (!parseContinuity(theArgVec[anArgIter++], aConti))
    {
      theDI << "Syntax error at '" << theArgVec[anArgIter - 1] << "'";
      return 1;
    }

    const TopoDS_Shape aFace = DBRep::GetExisting(theArgVec[anArgIter++]);
    if (aFace.IsNull() || aFace.ShapeType() != TopAbs_FACE)
    {
      theDI << "Syntax error: '" << theArgVec[anArgIter - 1] << "' is not a Face";
      return 1;
    }

    const Handle(BRepAdaptor_Surface) aSurfAdaptor = new BRepAdaptor_Surface(TopoDS::Face(aFace));
    const Handle(GeomPlate_PointConstraint) aPntConstr =
      new GeomPlate_PointConstraint(aUV[0], aUV[1], BRep_Tool::Surface(aSurfAdaptor->Face()), aConti, 0.001, 0.001, 0.001);
    Henri.Add(aPntConstr);
  }

  const char* aGErrVars[3] = {};
  for (; anArgIter < theNbArgs; ++anArgIter)
  {
    TCollection_AsciiString anArg(theArgVec[anArgIter]);
    anArg.LowerCase();
    if ((anArgIter + 1 < theNbArgs) && (anArg == "-g0error" || anArg == "-g1error" || anArg == "-g2error"))
    {
      aGErrVars[int(anArg.Value(3)) -'0'] = theArgVec[++anArgIter];
    }
    else
    {
      theDI << "Syntax error at '" << theArgVec[anArgIter] << "'";
      return 1;
    }
  }

  Handle(Draw_ProgressIndicator) aProgress = new Draw_ProgressIndicator(theDI, 1);
  Henri.Perform(aProgress->Start());
  if (aProgress->UserBreak())
  {
    theDI << "Error: UserBreak\n";
    return 0;
  }

  if (aGErrVars[0] != nullptr)
    Draw::Set(aGErrVars[0], Henri.G0Error());

  if (aGErrVars[1] != nullptr)
    Draw::Set(aGErrVars[1], Henri.G1Error());

  if (aGErrVars[2] != nullptr)
    Draw::Set(aGErrVars[2], Henri.G2Error());

  theDI << " dist. max = " << Henri.G0Error() << " ; angle max = " << Henri.G1Error() << " ; diffcurv max = " << Henri.G2Error() << "\n";

  const Standard_Integer nbcarreau = 9;
  const Standard_Integer degmax = 8;

  TColgp_SequenceOfXY aS2d;
  TColgp_SequenceOfXYZ aS3d;
  Henri.Disc2dContour(4, aS2d);
  Henri.Disc3dContour(4, 0, aS3d);

  const Standard_Real seuil = Max(0.0001, 10 * Henri.G0Error());
  const GeomPlate_PlateG0Criterion critere(aS2d, aS3d, seuil);

  const Handle(GeomPlate_Surface) aGPlate = Henri.Surface();
  const GeomPlate_MakeApprox anApproxMaker(aGPlate, critere, 0.0001, nbcarreau, degmax);
  const Handle(Geom_Surface) aSurf = anApproxMaker.Surface();

  Standard_Real Umin = 0.0, Umax = 0.0, Vmin = 0.0, Vmax = 0.0;
  aGPlate->Bounds(Umin, Umax, Vmin, Vmax);

  BRepBuilderAPI_MakeFace MF(aSurf, Umin, Umax, Vmin, Vmax, Precision::Confusion());

  DBRep::Set(theArgVec[1], MF.Face());
  return 0;
}

//////////////////////////////////////////////////////////////////////////////// 
//  commande approxplate : resultat face sur surface approchee
//////////////////////////////////////////////////////////////////////////////// 

static Standard_Integer approxplate (Draw_Interpretor & di,Standard_Integer n,const char** a)
{
  if (n < 9 ) return 1;
  Standard_Integer NbMedium=Draw::Atoi(a[2]);
  Standard_Integer NbCurFront=Draw::Atoi(a[3]);
  Handle(GeomPlate_HArray1OfHCurve) Fronts = new GeomPlate_HArray1OfHCurve(1,NbCurFront);
  Handle(TColStd_HArray1OfInteger) Tang = new TColStd_HArray1OfInteger(1,NbCurFront);
  Handle(TColStd_HArray1OfInteger) NbPtsCur = new TColStd_HArray1OfInteger(1,NbCurFront);
  
  GeomPlate_BuildPlateSurface Henri(3,15,2);

  Standard_Integer i;
  for (i=1; i<=NbCurFront ; i++) { 
    TopoDS_Shape aLocalShape(DBRep::Get(a[3*i+1],TopAbs_EDGE));
    TopoDS_Edge E = TopoDS::Edge(aLocalShape);
//    TopoDS_Edge E = TopoDS::Edge(DBRep::Get(a[3*i+1],TopAbs_EDGE));
    if(E.IsNull()) return 1;
    TopoDS_Shape aLocalFace(DBRep::Get(a[3*i+2],TopAbs_FACE));
    TopoDS_Face F = TopoDS::Face(aLocalFace);
//    TopoDS_Face F = TopoDS::Face(DBRep::Get(a[3*i+2],TopAbs_FACE));
    if(F.IsNull()) return 1;
    Standard_Integer T = Draw::Atoi(a[3*i+3]);
    Tang->SetValue(i,T);
    NbPtsCur->SetValue(i,NbMedium);
    Handle(BRepAdaptor_Surface) S = new BRepAdaptor_Surface();
    S->Initialize(F);
    Handle(BRepAdaptor_Curve2d) C = new BRepAdaptor_Curve2d();
    C->Initialize(E,F);
    Adaptor3d_CurveOnSurface ConS(C,S);
    Handle (Adaptor3d_CurveOnSurface) HConS = new Adaptor3d_CurveOnSurface(ConS);
    Fronts->SetValue(i,HConS);
    Handle(GeomPlate_CurveConstraint) Cont
      = new BRepFill_CurveConstraint(HConS,
				     Tang->Value(i),
				     NbPtsCur->Value(i));
    Henri.Add(Cont);
  }
  
  Handle(Draw_ProgressIndicator) aProgress = new Draw_ProgressIndicator(di, 1);
  Henri.Perform(aProgress->Start());
  if (aProgress->UserBreak())
  {
    di << "Error: UserBreak\n";
    return 0;
  }

  Standard_Real dmax = Henri.G0Error(),
                anmax = Henri.G1Error();
  //std::cout<<" dist. max = "<<dmax<<" ; angle max = "<<anmax<<std::endl;
  di<<" dist. max = "<<dmax<<" ; angle max = "<<anmax<<"\n";

  Tol3d = Draw::Atof(a[3*NbCurFront+4]);
  Standard_Integer Nbmax = Draw::Atoi(a[3*NbCurFront+5]);
  Standard_Integer degmax = Draw::Atoi(a[3*NbCurFront+6]);
  Standard_Integer CritOrder = Draw::Atoi(a[3*NbCurFront+7]);
  Handle(GeomPlate_Surface) surf = Henri.Surface();
  Handle(Geom_BSplineSurface) support;

  if (CritOrder==-1) {
    GeomPlate_MakeApprox MApp(surf,Tol3d,Nbmax,degmax,dmax,-1);
    support = MApp.Surface();
  }
  else if (CritOrder>=0) {
    TColgp_SequenceOfXY S2d;
    TColgp_SequenceOfXYZ S3d;
    S2d.Clear();
    S3d.Clear();
    Standard_Real seuil;
    if (CritOrder==0) {
      Henri.Disc2dContour(4,S2d);
      Henri.Disc3dContour(4,0,S3d);
      seuil = Max(Tol3d,dmax*10);
      GeomPlate_PlateG0Criterion Crit0(S2d,S3d,seuil);
      GeomPlate_MakeApprox MApp(surf,Crit0,Tol3d,Nbmax,degmax);
      support = MApp.Surface();
    }
    else if (CritOrder==1) {
      Henri.Disc2dContour(4,S2d);
      Henri.Disc3dContour(4,1,S3d);
      seuil = Max(Tol3d,anmax*10);
      GeomPlate_PlateG1Criterion Crit1(S2d,S3d,seuil);
      GeomPlate_MakeApprox MApp(surf,Crit1,Tol3d,Nbmax,degmax);
      support = MApp.Surface();
    }
  }

  BRepBuilderAPI_MakeWire MW;
  BRep_Builder B;
  for (i=1 ; i<=NbCurFront ; i++) {
    Standard_Integer iInOrder=Henri.Order()->Value(i);
    TopoDS_Edge E;
    if (Henri.Sense()->Value(iInOrder)==1) { 
      BRepBuilderAPI_MakeEdge ME(Henri.Curves2d()->Value(iInOrder),
                          support,
                          Fronts->Value(iInOrder)->LastParameter(),
                          Fronts->Value(iInOrder)->FirstParameter());
      E = ME.Edge();
    }
    else {
      BRepBuilderAPI_MakeEdge ME(Henri.Curves2d()->Value(iInOrder),
                          support,
                          Fronts->Value(iInOrder)->FirstParameter(),
                          Fronts->Value(iInOrder)->LastParameter());
      E = ME.Edge();
    }
    B.UpdateVertex(TopExp::FirstVertex(E), dmax);
    B.UpdateVertex(TopExp::LastVertex(E), dmax);
    BRepLib::BuildCurve3d(E);
    MW.Add(E);
    if (MW.IsDone()==Standard_False) {
      throw Standard_Failure("mkWire is over ");
    }
  }
  TopoDS_Wire W;
  W=MW.Wire();
  if (!(W.Closed())) throw Standard_Failure("Wire is not closed");
  BRepBuilderAPI_MakeFace MF(support,W,Standard_True);
  DBRep::Set(a[1],MF.Face());

  return 0;
}

static Standard_Integer filling( Draw_Interpretor & di, Standard_Integer n, const char** a )
{
#ifdef OCCT_DEBUG
  // Chronometrage
  OSD_Chronometer Chrono;
  Chrono.Reset();
  Chrono.Start();
#endif

  if (n < 7)
  {
    di.PrintHelp(a[0]);
    return 1;
  }
  
  Standard_Integer NbBounds = Draw::Atoi( a[2] );
  Standard_Integer NbConstraints = Draw::Atoi( a[3] );
  Standard_Integer NbPoints = Draw::Atoi( a[4] );

  BRepOffsetAPI_MakeFilling MakeFilling( Degree,
				   NbPtsOnCur,
				   NbIter,
				   Anisotropie,
				   Tol2d,
				   Tol3d,
				   TolAng,
				   TolCurv,
				   MaxDeg,
				   MaxSegments );
  TopoDS_Face InitFace = TopoDS::Face( DBRep::Get(a[5], TopAbs_FACE) );
  if (! InitFace.IsNull())
    MakeFilling.LoadInitSurface( InitFace );
  
  Standard_Integer i = (InitFace.IsNull())? 5 : 6, k;
  TopoDS_Edge E;
  TopoDS_Face F;
  gp_Pnt Point;
  Standard_Integer Order;
  TopTools_ListOfShape ListForHistory;
  for (k = 1; k <= NbBounds; k++)
  { 
    E.Nullify();
    F.Nullify();
    E = TopoDS::Edge( DBRep::Get(a[i], TopAbs_EDGE) );
    if (! E.IsNull())
      i++;
    F = TopoDS::Face( DBRep::Get(a[i], TopAbs_FACE) );
    if (! F.IsNull())
      i++;
    
    Order = Draw::Atoi( a[i++] );
    
    if (! E.IsNull() && ! F.IsNull())
      MakeFilling.Add( E, F, (GeomAbs_Shape)Order );
    else if (E.IsNull())
    {
      if (F.IsNull())
      {
        di<<"\nWrong parameters\n\n";
        return 1;
      }
      else
        MakeFilling.Add( F, (GeomAbs_Shape)Order );
    }
    else
      MakeFilling.Add( E, (GeomAbs_Shape)Order );

    //History 
    if (!E.IsNull())
      ListForHistory.Append(E);
  }
  for (k = 1; k <= NbConstraints; k++)
  { 
    E.Nullify();
    F.Nullify();
    E = TopoDS::Edge( DBRep::Get(a[i++], TopAbs_EDGE) );
    if (E.IsNull())
    {
      di<<"Wrong parameters\n";
      return 1;
    }
    F = TopoDS::Face( DBRep::Get(a[i], TopAbs_FACE) );
    if (! F.IsNull())
      i++;
    
    Order = Draw::Atoi( a[i++] );
    
    if (F.IsNull())
      MakeFilling.Add( E, (GeomAbs_Shape)Order, Standard_False );
    else
      MakeFilling.Add( E, F, (GeomAbs_Shape)Order, Standard_False );
  }
  for (k = 1; k <= NbPoints; k++)
  {
    if (DrawTrSurf::GetPoint( a[i], Point )) 
    {
      MakeFilling.Add( Point );
      i++;
    }
    else
    {
      Standard_Real U = Draw::Atof( a[i++] ), V = Draw::Atof( a[i++] );
      F = TopoDS::Face( DBRep::Get(a[i++], TopAbs_FACE));
      if (F.IsNull()) 
      {
        di<<"Wrong parameters\n";
        return 1;
      }
      Order = Draw::Atoi( a[i++] );
      
      MakeFilling.Add( U, V, F, (GeomAbs_Shape)Order );
    }
  }
  
  MakeFilling.Build();
  if (! MakeFilling.IsDone())
  {
    di<<"filling failed\n";
    return 0;
  }
  
  Standard_Real dmax = MakeFilling.G0Error(),
    angmax = MakeFilling.G1Error(),
    curvmax = MakeFilling.G2Error();
  di<<" dist. max = "<<dmax<<" ; angle max = "<<angmax<<" ; diffcurv max = "<<curvmax<<"\n";
  
  TopoDS_Face ResFace= TopoDS::Face( MakeFilling.Shape() );
  DBRep::Set( a[1], ResFace );

#ifdef OCCT_DEBUG
  Chrono.Stop();
  Standard_Real Tps;
  Chrono.Show(Tps);
  di<<"*** FIN DE FILLING ***\n";
  di<<"Temps de calcul  : "<<Tps<<"\n";
#endif

  //History 
  if (BRepTest_Objects::IsHistoryNeeded())
    BRepTest_Objects::SetHistory(ListForHistory, MakeFilling);

  return 0;
}

static Standard_Integer fillingparam( Draw_Interpretor & di, Standard_Integer n, const char** a )
{
  if ( n == 1) {

    //std::cout << "fillingparam : options are"  <<std::endl;
    //std::cout << "-l : to list current values" << std::endl;
    //std::cout << "-i : to set default values"   << std::endl;
    //std::cout << "-r deg nbPonC nbIt anis : to set filling options" <<std::endl;
    //std::cout << "-c t2d t3d tang tcur : to set tolerances" << std::endl;
    //std::cout << "-a maxdeg maxseg : Approximation option" << std::endl;
    di << "fillingparam : options are"  <<"\n";
    di << "-l : to list current values\n";
    di << "-i : to set default values"   << "\n";
    di << "-r deg nbPonC nbIt anis : to set filling options\n";
    di << "-c t2d t3d tang tcur : to set tolerances\n";
    di << "-a maxdeg maxseg : Approximation option\n";
  }
  else if (n > 1)
    {
      TCollection_AsciiString AS( a[1] );
      AS.LowerCase();
      const char* flag = AS.ToCString();
      if (strcmp( flag, "-l" ) == 0 && n == 2)
	{
	  //std::cout<<std::endl;
	  //std::cout<<"Degree = "<<Degree<<std::endl;
	  //std::cout<<"NbPtsOnCur = "<<NbPtsOnCur<<std::endl;
	  //std::cout<<"NbIter = "<<NbIter<<std::endl;
	  //std::cout<<"Anisotropie = "<<Anisotropie<<std::endl<<std::endl;
	  //
	  //std::cout<<"Tol2d = "<<Tol2d<<std::endl;
	  //std::cout<<"Tol3d = "<<Tol3d<<std::endl;
	  //std::cout<<"TolAng = "<<TolAng<<std::endl;
	  //std::cout<<"TolCurv = "<<TolCurv<<std::endl<<std::endl;
	  //
	  //std::cout<<"MaxDeg = "<<MaxDeg<<std::endl;
	  //std::cout<<"MaxSegments = "<<MaxSegments<<std::endl<<std::endl;
	  di<<"\n";
	  di<<"Degree = "<<Degree<<"\n";
	  di<<"NbPtsOnCur = "<<NbPtsOnCur<<"\n";
	  di<<"NbIter = "<<NbIter<<"\n";
	  di<<"Anisotropie = "<< (Standard_Integer) Anisotropie<<"\n\n";
	  
	  di<<"Tol2d = "<<Tol2d<<"\n";
	  di<<"Tol3d = "<<Tol3d<<"\n";
	  di<<"TolAng = "<<TolAng<<"\n";
	  di<<"TolCurv = "<<TolCurv<<"\n\n";
	  
	  di<<"MaxDeg = "<<MaxDeg<<"\n";
	  di<<"MaxSegments = "<<MaxSegments<<"\n\n";
	}
      else if (strcmp( flag, "-i" ) == 0 && n == 2)
	{
	  Degree = defDegree;
	  NbPtsOnCur = defNbPtsOnCur;
	  NbIter = defNbIter;
	  Anisotropie = defAnisotropie;
	  
	  Tol2d = defTol2d;
	  Tol3d = defTol3d;
	  TolAng = defTolAng;
	  TolCurv = defTolCurv;
	  
	  MaxDeg = defMaxDeg;
	  MaxSegments = defMaxSegments;
	}
      else if (strcmp( flag, "-r" ) == 0 && n == 6)
	{
	  Degree      = Draw::Atoi( a[2] );
	  NbPtsOnCur  = Draw::Atoi( a[3] );
	  NbIter      = Draw::Atoi( a[4] );
	  Anisotropie = Draw::Atoi( a[5] ) != 0;
	}
      else if (strcmp( flag, "-c" ) == 0 && n == 6)
	{
	  Tol2d   = Draw::Atof( a[2] ); 
	  Tol3d   = Draw::Atof( a[3] );
	  TolAng  = Draw::Atof( a[4] );
	  TolCurv = Draw::Atof( a[5] );
	}
      else if (strcmp( flag, "-a" ) == 0 && n == 4)
	{
	  MaxDeg      = Draw::Atoi( a[2] );
	  MaxSegments = Draw::Atoi( a[3] );
	}
      else
	{
	  //std::cout<<"Wrong parameters"<<std::endl;
	  di<<"Wrong parameters\n";
	  return 1;
	}
    }
  return 0;
}




void  BRepTest::FillingCommands(Draw_Interpretor& theCommands)
{
  static Standard_Boolean done = Standard_False;
  if (done) return;
  done = Standard_True;

  DBRep::BasicCommands(theCommands);
  GeometryTest::SurfaceCommands(theCommands);

  const char* g = "Surface filling topology commands";
  theCommands.Add("plate",
		  "plate result nbrpntoncurve nbrcurfront edge face tang (0:vif;1:tang) ...",
		  __FILE__,
		  plate,
		  g) ;

  theCommands.Add("gplate", R"(
gplate result nbCurveConstraints nbPntConstraints [faceInit]
       [[edge {-1,G0}] [edge       {G1,G2} surf] ...]
       [[point       ] [u v  {-1,G0,G1,G2} surf] ...]
       [-g0error varName] [-g1error varName] [-g2error varName]
Computes surface usingGeomPlate_BuildPlateSurface algorithm.
 result              output surface (Face)
 nbCurveConstraints  number of curve constraints
 nbPntConstraints    number of point constraints
 faceInit            (optional) initial face to take UV-bounded surface from
 'edge {-1,G0}'      3d curve (Edge) constraint
 'edge {G1,G2} surf' 2d curve (Edge) constraint on a given surface (Face)
 'point'             3d point constraint (DRAW point)
 'u v GN surf'       2d point constraint on a given surface (Face)
 {-1,G0,G1,G2}       constraint order (-1 means to skip deviation limits)
 -g0error            variable name to put maximum G0Error
 -g1error            variable name to put maximum G1Error
 -g2error            variable name to put maximum G2Error
)", __FILE__, gplate, g);

  theCommands.Add("approxplate",
		  "approxplate result nbrpntoncurve nbrcurfront edge face tang (0:vif;1:tang) ... tol nmax degmax crit",
		  __FILE__,
		  approxplate,
		  g) ;



  theCommands.Add("filling",
		  "filling result nbB nbC nbP [SurfInit] [edge][face]order... edge[face]order... point/u v face order...",
		  __FILE__,
		  filling,
		  g) ;

  theCommands.Add("fillingparam",
		  "fillingparam : no arg give help",
		  __FILE__,
		  fillingparam,
		  g) ;

}
