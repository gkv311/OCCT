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

#include <AIS_ScaleRuler.hxx>

#include <AIS_InteractiveContext.hxx>
#include <Font_NameOfFont.hxx>
#include <Graphic3d_ArrayOfTriangles.hxx>
#include <Graphic3d_AspectLine3d.hxx>
#include <Graphic3d_Text.hxx>
#include <Select3D_SensitivePrimitiveArray.hxx>
#include <Units.hxx>
#include <V3d_View.hxx>

// meter units
static constexpr const char* LENGTH_UNITS[] = {
  "Em", //
  "Pm",
  "Tm",
  "Gm",
  "Mm",
  "km",
  "m",
  "cm",
  "mm",
  "\xC2\xB5m",
  "nm",
  "pm",
  "\0" // mark and of the array
};

//! Method to create array of triangles.
static Handle(Graphic3d_ArrayOfTriangles) createQuad(const Graphic3d_Vec2d& theXY, // left bottom
                                                     const Graphic3d_Vec2d& theSize)
{
  Handle(Graphic3d_ArrayOfTriangles) aTris = new Graphic3d_ArrayOfTriangles(4, 6);

  const int aVertIndex = aTris->VertexNumber() + 1;
  aTris->AddVertex(gp_Pnt(theXY.x(), theXY.y() + theSize.y(), 0.0));
  aTris->AddVertex(gp_Pnt(theXY.x(), theXY.y(), 0.0));
  aTris->AddVertex(gp_Pnt(theXY.x() + theSize.x(), theXY.y(), 0.0));
  aTris->AddVertex(gp_Pnt(theXY.x() + theSize.x(), theXY.y() + theSize.y(), 0.0));
  aTris->AddQuadTriangleEdges(aVertIndex, aVertIndex + 1, aVertIndex + 2, aVertIndex + 3);
  return aTris;
}

IMPLEMENT_STANDARD_RTTIEXT(AIS_ScaleRuler, AIS_InteractiveObject)

// ================================================================
// Function : AIS_ScaleRuler
// ================================================================
AIS_ScaleRuler::AIS_ScaleRuler() : AIS_ScaleRuler(Aspect_TOTP_RIGHT_LOWER, Graphic3d_Vec2i(myDefaultOffset), myBaseSize)
{
  //
}

// ================================================================
// Function : AIS_ScaleRuler
// ================================================================
AIS_ScaleRuler::AIS_ScaleRuler(const Aspect_TypeOfTriedronPosition theCorner,
                               const Graphic3d_Vec2i&              theOffset,
                               const Graphic3d_Vec2&               theBaseSize)
{
  myBaseSize = theBaseSize;
  myTransformPersistence = new Graphic3d_TransformPers(Graphic3d_TMF_2d, theCorner, theOffset);
  myTransformPersistence->SetDensityIndependent(true);
  myDrawer->SetZLayer(Graphic3d_ZLayerId_TopOSD);

  myDrawer->SetLineAspect(new Prs3d_LineAspect(Quantity_NOC_GRAY90, Aspect_TOL_DASH, 1.0f));
  myDrawer->SetTextAspect(new Prs3d_TextAspect());
  myDrawer->TextAspect()->SetFont(Font_NOF_MONOSPACE);
  myDrawer->TextAspect()->SetHeight(myDefaultFontHeight);
  myDrawer->TextAspect()->SetColor(Quantity_NOC_WHITE);
  myDrawer->TextAspect()->Aspect()->SetColorSubTitle(Quantity_NOC_BLACK);
  myDrawer->TextAspect()->Aspect()->SetTextDisplayType(Aspect_TODT_SHADOW);

  myShadowAsp = new Graphic3d_AspectLine3d();
  *myShadowAsp = *myDrawer->LineAspect()->Aspect();
}

// ================================================================
// Function : ~AIS_ScaleRuler
// ================================================================
AIS_ScaleRuler::~AIS_ScaleRuler()
{
  //
}

// ================================================================
// Function : SetFontHeight
// ================================================================
void AIS_ScaleRuler::SetFontHeight(Standard_Real theHeight)
{
  if (myDrawer->TextAspect()->Height() != theHeight)
  {
    myDrawer->TextAspect()->SetHeight(theHeight);
    SetToUpdate();
  }
}

// ================================================================
// Function : SetOffset
// ================================================================
void AIS_ScaleRuler::SetOffset(const Graphic3d_Vec2i& theOffset)
{
  if (!myTransformPersistence->Offset2d().IsEqual(theOffset))
  {
    myTransformPersistence->SetOffset2d(theOffset);
    SetToUpdate();
  }
}

// ================================================================
// Function : SetCorner
// ================================================================
void AIS_ScaleRuler::SetCorner(Aspect_TypeOfTriedronPosition theCorner)
{
  if (myTransformPersistence->Corner2d() != theCorner)
  {
    myTransformPersistence->SetCorner2d(theCorner);
    SetToUpdate();
  }
}

// ================================================================
// Function : SetCorner
// ================================================================
void AIS_ScaleRuler::SetBaseSize(const Graphic3d_Vec2& theSize)
{
  if (myBaseSize != theSize)
  {
    myBaseSize = theSize;
    SetToUpdate();
  }
}

// ================================================================
// Function : getPrsCorner
// ================================================================
Graphic3d_Vec2i AIS_ScaleRuler::getPrsCorner(bool theIsFullLen) const
{
  const Graphic3d_Vec2i aDispSize(
    Graphic3d_Vec2d(!theIsFullLen && myDispLenBase > 0.0f ? myDispLenBase : myBaseSize.x(), myBaseSize.y()));

  Graphic3d_Vec2i aLeftBot;
  if ((myTransformPersistence->Corner2d() & Aspect_TOTP_RIGHT) != 0)
    aLeftBot.x() = -aDispSize.x();
  else if ((myTransformPersistence->Corner2d() & Aspect_TOTP_LEFT) == 0)
    aLeftBot.x() = -aDispSize.x() / 2;

  if ((myTransformPersistence->Corner2d() & Aspect_TOTP_TOP) != 0)
    aLeftBot.y() = -aDispSize.y() - int(myDrawer->TextAspect()->Height());

  return aLeftBot;
}

// ================================================================
// Function : Compute
// ================================================================
void AIS_ScaleRuler::Compute(const Handle(PrsMgr_PresentationManager)&,
                             const Handle(Prs3d_Presentation)& thePrs,
                             const Standard_Integer            theMode)
{
  thePrs->SetInfiniteState(true);
  if (theMode != 0)
    return;

  const Graphic3d_Vec2i aSize = Graphic3d_Vec2i(Graphic3d_Vec2d(myBaseSize));
  const Graphic3d_Vec2i aDispSize(
    Graphic3d_Vec2d(myDispLenBase > 0.0f ? myDispLenBase : myBaseSize.x(), myBaseSize.y()));
  const Graphic3d_Vec2d aCornLeftBot = Graphic3d_Vec2d(getPrsCorner(false));

  // draw ruler line
  const double aThick = myBaseThick;
  const double aShadowOffset = 1.0 / myDispResRatio;
  for (int aStep = 0; aStep <= 1; ++aStep)
  {
    if (aStep == 0)
    {
      const Aspect_TypeOfDisplayText aTextStyle = myDrawer->TextAspect()->Aspect()->TextDisplayType();
      if (aTextStyle != Aspect_TODT_SHADOW && aTextStyle != Aspect_TODT_DEKALE)
        continue;

      myShadowAsp->SetColor(myDrawer->TextAspect()->Aspect()->ColorSubTitle());
    }

    Graphic3d_Vec2d aCorn = aCornLeftBot
                          + Graphic3d_Vec2d(aStep == 0 ? aShadowOffset : 0.0, aStep == 0 ? -aShadowOffset : 0.0);
    Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
    aGroup->SetGroupPrimitivesAspect(aStep == 0 ? myShadowAsp : myDrawer->LineAspect()->Aspect());
    aGroup->AddPrimitiveArray(createQuad(aCorn, Graphic3d_Vec2d(aDispSize.x(), aThick)));
    aGroup->AddPrimitiveArray(createQuad(aCorn, Graphic3d_Vec2d(aThick, aSize.y())));
    aGroup->AddPrimitiveArray(createQuad(aCorn + Graphic3d_Vec2d(Max(Standard_Real(aDispSize.x()) - aThick, 0.0), 0.0),
                                         Graphic3d_Vec2d(aThick, aSize.y())));
  }
  // draw label
  {
    Handle(Graphic3d_Text) aLabel = new Graphic3d_Text((float)myDrawer->TextAspect()->Height());
    aLabel->SetText(!myDispLenText.IsEmpty() ? myDispLenText : "??");
    aLabel->SetVerticalAlignment(Graphic3d_VTA_BOTTOM);

    const int    aShiftX = (int)Min(8.0, aSize.x() * 0.1);
    const double aShiftY = aThick * 3.0;

    gp_Pnt aTextPos = gp_XYZ(0.0, aCornLeftBot.y() + aShiftY, 0.0);
    if ((myTransformPersistence->Corner2d() & Aspect_TOTP_LEFT) != 0)
    {
      aTextPos.SetX(aCornLeftBot.x() + aShiftX);
      aLabel->SetHorizontalAlignment(Graphic3d_HTA_LEFT);
    }
    else if ((myTransformPersistence->Corner2d() & Aspect_TOTP_RIGHT) != 0)
    {
      aTextPos.SetX(aCornLeftBot.x() + aDispSize.x() - aShiftX);
      aLabel->SetHorizontalAlignment(Graphic3d_HTA_RIGHT);
    }
    else
    {
      aTextPos.SetX(aCornLeftBot.x() + aDispSize.x() / 2);
      aLabel->SetHorizontalAlignment(Graphic3d_HTA_CENTER);
    }
    aLabel->SetPosition(aTextPos);

    Handle(Graphic3d_Group) aGroup = thePrs->NewGroup();
    aGroup->SetGroupPrimitivesAspect(myDrawer->TextAspect()->Aspect());
    aGroup->AddText(aLabel);
  }
}

// ================================================================
// Function : ComputeSelection
// ================================================================
void AIS_ScaleRuler::ComputeSelection(const Handle(SelectMgr_Selection)& theSel, const Standard_Integer theMode)
{
  if (theMode != 0)
    return;

  Handle(SelectMgr_EntityOwner) anOwner = Handle(SelectMgr_EntityOwner)::DownCast(myOwner);
  if (anOwner.IsNull())
    anOwner = new SelectMgr_EntityOwner(this);

  // compute sensitivity for entire rectangle
  const Graphic3d_Vec2d aSize = Graphic3d_Vec2d(myBaseSize);
  const Graphic3d_Vec2d aCornLeftBot = Graphic3d_Vec2d(getPrsCorner(true));

  Handle(Graphic3d_ArrayOfTriangles)       aTris = createQuad(aCornLeftBot, aSize);
  Handle(Select3D_SensitivePrimitiveArray) aSens = new Select3D_SensitivePrimitiveArray(anOwner);
  aSens->InitTriangulation(aTris->Attributes(), aTris->Indices(), TopLoc_Location());
  theSel->Add(aSens);
}

// ================================================================
// Function : ProcessDragging
// ================================================================
Standard_Boolean AIS_ScaleRuler::ProcessDragging(const Handle(AIS_InteractiveContext)& theContext,
                                                 const Handle(V3d_View)&               theView,
                                                 const Handle(SelectMgr_EntityOwner)&,
                                                 const Graphic3d_Vec2i&,
                                                 const Graphic3d_Vec2i& theDragTo,
                                                 const AIS_DragAction   theAction)
{
  const double    aResRatio = theView->RenderingParams().ResolutionRatio();
  Graphic3d_Vec2i aScaledDragTo(theDragTo);
  aScaledDragTo.x() = int(double(aScaledDragTo.x()) / aResRatio);
  aScaledDragTo.y() = int(double(aScaledDragTo.y()) / aResRatio);
  switch (theAction)
  {
    case AIS_DragAction_Start:
    {
      return true;
    }
    case AIS_DragAction_Update:
    {
      const Handle(Prs3d_Drawer)& aStyle = theContext->HighlightStyle(Prs3d_TypeOfHighlight_LocalDynamic);
      const Graphic3d_ZLayerId aZLayer = aStyle->ZLayer() != Graphic3d_ZLayerId_UNKNOWN ? aStyle->ZLayer() : ZLayer();

      Handle(Prs3d_Presentation) aPrs = GetHilightPresentation(theContext->MainPrsMgr());
      aPrs->Clear();
      aPrs->SetIsForHighlight(true);
      if (aPrs->GetZLayer() != aZLayer)
        aPrs->SetZLayer(aZLayer);

      if (aPrs->TransformPersistence().IsNull() || aPrs->TransformPersistence() == myTransformPersistence)
      {
        aPrs->CStructure()->ViewAffinity = new Graphic3d_ViewAffinity();
        Handle(Graphic3d_TransformPers) aPers = new Graphic3d_TransformPers(Graphic3d_TMF_2d,
                                                                            Aspect_TOTP_LEFT_UPPER,
                                                                            aScaledDragTo);
        aPers->SetDensityIndependent(myTransformPersistence->IsDensityIndependent());
        aPrs->SetTransformPersistence(aPers);
      }
      aPrs->TransformPersistence()->SetOffset2d(aScaledDragTo);
      aPrs->CStructure()->ViewAffinity->SetVisible(false);
      aPrs->CStructure()->ViewAffinity->SetVisible(theView->View()->Identification(), true);

      theContext->MainSelector()->Pick(theDragTo.x(), theDragTo.y(), theView);
      ProcessRedraw(theContext, theView, AIS_RedrawProgress_BeforeRedraw);

      Compute(theContext->MainPrsMgr(), aPrs, 0);

      aPrs->Display();
      return true;
    }
    case AIS_DragAction_Stop:
    case AIS_DragAction_Abort:
    {
      Handle(Prs3d_Presentation) aPrs = GetHilightPresentation(nullptr);
      if (!aPrs.IsNull())
        aPrs->Erase();

      return true;
    }
    default: return false;
  }
}

//! Get 3D point for measuring scale in perspective projection
static gp_Pnt getMeasurePoint(const Handle(StdSelect_ViewerSelector3d)& theSelector,
                              const Handle(Graphic3d_Camera)&           theCam)
{
  for (int aPickIter = 1; aPickIter <= theSelector->NbPicked(); ++aPickIter)
  {
    Handle(SelectMgr_SelectableObject) aDetObj = theSelector->Picked(aPickIter)->Selectable();
    if (!aDetObj->TransformPersistence().IsNull() && aDetObj->TransformPersistence()->IsTrihedronOr2d())
      continue;

    const SelectMgr_SortCriterion& aPicked = theSelector->PickedData(aPickIter);
    if (!Precision::IsInfinite(aPicked.Point.X()))
      return aPicked.Point;
  }
  return theCam->Center();
}

//! Round off to closest integer / multiple of 10^n units.
static void findDispUnit(double&                        theLen,
                         TCollection_AsciiString&       theDispUnit,
                         const TCollection_AsciiString& theUnitFrom)
{
  for (int aUnitIter = 0; *LENGTH_UNITS[aUnitIter] != '\0'; ++aUnitIter)
  {
    const char* aUnitTo = LENGTH_UNITS[aUnitIter];
    double      aConvLen = Units::Convert(theLen, theUnitFrom.ToCString(), aUnitTo);
    if (aConvLen <= 1.0 && *LENGTH_UNITS[aUnitIter + 1] != '\0')
      continue;

    const double aLog10 = Log10(aConvLen);
    if (aLog10 >= 1.0)
    {
      const double aPow10 = Pow(10.0, Floor(aLog10));
      aConvLen = Floor(aConvLen / aPow10) * aPow10;
    }
    else if (aConvLen > 1.0)
    {
      aConvLen = Floor(aConvLen);
    }

    theLen = Units::Convert(aConvLen, aUnitTo, theUnitFrom.ToCString());

    theDispUnit = TCollection_AsciiString(aConvLen) + " " +aUnitTo;
    return;
  }
}

// ================================================================
// Function : ProcessRedraw
// ================================================================
AIS_RedrawProgressResult AIS_ScaleRuler::ProcessRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                                       const Handle(V3d_View)&               theView,
                                                       const AIS_RedrawProgress              theRedrawProgress)
{
  if (theRedrawProgress != AIS_RedrawProgress_BeforeRedraw)
    return AIS_RedrawProgressResult_None;

  AIS_RedrawProgressResult toUpdate = AIS_InteractiveObject::ProcessRedraw(theCtx, theView, theRedrawProgress);

  const Handle(Graphic3d_Camera)& aCam = theView->Camera();
  const double                    aResRatio = theView->RenderingParams().ResolutionRatio();

  const gp_Pnt aPnt = getMeasurePoint(theCtx->MainSelector(), aCam);
  const gp_Dir aForward = aCam->Direction();
  const double aDistFromEye = gp_Vec(aCam->Eye(), aPnt).XYZ().Dot(aForward.XYZ());
  const double aFocus = aCam->IsOrthographic() ? aCam->Distance() : aDistFromEye;
  const gp_XYZ aViewDim = aCam->ViewDimensions(aFocus);
  // scale factor to pixels
  const Graphic3d_Vec2d aWinDims(theView->Window()->Dimensions());
  const double          aPixelScale = Abs(aViewDim.Y()) / double(aWinDims.y());

  TCollection_AsciiString aDispUnit;
  double                  aLen = myBaseSize.x() * aPixelScale * aResRatio;
  findDispUnit(aLen, aDispUnit, myDrawer->DimLengthModelUnits());

  const double aLenPx = aLen / (aPixelScale * aResRatio);
  if (myDispLenText == aDispUnit && myDispLenBase == aLenPx)
    return toUpdate;

  myDispLenBase = aLenPx;
  myDispLenSys = aLen;
  myDispLenText = aDispUnit;
  myDispResRatio = aResRatio;

  // recompute presentation only (avoid selection changes)
  InteractiveContext()->RecomputePrsOnly(this, false, true);
  if (theView->Viewer()->ZLayerSettings(myDrawer->ZLayer()).IsImmediate())
    theView->InvalidateImmediate();
  else
    theView->Invalidate();

  return toUpdate;
}
