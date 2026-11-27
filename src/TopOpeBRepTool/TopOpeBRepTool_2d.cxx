// Created on: 1998-03-23
// Created by: Jean Yves LEBEY
// Copyright (c) 1998-1999 Matra Datavision
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

#include <TopOpeBRepTool_2d.hxx>

#include <gp_Vec2d.hxx>
#include <TopoDS.hxx>
#include <TopExp.hxx>
#include <BRep_Tool.hxx>
#include <Geom_Curve.hxx>
#include <ProjLib_ProjectedCurve.hxx>
#include <Geom_Surface.hxx>
#include <TopOpeBRepTool_CurveTool.hxx>
#include <TopOpeBRepTool_DataMapOfShapeListOfC2DF.hxx>
#include <TopOpeBRepTool_C2DF.hxx>
#include <TopOpeBRepTool_ListOfC2DF.hxx>
#include <TopOpeBRepTool_tol.hxx>
#include <TopOpeBRepTool_EXPORT.hxx>
#include <TopOpeBRepTool_TOOL.hxx>

#ifdef DRAW
#include <TopOpeBRepTool_DRAW.hxx>
#include <Geom2d_Curve.hxx>
#endif

#ifdef OCCT_DEBUG
void debc2dnull(void) {}
Standard_EXPORT Standard_Boolean TopOpeBRepTool_GettraceC2D();
#endif

Standard_EXPORT Handle(Geom2d_Curve) MakePCurve(const ProjLib_ProjectedCurve& PC);

//=======================================================================
//function : TopOpeBRepTool_2d
//=======================================================================
TopOpeBRepTool_2d::TopOpeBRepTool_2d()
{
  //
}

//=======================================================================
//function : ~TopOpeBRepTool_2d
//=======================================================================
TopOpeBRepTool_2d::~TopOpeBRepTool_2d()
{
  //
}

// ------------------------------------------------------------------------------------
const TopoDS_Face& TopOpeBRepTool_2d::FancestorE(const TopoDS_Edge& E) const
{
  if (myMapEdgeFaces.IsEmpty()) { /// TODO
    //TopExp::MapShapesAndAncestors(myS1, TopAbs_EDGE, TopAbs_FACE, myMapEdgeFaces);
    //TopExp::MapShapesAndAncestors(myS2, TopAbs_EDGE, TopAbs_FACE, myMapEdgeFaces);
  }

  const TopTools_ListOfShape* lf = myMapEdgeFaces.Seek(E);
  if (lf != nullptr && !lf->IsEmpty())
  {
    return TopoDS::Face(lf->First());
  }

  static const TopoDS_Face anEmptyFace;
  return anEmptyFace;
}

// ------------------------------------------------------------------------------------
int TopOpeBRepTool_2d::Prepare(const TopoDS_Shape& S1,const TopoDS_Shape& S2)
{
  myNbC2d = 0;
  myS1 = S1;
  myS2 = S2;
  myMapEdgeFaces.Clear();

  TopExp::MapShapesAndAncestors(myS1, TopAbs_EDGE, TopAbs_FACE, myMapEdgeFaces);
  TopExp::MapShapesAndAncestors(myS2, TopAbs_EDGE, TopAbs_FACE, myMapEdgeFaces);
  return 0;
}

// ------------------------------------------------------------------------------------
Standard_Boolean TopOpeBRepTool_2d::HasC3D(const TopoDS_Edge& E)
{
  TopLoc_Location loc; Standard_Real f3d,l3d; Handle(Geom_Curve) C3D = BRep_Tool::Curve(E,loc,f3d,l3d);
  Standard_Boolean b = (!C3D.IsNull());
  return b;
}

// ------------------------------------------------------------------------------------
Standard_Boolean TopOpeBRepTool_2d::HasCurveOnSurface(const TopoDS_Edge& E,const TopoDS_Face& F) const
{
  Handle(Geom2d_Curve) C2D;
  return HasOldCurveOnSurface(E, F, C2D) || HasNewCurveOnSurface(E, F, C2D);
}

// ------------------------------------------------------------------------------------
Standard_Boolean TopOpeBRepTool_2d::HasOldCurveOnSurface(const TopoDS_Edge& E, const TopoDS_Face& F,
                                                         Handle(Geom2d_Curve)& C2D,
                                                         Standard_Real& f2d, Standard_Real& l2d,
                                                         Standard_Real& tol)
{
  Standard_Boolean hasold = Standard_False;
  tol = BRep_Tool::Tolerance(E);
  C2D = BRep_Tool::CurveOnSurface(E,F,f2d,l2d);
  hasold = (!C2D.IsNull());
  return hasold;
}

// ------------------------------------------------------------------------------------
TopOpeBRepTool_C2DF* TopOpeBRepTool_2d::PNewCurveOnSurface(const TopoDS_Edge& E, const TopoDS_Face& F) const
{
  const TopOpeBRepTool_ListOfC2DF* lc2df = myMapOfC2DF.Seek(E);
  if (lc2df == nullptr)
    return nullptr;

  for (TopOpeBRepTool_ListIteratorOfListOfC2DF it(*lc2df); it.More(); it.Next())
  {
    TopOpeBRepTool_C2DF& c2df = it.ChangeValue();
    if (c2df.IsFace(F))
    {
      return &c2df;
    }
  }
  return nullptr;
}
Standard_Boolean TopOpeBRepTool_2d::HasNewCurveOnSurface(const TopoDS_Edge& E, const TopoDS_Face& F,
                                                         Handle(Geom2d_Curve)& C2D,
                                                         Standard_Real& f2d, Standard_Real& l2d,
                                                         Standard_Real& tol) const
{
  const TopOpeBRepTool_C2DF* pc2df = PNewCurveOnSurface(E, F);
  Standard_Boolean hasnew = (pc2df != NULL);
  if (hasnew) C2D = pc2df->PC(f2d,l2d,tol);
  return hasnew;
}

// ------------------------------------------------------------------------------------
void TopOpeBRepTool_2d::AddNewCurveOnSurface(const Handle(Geom2d_Curve)& C2D,
                                             const TopoDS_Edge& E, const TopoDS_Face& F,
                                             const Standard_Real f2d, const Standard_Real l2d,
                                             const Standard_Real tol)
{
  if (C2D.IsNull())
    return;

  TopOpeBRepTool_ListOfC2DF& lc2df = *myMapOfC2DF.Bound(E, TopOpeBRepTool_ListOfC2DF());
  lc2df.Append(TopOpeBRepTool_C2DF(C2D, f2d, l2d, tol, F));
}

// ------------------------------------------------------------------------------------
Handle(Geom2d_Curve) TopOpeBRepTool_2d::make2d(const TopoDS_Edge& E, const TopoDS_Face& F,
                                               Standard_Real& f2d, Standard_Real& l2d,
                                               Standard_Real& tol,
                                               const Standard_Boolean trim3d)
{
  Handle(Geom2d_Curve) C2D = BRep_Tool::CurveOnSurface(E,F,f2d,l2d);
  if (!C2D.IsNull()) return C2D;

  // pas de 2D
  Standard_Real f3d,l3d;TopLoc_Location eloc;
  Handle(Geom_Curve) C1 = BRep_Tool::Curve(E,eloc,f3d,l3d);
  Standard_Boolean hasC3D = (!C1.IsNull());

  if ( hasC3D ) {
    Standard_Boolean elocid = eloc.IsIdentity();
    Handle(Geom_Curve) C2;
    if (elocid ) C2 = C1;
    else C2 = Handle(Geom_Curve)::DownCast(C1->Transformed(eloc.Transformation()));
    Standard_Real f=0.,l=0.;
    if (trim3d) {f=f3d; l=l3d;}
    C2D = TopOpeBRepTool_CurveTool::MakePCurveOnFace(F,C2,tol,f,l);
    f2d = f3d; l2d = l3d;
    return C2D;
  }
  else {
    // E sans courbe 2d sur F, E sans courbe 3d
    // une face accedant a E : FE
    const TopoDS_Face& FE = FancestorE(E);
    if (FE.IsNull()) return C2D;
    Standard_Boolean compminmaxUV = Standard_False;
    BRepAdaptor_Surface BAS(F,compminmaxUV);
    Handle(BRepAdaptor_Surface) BAHS = new BRepAdaptor_Surface(BAS);
    BRepAdaptor_Curve AC(E,FE);
    Handle(BRepAdaptor_Curve) AHC = new BRepAdaptor_Curve(AC);
    Standard_Real tolin; FTOL_FaceTolerances3d(F,FE,tolin);
    ProjLib_ProjectedCurve projcurv(BAHS,AHC,tolin);    
    C2D = MakePCurve(projcurv);
    Standard_Real f,l; BRep_Tool::Range(E,f,l);
    f2d = f; l2d = l;
  }
  
#ifdef OCCT_DEBUG
  if (TopOpeBRepTool_GettraceC2D() && C2D.IsNull()) {
    std::cout<<"#FC2D_make2d1 --> PCurve IsNull"<<std::endl;
  }
#endif

  return C2D;
} // make2d1

// ------------------------------------------------------------------------------------
//modified by NIZHNY-MZV  Mon Oct  4 10:37:36 1999
Handle(Geom2d_Curve) TopOpeBRepTool_2d::MakeCurveOnSurface(const TopoDS_Edge& E, const TopoDS_Face& F,
                                                           Standard_Real& f, Standard_Real& l,
                                                           Standard_Real& tol,
                                                           const Standard_Boolean trim3d)
{
#ifdef DRAW
  if (TopOpeBRepTool_GettraceC2D()) {
    std::cout<<"\n#FC2D_MakeCurveOnSurface : "<<std::endl;
    TCollection_AsciiString fnam("c2df");myNbC2d++;fnam=fnam+myNbC2d;FDRAW_DINS("",F,fnam,"");
    TCollection_AsciiString enam("c2de");myNbC2d++;enam=enam+myNbC2d;FDRAW_DINE(" ",E,enam,"\n");
    std::cout.flush(); debc2dnull();
  }
#endif
  
  Handle(Geom2d_Curve) C2D = make2d(E, F, f, l, tol, trim3d);
  AddNewCurveOnSurface(C2D, E, F, f, l, tol);
  return C2D;
}

// ------------------------------------------------------------------------------------
Handle(Geom2d_Curve) TopOpeBRepTool_2d::CurveOnSurface(const TopoDS_Edge& E, const TopoDS_Face& F,
                                                       Standard_Real& f, Standard_Real& l,
                                                       Standard_Real& tol,
                                                       const Standard_Boolean trim3d)
{
  Handle(Geom2d_Curve) C2D;
  const Standard_Boolean hasold = HasOldCurveOnSurface(E, F, C2D, f, l, tol);
  if (hasold) {
    return C2D;
  }
  const Standard_Boolean hasnew = HasNewCurveOnSurface(E, F, C2D, f, l, tol);
  if (hasnew) {
    return C2D;
  }
  C2D = MakeCurveOnSurface(E, F, f, l, tol, trim3d);
  return C2D;
}

// ------------------------------------------------------------------------------------
Handle(Geom2d_Curve) TopOpeBRepTool_2d::EditableCurveOnSurface(const TopoDS_Edge& E, const TopoDS_Face& F,
                                                               Standard_Real& f, Standard_Real& l,
                                                               Standard_Real& tol,
                                                               const Standard_Boolean trim3d)
{
  Standard_Boolean hasold = Standard_False;
  {
    Handle(Geom2d_Curve) C2D;
    hasold = HasOldCurveOnSurface(E, F, C2D, f, l, tol);
    if (hasold) {
      Handle(Geom2d_Curve) copC2D = Handle(Geom2d_Curve)::DownCast(C2D->Copy());
      return copC2D;
    }
  }
  Standard_Boolean hasnew = Standard_False;
  {
    Handle(Geom2d_Curve) newC2D;
    hasnew = HasNewCurveOnSurface(E, F, newC2D, f, l, tol);
    if (hasnew) {
      return newC2D;
    }  
  }
  Handle(Geom2d_Curve) makC2D = MakeCurveOnSurface(E, F, f, l, tol, trim3d);
  return makC2D;
}

// ------------------------------------------------------------------------------------
void TopOpeBRepTool_2d::translate(Handle(Geom2d_Curve) C2D,
                                  const TopoDS_Edge&,
                                  const TopoDS_Face& F,
                                  const TopoDS_Edge& EF)
{
  TopLoc_Location sloc; const Handle(Geom_Surface)& S1 = BRep_Tool::Surface(F,sloc);
  Standard_Boolean isperio = S1->IsUPeriodic() || S1->IsVPeriodic();
  gp_Dir2d d2d; gp_Pnt2d O2d; Standard_Boolean isuiso,isviso; 
  Standard_Boolean uviso = TopOpeBRepTool_TOOL::UVISO(C2D,isuiso,isviso,d2d,O2d);
  Standard_Boolean EFnull = EF.IsNull();
  
  if (isperio && uviso && !EFnull) {
    // C2D prend comme origine dans F l'origine de la pcurve de EF dans F
    TopoDS_Face FFOR = F;
    FFOR.Orientation(TopAbs_FORWARD);
    gp_Pnt2d p1,p2; BRep_Tool::UVPoints(EF,FFOR,p1,p2);
    Standard_Real pEF = isuiso ? p1.X() : p1.Y();
    Standard_Real pC2D = isuiso ? O2d.X() : O2d.Y();
    Standard_Real factor = pEF - pC2D;
    Standard_Boolean b = (Abs(factor) > 1.e-6);
    if ( b ) {
      gp_Vec2d transl(1.,0.); if (isviso) transl = gp_Vec2d(0.,1.);
      transl.Multiply(factor);
      C2D->Translate(transl);
    }
  }
}

// ------------------------------------------------------------------------------------
Handle(Geom2d_Curve) TopOpeBRepTool_2d::make2d(const TopoDS_Edge& E, const TopoDS_Face& F,
                                               const TopoDS_Edge& EF,
                                               Standard_Real& f2d, Standard_Real& l2d,
                                               Standard_Real& tol,
                                               const Standard_Boolean trim3d)
{
  Handle(Geom2d_Curve) C2D = BRep_Tool::CurveOnSurface(E,F,f2d,l2d);
  if (!C2D.IsNull()) return C2D;

  // pas de 2D
  Standard_Real f3d,l3d;TopLoc_Location eloc;
  Handle(Geom_Curve) C1 = BRep_Tool::Curve(E,eloc,f3d,l3d);
  Standard_Boolean hasC3D = (!C1.IsNull());

  if ( hasC3D ) {
    Standard_Boolean elocid = eloc.IsIdentity();
    Handle(Geom_Curve) C2;
    if (elocid ) C2 = C1;
    else C2 = Handle(Geom_Curve)::DownCast(C1->Transformed(eloc.Transformation()));
    Standard_Real f=0.,l=0.;
    if (trim3d) {f=f3d; l=l3d;}
    C2D = TopOpeBRepTool_CurveTool::MakePCurveOnFace(F,C2,tol,f,l);
    f2d = f3d; l2d = l3d;
    translate(C2D, E, F, EF);
    return C2D;
  }
  else {
    // E sans courbe 2d sur F, E sans courbe 3d
    // une face accedant a E : FE
    const TopoDS_Face& FE = FancestorE(E);
    if (FE.IsNull()) return C2D;
    Standard_Boolean compminmaxUV = Standard_False;
    BRepAdaptor_Surface BAS(F,compminmaxUV);
    Handle(BRepAdaptor_Surface) BAHS = new BRepAdaptor_Surface(BAS);
    BRepAdaptor_Curve AC(E,FE);
    Handle(BRepAdaptor_Curve) AHC = new BRepAdaptor_Curve(AC);
    Standard_Real tolin; FTOL_FaceTolerances3d(F,FE,tolin);
    ProjLib_ProjectedCurve projcurv(BAHS,AHC,tolin);
    C2D = MakePCurve(projcurv);
    Standard_Real f,l; BRep_Tool::Range(E,f,l);
    f2d = f; l2d = l;
    translate(C2D, E, F, EF);
  }

#ifdef OCCT_DEBUG
  if (TopOpeBRepTool_GettraceC2D() && C2D.IsNull()) {
    std::cout<<"#FC2D_make2d2 --> PCurve IsNull"<<std::endl;
  }
#endif

  return C2D;
} // make2d2

// ------------------------------------------------------------------------------------
Handle(Geom2d_Curve) TopOpeBRepTool_2d::CurveOnSurface(const TopoDS_Edge& E, const TopoDS_Face& F,
                                                       const TopoDS_Edge& EF,
                                                       Standard_Real& f2d, Standard_Real& l2d,
                                                       Standard_Real& tol,
                                                       const Standard_Boolean trim3d)
{
  Handle(Geom2d_Curve) C2D;

  const Standard_Boolean hasold = HasOldCurveOnSurface(E, F, C2D, f2d, l2d, tol);
  if (hasold) return C2D;

  TopOpeBRepTool_C2DF* pc2df = PNewCurveOnSurface(E, F);
  if (pc2df != NULL) {
    C2D = pc2df->PC(f2d,l2d,tol);
    translate(C2D, E, F, EF);
    pc2df->SetPC(C2D,f2d,l2d,tol);
    return C2D;
  }
  
#ifdef DRAW
  if (TopOpeBRepTool_GettraceC2D()) {
    std::cout<<"\n#FC2D_CurveOnSurface : "<<std::endl;
    TCollection_AsciiString fnam("c2df");myNbC2d++;fnam=fnam+myNbC2d;FDRAW_DINS("",F,fnam,"");
    TCollection_AsciiString enam("c2de");myNbC2d++;enam=enam+myNbC2d;FDRAW_DINE(" ",E,enam,"\n");
    std::cout.flush();debc2dnull();
  }
#endif

  C2D = make2d(E, F, EF, f2d, l2d, tol, trim3d);
  AddNewCurveOnSurface(C2D, E, F, f2d, l2d, tol);
  return C2D;
}
