// Copyright (c) 2024-2026 Kirill Gavrilov
//
// This file is part of Open CASCADE Technology software library.
//
// This library is free software; you can redistribute it and/or modify it under
// the terms of the GNU Lesser General Public License version 2.1 as published
// by the Free Software Foundation, with special exception defined in the file
// OCCT_LGPL_EXCEPTION.txt. Consult the file LICENSE_LGPL_21.txt included in OCCT
// distribution for complete text of the license and disclaimer of any warranty.
//
// Alternatively, this file may be used under the terms of
// commercial license or contractual agreement.

#include <AIS_ClippingPlanes.hxx>

#include <AIS_InteractiveContext.hxx>
#include <Graphic3d_ArrayOfPoints.hxx>
#include <Graphic3d_ArrayOfPolylines.hxx>
#include <IntAna_IntConicQuad.hxx>
#include <Prs3d_BndBox.hxx>
#include <Prs3d_PointAspect.hxx>
#include <Select3D_SensitivePrimitiveArray.hxx>
#include <V3d_View.hxx>

IMPLEMENT_STANDARD_RTTIEXT(AIS_ClippingPlanes, AIS_InteractiveObject)
IMPLEMENT_STANDARD_RTTIEXT(AIS_ClippingPlanesOwner, SelectMgr_EntityOwner)

// =======================================================================
// function : AIS_ClippingPlanes
// =======================================================================
AIS_ClippingPlanes::AIS_ClippingPlanes() : myBndBoxPlanes(new Graphic3d_SequenceOfHClipPlane())
{
  myDrawer->SetDisplayMode(0);
  SetInfiniteState(true);
  SetZLayer(Graphic3d_ZLayerId_Top);

  // disable clipping planes
  {
    Handle(Graphic3d_SequenceOfHClipPlane) aPlanes = new Graphic3d_SequenceOfHClipPlane();
    aPlanes->SetOverrideGlobal(true);
    SetClipPlanes(aPlanes);
  }

  // define aspects
  {
    const Quantity_Color     aColor = DefaultPlaneColor();
    Graphic3d_MaterialAspect aMat(Graphic3d_NOM_PLASTIC);
    aMat.SetTransparency(0.8f);
    Handle(Graphic3d_AspectFillArea3d) aFillAspect =
      new Graphic3d_AspectFillArea3d(Aspect_IS_SOLID, aColor, Quantity_NOC_RED, Aspect_TOL_SOLID, 1.0, aMat, aMat);
    // make polygon offset greater than default, so that capping plane will be drawn in front of plane
    aFillAspect->SetPolygonOffsets(Aspect_POM_Fill, 2.0f);
    aFillAspect->SetAlphaMode(Graphic3d_AlphaMode_Blend);
    aFillAspect->SetShadingModel(Graphic3d_TypeOfShadingModel_Unlit);
    aFillAspect->SetInteriorColor(Quantity_ColorRGBA(aColor, aMat.Alpha()));

    myDrawer->SetShadingAspect(new Prs3d_ShadingAspect(aFillAspect));
    myDrawer->SetFaceBoundaryAspect(new Prs3d_LineAspect(aColor, Aspect_TOL_SOLID, 1.0));
    myDrawer->SetFaceBoundaryDraw(true);
  }
  {
    Quantity_Color                     aColor = Quantity_NOC_ORANGE;
    Handle(Graphic3d_AspectFillArea3d) aFillAspect = new Graphic3d_AspectFillArea3d();
    *aFillAspect = *myDrawer->ShadingAspect()->Aspect();
    aFillAspect->SetInteriorColor(Quantity_ColorRGBA(aColor, 0.4f));

    myHilightDrawer = new Prs3d_Drawer();
    myHilightDrawer->Link(myDrawer);
    myHilightDrawer->SetAutoTriangulation(false);
    myHilightDrawer->SetColor(aColor);
    myHilightDrawer->SetTransparency(0.6f);
    myHilightDrawer->SetZLayer(Graphic3d_ZLayerId_UNKNOWN);
    myHilightDrawer->SetDisplayMode(0);
    myHilightDrawer->SetShadingAspect(new Prs3d_ShadingAspect(aFillAspect));
    myHilightDrawer->SetFaceBoundaryAspect(new Prs3d_LineAspect(Quantity_NOC_ORANGE, Aspect_TOL_SOLID, 1.0));
  }
  {
    myDynHilightDrawer = new Prs3d_Drawer();
    myDynHilightDrawer->Link(myDrawer);
    myDynHilightDrawer->SetColor(Quantity_NOC_CYAN1);
    myDynHilightDrawer->SetTransparency(0.8f);
    myDynHilightDrawer->SetAutoTriangulation(false);
    myDynHilightDrawer->SetZLayer(Graphic3d_ZLayerId_Topmost);
    myDynHilightDrawer->SetDisplayMode(0);
    myDynHilightDrawer->SetFaceBoundaryAspect(new Prs3d_LineAspect(Quantity_NOC_CYAN1, Aspect_TOL_SOLID, 1.0));
  }
}

// =======================================================================
// function : ClipPolygonByPlanes
// =======================================================================
void AIS_ClippingPlanes::ClipPolygonByPlanes(NCollection_Sequence<gp_Pnt>&                 thePol,
                                             const Handle(Graphic3d_SequenceOfHClipPlane)& theClipPlanes,
                                             const Handle(Graphic3d_ClipPlane)&            theDispPlane)
{
  if (theClipPlanes.IsNull() || theClipPlanes->IsEmpty())
    return;

  const double aTol = Precision::Intersection();
  for (Graphic3d_SequenceOfHClipPlane::Iterator aPlnIter(theClipPlanes); aPlnIter.More(); aPlnIter.Next())
  {
    const Handle(Graphic3d_ClipPlane)& aClipPlane = aPlnIter.Value();
    // clip regardless activity state
    //if (!aClipPlane->IsOn()) { continue; }
    for (const Graphic3d_ClipPlane* aSubPlaneIter = aClipPlane.get(); aSubPlaneIter != nullptr;
         aSubPlaneIter = aSubPlaneIter->ChainNextPlane().get())
    {
      if (theDispPlane == aSubPlaneIter)
      {
        continue;
      }

      const gp_Pln aPln = aSubPlaneIter->ToPlane();
      for (int aSegIter = 0; aSegIter < thePol.Size(); ++aSegIter)
      {
        const gp_Pnt& aPnt1 = thePol.Value(aSegIter + 1);
        const gp_Pnt& aPnt2 = thePol.Value(aSegIter + 2 <= thePol.Size() ? aSegIter + 2 : 1);
        const double  aSegLen = aPnt1.Distance(aPnt2);
        if (aSegLen <= aTol)
          continue;

        const gp_Lin        aLine(aPnt1, gp_Vec(aPnt1, aPnt2));
        IntAna_IntConicQuad anIntTool(aLine, aPln, Precision::Angular(), aTol);
        if (!anIntTool.IsDone() || anIntTool.IsParallel() || anIntTool.NbPoints() < 1)
          continue;

        if (anIntTool.ParamOnConic(1) <= aTol || (anIntTool.ParamOnConic(1) - aSegLen) >= -aTol)
          continue;

        thePol.InsertAfter(aSegIter + 1, anIntTool.Point(1));
        ++aSegIter;
      }

      for (NCollection_Sequence<gp_Pnt>::Iterator aPntIter(thePol); aPntIter.More();)
      {
        const gp_Pnt&         aPnt = aPntIter.Value();
        const Graphic3d_Vec4d aCheckPnt(aPnt.X(), aPnt.Y(), aPnt.Z(), 1.0);
        const double          aVal = aSubPlaneIter->GetEquation().Dot(aCheckPnt);
        if (aClipPlane->IsChain())
        {
          if (aVal > aTol)
          {
            thePol.Remove(aPntIter);
            continue;
          }
        }
        else if (aVal < -aTol) // clip points by plane, but skip points ON the plane
        {
          thePol.Remove(aPntIter);
          continue;
        }

        aPntIter.Next();
      }
    }
  }
}

// =======================================================================
// function : GetPlaneQuadNodes
// =======================================================================
NCollection_Sequence<gp_Pnt> AIS_ClippingPlanes::GetPlaneQuadNodes(const Bnd_Box& theBox, const gp_Pln& thePlane)
{
  const Handle(Graphic3d_ArrayOfSegments) aBndSegs = Prs3d_BndBox::FillSegments(theBox);
  Handle(Graphic3d_SequenceOfHClipPlane)  aBndPlanes = GetClippingBndBox(theBox);
  return GetPlaneQuadNodes(aBndPlanes, aBndSegs, thePlane);
}

// =======================================================================
// function : GetPlaneQuadNodes
// =======================================================================
NCollection_Sequence<gp_Pnt> AIS_ClippingPlanes::GetPlaneQuadNodes(
  const Handle(Graphic3d_SequenceOfHClipPlane)& theBndPlanes,
  const Handle(Graphic3d_ArrayOfSegments)&      theBndSegs,
  const gp_Pln&                                 thePlane)
{
  const gp_Dir anXDir = thePlane.XAxis().Direction();
  const gp_Dir anYDir = thePlane.YAxis().Direction();
  gp_Pnt       aLoc = thePlane.Axis().Location();

  Graphic3d_Vec2d aSizeXY(Precision::Confusion());
  for (int anEdgeIter = 0; anEdgeIter < theBndSegs->EdgeNumber() / 2; ++anEdgeIter)
  {
    const int    aInd[2] = {theBndSegs->Edge(anEdgeIter * 2 + 1), theBndSegs->Edge(anEdgeIter * 2 + 2)};
    const gp_Vec aSeg(theBndSegs->Vertice(aInd[0]), theBndSegs->Vertice(aInd[1]));
    aSizeXY.x() = Max(aSizeXY.x(), Abs(aSeg.XYZ().Dot(anXDir.XYZ())));
    aSizeXY.y() = Max(aSizeXY.y(), Abs(aSeg.XYZ().Dot(anYDir.XYZ())));
  }
  if (!theBndPlanes.IsNull() && !theBndPlanes->IsEmpty())
    aSizeXY *= 5.0;

  Graphic3d_Vec2d aMoveVec;
  for (int aVertIter = 1; aVertIter <= theBndSegs->VertexNumber(); ++aVertIter)
  {
    const gp_Vec aVec(aLoc, theBndSegs->Vertice(aVertIter));
    const double aDotDX = aVec.XYZ().Dot(anXDir.XYZ());
    const double aDotDY = aVec.XYZ().Dot(anYDir.XYZ());
    aMoveVec.x() += aDotDX;
    aMoveVec.y() += aDotDY;
  }
  aMoveVec.x() /= double(theBndSegs->VertexNumber());
  aMoveVec.y() /= double(theBndSegs->VertexNumber());
  aLoc = aLoc.XYZ() + anXDir.XYZ() * aMoveVec.x() + anYDir.XYZ() * aMoveVec.y();

  NCollection_Sequence<gp_Pnt> aResNodes;
  aResNodes.Append(aLoc.XYZ() - anXDir.XYZ() * aSizeXY.x() - anYDir.XYZ() * aSizeXY.y());
  aResNodes.Append(aLoc.XYZ() - anXDir.XYZ() * aSizeXY.x() + anYDir.XYZ() * aSizeXY.y());
  aResNodes.Append(aLoc.XYZ() + anXDir.XYZ() * aSizeXY.x() + anYDir.XYZ() * aSizeXY.y());
  aResNodes.Append(aLoc.XYZ() + anXDir.XYZ() * aSizeXY.x() - anYDir.XYZ() * aSizeXY.y());

  ClipPolygonByPlanes(aResNodes, theBndPlanes, Handle(Graphic3d_ClipPlane)());
  return aResNodes;
}

// =======================================================================
// function : GetPlaneTriangles
// =======================================================================
Handle(Graphic3d_ArrayOfTriangles) AIS_ClippingPlanes::GetPlaneTriangles(
  const Handle(Graphic3d_SequenceOfHClipPlane)& theBndPlanes,
  const Handle(Graphic3d_ArrayOfSegments)&      theBndSegs,
  const Handle(Graphic3d_SequenceOfHClipPlane)& theClipPlanes,
  const Handle(Graphic3d_ClipPlane)&            theDispPlane,
  const double                                  theOffset)
{
  const gp_Pln aPln = theDispPlane->ToPlane();
  const gp_Dir aNorm = aPln.Axis().Direction();
  const gp_XYZ anOffset = aNorm.XYZ() * theOffset;

  NCollection_Sequence<gp_Pnt> aNodes = GetPlaneQuadNodes(theBndPlanes, theBndSegs, aPln);
  ClipPolygonByPlanes(aNodes, theClipPlanes, theDispPlane);
  if (aNodes.Size() <= 2)
    return Handle(Graphic3d_ArrayOfTriangles)();

  int aNbNodes = aNodes.Length();
  int aNbTris = 1;
  switch (aNodes.Length())
  {
    case 3: aNbTris = 1; break;
    case 4: aNbTris = 2; break;
    default:
      aNbNodes = aNodes.Length() + 1;
      aNbTris = aNodes.Length();
      break;
  }

  Handle(Graphic3d_ArrayOfTriangles) aTris = new Graphic3d_ArrayOfTriangles(aNbNodes,
                                                                            aNbTris * 3,
                                                                            Graphic3d_ArrayFlags_None);
  if (aNbTris > 2)
  {
    gp_XYZ aCenter;
    for (const gp_Pnt& aNodeIter : aNodes)
      aCenter += aNodeIter.XYZ();

    aCenter /= double(aNodes.Length());
    aTris->AddVertex(aCenter + anOffset);
  }
  for (const gp_Pnt& aNodeIter : aNodes)
    aTris->AddVertex(aNodeIter.XYZ() + anOffset);

  switch (aNbTris)
  {
    case 1: aTris->AddEdges(1, 2, 3); break;
    case 2: aTris->AddQuadTriangleEdges(1, 2, 3, 4); break;
    default: aTris->AddTriangleFanEdges(1, aNbNodes, true); break;
  }

  return aTris;
}

// =======================================================================
// function : GetPlaneEdges
// =======================================================================
Handle(Graphic3d_ArrayOfPolylines) AIS_ClippingPlanes::GetPlaneEdges(
  const Handle(Graphic3d_SequenceOfHClipPlane)& theBndPlanes,
  const Handle(Graphic3d_ArrayOfSegments)&      theBndSegs,
  const Handle(Graphic3d_SequenceOfHClipPlane)& theClipPlanes,
  const Handle(Graphic3d_ClipPlane)&            theDispPlane,
  const double                                  theOffset)
{
  const gp_Pln aPln = theDispPlane->ToPlane();
  const gp_Dir aNorm = aPln.Axis().Direction();
  const gp_XYZ anOffset = aNorm.XYZ() * theOffset;

  NCollection_Sequence<gp_Pnt> aNodes = GetPlaneQuadNodes(theBndPlanes, theBndSegs, aPln);
  ClipPolygonByPlanes(aNodes, theClipPlanes, theDispPlane);
  if (aNodes.Size() < 2)
    return Handle(Graphic3d_ArrayOfPolylines)();

  Handle(Graphic3d_ArrayOfPolylines) aPolyline = new Graphic3d_ArrayOfPolylines(aNodes.Size() + 1,
                                                                                Graphic3d_ArrayFlags_None);
  for (const gp_Pnt& aNodeIter : aNodes)
    aPolyline->AddVertex(aNodeIter.XYZ() + anOffset);

  aPolyline->AddVertex(aNodes.First().XYZ() + anOffset);
  return aPolyline;
}

// =======================================================================
// function : GetClippingBndBox
// =======================================================================
Handle(Graphic3d_SequenceOfHClipPlane) AIS_ClippingPlanes::GetClippingBndBox(const Bnd_Box& theBox)
{
  Handle(Graphic3d_SequenceOfHClipPlane) aPlanes = new Graphic3d_SequenceOfHClipPlane();
  if (theBox.IsVoid())
    return aPlanes;

  const gp_Pln aBoxSides[6] = {
    gp_Pln(theBox.CornerMin(), gp::DX()),
    gp_Pln(theBox.CornerMin(), gp::DY()),
    gp_Pln(theBox.CornerMin(), gp::DZ()),
    gp_Pln(theBox.CornerMax(), -gp::DX()),
    gp_Pln(theBox.CornerMax(), -gp::DY()),
    gp_Pln(theBox.CornerMax(), -gp::DZ()),
  };
  for (const gp_Pln& aSideIter : aBoxSides)
  {
    Handle(Graphic3d_ClipPlane) aBoxSide = new Graphic3d_ClipPlane(aSideIter);
    aPlanes->Append(aBoxSide);
  }
  return aPlanes;
}

// =======================================================================
// function : updateSceneBox
// =======================================================================
void AIS_ClippingPlanes::updateSceneBox(const Handle(PrsMgr_PresentationManager)& thePrsMgr)
{
  myCachedBox = mySceneBox;
  if (myCachedBox.IsVoid())
  {
    for (Graphic3d_IndexedMapOfView::Iterator aViewIter(thePrsMgr->StructureManager()->DefinedViews());
         aViewIter.More();
         aViewIter.Next())
    {
      const Graphic3d_CView* aView = aViewIter.Value();
      if (myViewAffinity->IsVisible(aView->Identification()))
      {
        myCachedBox = aView->MinMaxValues(false);
        break;
      }
    }

    if (!myCachedBox.IsVoid())
    {
      // add a gap
      const gp_XYZ aGap = (myCachedBox.CornerMax().XYZ() - myCachedBox.CornerMin().XYZ()) * 0.1;
      myCachedBox.Add(gp_Pnt(myCachedBox.CornerMin().XYZ() - aGap));
      myCachedBox.Add(gp_Pnt(myCachedBox.CornerMax().XYZ() + aGap));
    }
  }
  if (myCachedBox.IsVoid())
  {
    myCachedBox.Add(gp_Pnt(-1.0, -1.0, -1.0));
    myCachedBox.Add(gp_Pnt(1.0, 1.0, 1.0));
  }

  myBndBoxPlanes = GetClippingBndBox(myCachedBox);
}

// =======================================================================
// function : Compute
// =======================================================================
void AIS_ClippingPlanes::Compute(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                 const Handle(Prs3d_Presentation)&         thePrs,
                                 const Standard_Integer                    theMode)
{
  thePrs->SetInfiniteState(true);
  if (theMode != 0)
    return;

  updateSceneBox(thePrsMgr);
  const Handle(Graphic3d_ArrayOfSegments) aBndSegs = Prs3d_BndBox::FillSegments(myCachedBox);

  Handle(Graphic3d_Group) aTrisGroup;
  if (myDatumDispMode == Prs3d_DM_Shaded)
  {
    aTrisGroup = thePrs->NewGroup();
    aTrisGroup->SetGroupPrimitivesAspect(myDrawer->ShadingAspect()->Aspect());
  }

  Handle(Graphic3d_Group) aLineGroup;
  if (myDatumDispMode == Prs3d_DM_WireFrame || myDrawer->FaceBoundaryDraw())
  {
    aLineGroup = thePrs->NewGroup();
    aLineGroup->SetGroupPrimitivesAspect(myDrawer->FaceBoundaryAspect()->Aspect());
  }

  Handle(Graphic3d_Group) aPntGroup;
  if (myToDisplayPoints)
  {
    aPntGroup = thePrs->NewGroup();
    aPntGroup->SetGroupPrimitivesAspect(myDrawer->PointAspect()->Aspect());
  }

  Handle(Graphic3d_SequenceOfHClipPlane) aPlanes = myManagedPlanes;
  if (!myDispPlane.IsNull())
  {
    if (myManagedPlanes.IsNull() || !myManagedPlanes->Contains(myDispPlane))
    {
      aPlanes = new Graphic3d_SequenceOfHClipPlane();
      aPlanes->Append(myDispPlane);
    }
  }

  for (Graphic3d_SequenceOfHClipPlane::Iterator aPlaneIter(aPlanes); aPlaneIter.More(); aPlaneIter.Next())
  {
    const Handle(Graphic3d_ClipPlane)& aClipPlane = aPlaneIter.Value();
    // display regardless activity state
    //if (!aClipPlane->IsOn()) { continue; }
    for (const Graphic3d_ClipPlane* aSubPlaneIter = aClipPlane.get(); aSubPlaneIter != nullptr;
         aSubPlaneIter = aSubPlaneIter->ChainNextPlane().get())
    {
      if (!myDispPlane.IsNull() && myDispPlane != aSubPlaneIter)
        continue;

      // shaded presentation
      if (Handle(Graphic3d_ArrayOfTriangles) aTris =
            !aTrisGroup.IsNull() ? GetPlaneTriangles(myBndBoxPlanes, aBndSegs, myManagedPlanes, aSubPlaneIter)
                                 : Handle(Graphic3d_ArrayOfTriangles)())
        aTrisGroup->AddPrimitiveArray(aTris);

      // boundaries presentation
      if (Handle(Graphic3d_ArrayOfPolylines) aPolyline =
            !aLineGroup.IsNull() ? GetPlaneEdges(myBndBoxPlanes, aBndSegs, myManagedPlanes, aSubPlaneIter)
                                 : Handle(Graphic3d_ArrayOfPolylines)())
        aLineGroup->AddPrimitiveArray(aPolyline);

      // points presentation (diagnostics)
      if (!aPntGroup.IsNull())
      {
        NCollection_Sequence<gp_Pnt> aNodes = GetPlaneQuadNodes(myBndBoxPlanes, aBndSegs, aSubPlaneIter->ToPlane());
        ClipPolygonByPlanes(aNodes, myManagedPlanes, aSubPlaneIter);

        Handle(Graphic3d_ArrayOfPoints) aPnts = new Graphic3d_ArrayOfPoints(aNodes.Size(), Graphic3d_ArrayFlags_None);
        for (const gp_Pnt& aNodeIter : aNodes)
          aPnts->AddVertex(aNodeIter);

        aPntGroup->AddPrimitiveArray(aPnts);
      }
    }
  }
}

// =======================================================================
// function : ComputeSelection
// =======================================================================
void AIS_ClippingPlanes::ComputeSelection(const Handle(SelectMgr_Selection)& theSel, const Standard_Integer theMode)
{
  if (theMode != 0)
    return;

  Handle(Graphic3d_SequenceOfHClipPlane) aPlanes = myManagedPlanes;
  if (!myDispPlane.IsNull())
  {
    if (myManagedPlanes.IsNull() || !myManagedPlanes->Contains(myDispPlane))
    {
      aPlanes = new Graphic3d_SequenceOfHClipPlane();
      aPlanes->Append(myDispPlane);
    }
  }

  const Handle(Graphic3d_ArrayOfSegments) aBndSegs = Prs3d_BndBox::FillSegments(myCachedBox);
  for (Graphic3d_SequenceOfHClipPlane::Iterator aPlaneIter(aPlanes); aPlaneIter.More(); aPlaneIter.Next())
  {
    const Handle(Graphic3d_ClipPlane)& aClipPlane = aPlaneIter.Value();
    // display regardless activity state
    //if (!aClipPlane->IsOn()) { continue; }
    for (const Graphic3d_ClipPlane* aSubPlaneIter = aClipPlane.get(); aSubPlaneIter != nullptr;
         aSubPlaneIter = aSubPlaneIter->ChainNextPlane().get())
    {
      if (!myDispPlane.IsNull() && myDispPlane != aSubPlaneIter)
        continue;

      // apply negative offset to avoid fighting between clipping plane and clipped objects
      const gp_Dir aPlaneNorm = aSubPlaneIter->ToPlane().Axis().Direction();
      const gp_XYZ anOffsetXYZ = (myCachedBox.CornerMax().XYZ() - myCachedBox.CornerMin().XYZ()) * 0.01;
      double       anOffsetVal = Abs(aPlaneNorm.XYZ().Dot(anOffsetXYZ));

      Handle(Graphic3d_ArrayOfTriangles) aTris =
        GetPlaneTriangles(myBndBoxPlanes, aBndSegs, myManagedPlanes, aSubPlaneIter, -anOffsetVal);
      if (aTris.IsNull())
        continue;

      // find opposite directed planes in chain
      Handle(Graphic3d_ClipPlane) anOppPlane;
      for (const Graphic3d_ClipPlane* aCoPlaneIter = aClipPlane.get(); aCoPlaneIter != nullptr;
           aCoPlaneIter = aCoPlaneIter->ChainNextPlane().get())
      {
        if (aCoPlaneIter == aSubPlaneIter)
          continue;

        if (aCoPlaneIter->ToPlane().Axis().Direction().IsOpposite(aPlaneNorm, Precision::Angular()))
        {
          anOppPlane = aCoPlaneIter;
          break;
        }
      }

      Handle(AIS_ClippingPlanesOwner) aPlaneOwner = new AIS_ClippingPlanesOwner(this, aSubPlaneIter, anOppPlane);
      Handle(Select3D_SensitivePrimitiveArray) aSens = new Select3D_SensitivePrimitiveArray(aPlaneOwner);
      aSens->SetSensitivityFactor(0);
      aSens->InitTriangulation(aTris->Attributes(), aTris->Indices(), TopLoc_Location());
      theSel->Add(aSens);
    }
  }
}

// =======================================================================
// function : ProcessDragging
// =======================================================================
Standard_Boolean AIS_ClippingPlanes::ProcessDragging(const Handle(AIS_InteractiveContext)& theCtx,
                                                     const Handle(V3d_View)&               theView,
                                                     const Handle(SelectMgr_EntityOwner)&  theOwner,
                                                     const Graphic3d_Vec2i&                theFrom,
                                                     const Graphic3d_Vec2i&                theTo,
                                                     const AIS_DragAction                  theAction)
{
  Handle(AIS_ClippingPlanesOwner) anOwner = Handle(AIS_ClippingPlanesOwner)::DownCast(theOwner);
  if (anOwner.IsNull())
    return false;

  const Handle(Graphic3d_ClipPlane)& aDragPlane = anOwner->Plane();
  const Handle(Graphic3d_ClipPlane)& aOppPlane = anOwner->OppositePlane();
  if (theAction == AIS_DragAction_Stop || theAction == AIS_DragAction_Abort)
  {
    theCtx->ClearDetected();
    theOwner->Unhilight(theCtx->MainPrsMgr());

    if (theAction == AIS_DragAction_Abort)
    {
      gp_Pln aPln = aDragPlane->ToPlane();
      aPln.SetLocation(myDragFromLoc);
      aDragPlane->SetEquation(aPln);
      if (!aOppPlane.IsNull())
      {
        aPln = aOppPlane->ToPlane();
        aPln.SetLocation(myDragOppLoc);
        aOppPlane->SetEquation(aPln);
      }
      theCtx->RecomputePrsOnly(this, false, true);
    }
    theCtx->RecomputeSelectionOnly(this);
    return true;
  }

  if (theAction == AIS_DragAction_Start)
  {
    myDragFromLoc = aDragPlane->ToPlane().Location();
    if (!aOppPlane.IsNull())
      myDragOppLoc = aOppPlane->ToPlane().Location();

    // compute Z plane
    myDragPointZ = RealLast();
    for (int aPickIter = 1; aPickIter <= theCtx->MainSelector()->NbPicked(); ++aPickIter)
    {
      if (theCtx->MainSelector()->Picked(aPickIter) == theOwner)
      {
        const gp_Pnt aDragPoint = theCtx->MainSelector()->PickedPoint(aPickIter);
        const gp_Pnt anObjFromProj = theView->Camera()->Project(aDragPoint);
        myDragPointZ = anObjFromProj.Z();
        break;
      }
    }
    return !Precision::IsInfinite(myDragPointZ);
  }
  if (Precision::IsInfinite(myDragPointZ))
    return false;

  const int aDeltaPx = (theTo - theFrom).cwiseAbs().maxComp();
  gp_Vec    aMoveVec;
  if (aDeltaPx > 2)
  {
    const Graphic3d_Vec2i           aWinSize = theView->Window()->Dimensions();
    const Handle(Graphic3d_Camera)& aCam = theView->Camera();

    gp_Pnt aMouseUnproj[2];
    for (int aPntIter = 0; aPntIter < 2; ++aPntIter)
    {
      const Graphic3d_Vec2i& aPnt = aPntIter == 0 ? theFrom : theTo;
      const gp_Pnt           aMouse(2.0 * aPnt.x() / aWinSize.x() - 1.0,
                          2.0 * (aWinSize.y() - 1 - aPnt.y()) / aWinSize.y() - 1.0,
                          myDragPointZ);
      aMouseUnproj[aPntIter] = aCam->UnProject(aMouse);
    }
    aMoveVec = gp_Vec(aMouseUnproj[0], aMouseUnproj[1]);
  }

  gp_Pln       aPln = aDragPlane->ToPlane();
  const gp_Dir aNorm = aPln.Axis().Direction();
  const double aDisp = aMoveVec.XYZ().Dot(aNorm.XYZ());
  gp_XYZ       aNewLoc = myDragFromLoc.XYZ() + aNorm.XYZ() * aDisp;
  if (myCachedBox.IsOut(aNewLoc))
  {
    const gp_Pnt aMin = myCachedBox.CornerMin();
    const gp_Pnt aMax = myCachedBox.CornerMax();
    gp_XYZ       aNewLocClamped = aNewLoc;
    for (int aCompIter = 1; aCompIter <= 3; ++aCompIter)
      aNewLocClamped.SetCoord(aCompIter,
                              Max(Min(aNewLocClamped.Coord(aCompIter), aMax.Coord(aCompIter)), aMin.Coord(aCompIter)));

    const double aDispClamp = gp_Vec(myDragFromLoc, aNewLocClamped).XYZ().Dot(aPln.Axis().Direction().XYZ());
    aNewLoc = myDragFromLoc.XYZ() + aNorm.XYZ() * aDispClamp;
  }

  aPln.SetLocation(aNewLoc);
  aDragPlane->SetEquation(aPln);

  if (!aOppPlane.IsNull())
  {
    // move opposite plane, when they collide
    gp_Pln anOppPln = aOppPlane->ToPlane();
    gp_Vec anOppVec(aNewLoc, anOppPln.Location());
    double anOppDist = anOppVec.XYZ().Dot(aNorm.XYZ());
    if (anOppDist > Precision::Confusion())
    {
      anOppPln.SetLocation(aNewLoc - aNorm.XYZ() * Precision::Confusion());
      aOppPlane->SetEquation(anOppPln);
    }
  }

  theCtx->ClearDetected();
  theCtx->RecomputePrsOnly(this, false, true);
  theOwner->HilightWithColor(theCtx->MainPrsMgr(), myHilightDrawer, 0);
  return true;
}

// =======================================================================
// function : AIS_ClippingPlanesOwner
// =======================================================================
AIS_ClippingPlanesOwner::AIS_ClippingPlanesOwner(const Handle(AIS_ClippingPlanes)&  theObject,
                                                 const Handle(Graphic3d_ClipPlane)& thePlane,
                                                 const Handle(Graphic3d_ClipPlane)& theOppPlane,
                                                 Standard_Integer                   thePriority)
: SelectMgr_EntityOwner((const Handle(SelectMgr_SelectableObject)&)theObject, thePriority),
  myPlane(thePlane),
  myOppPlane(theOppPlane)
{
  myFromDecomposition = true;
}

// =======================================================================
// function : HandleMouseClick
// =======================================================================
Standard_Boolean AIS_ClippingPlanesOwner::HandleMouseClick(const Graphic3d_Vec2i&,
                                                           Aspect_VKeyMouse theButton,
                                                           Aspect_VKeyFlags theModifiers,
                                                           bool             theIsDoubleClick)
{
  if (theButton != Aspect_VKeyMouse_LeftButton || theModifiers != Aspect_VKeyFlags_NONE || theIsDoubleClick)
    return false;

  return true;
}

// =======================================================================
// function : Unhilight
// =======================================================================
void AIS_ClippingPlanesOwner::Unhilight(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                        const Standard_Integer                    theMode)
{
  if (Handle(Prs3d_Presentation) aPrs = Selectable()->GetSelectPresentation(Handle(PrsMgr_PresentationManager)()))
    aPrs->Erase();

  base_type::Unhilight(thePrsMgr, theMode);
}

// =======================================================================
// function : HilightWithColor
// =======================================================================
void AIS_ClippingPlanesOwner::HilightWithColor(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                               const Handle(Prs3d_Drawer)&               theStyle,
                                               const Standard_Integer                    theMode)
{
  Handle(AIS_ClippingPlanes) anObj = Handle(AIS_ClippingPlanes)::DownCast(mySelectable);
  if (anObj.IsNull())
  {
    base_type::HilightWithColor(thePrsMgr, theStyle, theMode);
    return;
  }

  Handle(Prs3d_Presentation) aPrs = thePrsMgr->IsImmediateModeOn() ? anObj->GetHilightPresentation(thePrsMgr)
                                                                   : anObj->GetSelectPresentation(thePrsMgr);
  const Graphic3d_ZLayerId   aZLayer = theStyle->ZLayer() != -1
                                       ? theStyle->ZLayer()
                                       : (thePrsMgr->IsImmediateModeOn() ? Graphic3d_ZLayerId_Top : anObj->ZLayer());
  aPrs->Clear();
  aPrs->SetInfiniteState(true);
  if (aPrs->GetZLayer() != aZLayer)
    aPrs->SetZLayer(aZLayer);

  const Handle(Graphic3d_ArrayOfSegments)       aBndSegs = Prs3d_BndBox::FillSegments(anObj->CachedBox());
  const Handle(Graphic3d_SequenceOfHClipPlane)& aBoxPlanes = anObj->BndBoxPlanes();
  const Handle(Graphic3d_SequenceOfHClipPlane)& aManPlanes = anObj->ManagedPlanes();
  if (anObj->DatumDisplayMode() == Prs3d_DM_Shaded)
  {
    if (Handle(Graphic3d_ArrayOfTriangles) aTris =
          AIS_ClippingPlanes::GetPlaneTriangles(aBoxPlanes, aBndSegs, aManPlanes, myPlane))
    {
      Handle(Graphic3d_Group) aTrisGroup = aPrs->NewGroup();
      aTrisGroup->SetGroupPrimitivesAspect(theStyle->ShadingAspect()->Aspect());
      aTrisGroup->AddPrimitiveArray(aTris);
    }
  }

  if (anObj->DatumDisplayMode() == Prs3d_DM_WireFrame || theStyle->FaceBoundaryDraw())
  {
    if (Handle(Graphic3d_ArrayOfPolylines) aPolyline =
          AIS_ClippingPlanes::GetPlaneEdges(aBoxPlanes, aBndSegs, aManPlanes, myPlane))
    {
      Handle(Graphic3d_Group) aLineGroup = aPrs->NewGroup();
      aLineGroup->SetGroupPrimitivesAspect(theStyle->FaceBoundaryAspect()->Aspect());
      aLineGroup->AddPrimitiveArray(aPolyline);
    }
  }

  if (thePrsMgr->IsImmediateModeOn())
    thePrsMgr->AddToImmediateList(aPrs);
  else
    aPrs->Display();
}
