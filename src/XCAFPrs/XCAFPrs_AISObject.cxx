// Created on: 2000-08-11
// Created by: Andrey BETENEV
// Copyright (c) 2000-2014 OPEN CASCADE SAS
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

#include <XCAFPrs_AISObject.hxx>

#include <BRep_Builder.hxx>
#include <BRepBndLib.hxx>
#include <BRepTools.hxx>
#include <gp_Pnt.hxx>
#include <Graphic3d_AspectFillArea3d.hxx>
#include <Prs3d_Drawer.hxx>
#include <Prs3d_DimensionAspect.hxx>
#include <Prs3d_IsoAspect.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Prs3d_ShadingAspect.hxx>
#include <Prs3d_Text.hxx>
#include <StdPrs_ToolTriangulatedShape.hxx>
#include <StdSelect_BRepSelectionTool.hxx>
#include <TDataStd_Name.hxx>
#include <TPrsStd_AISPresentation.hxx>
#include <TopoDS_Iterator.hxx>
#include <XCAFDoc_ShapeTool.hxx>
#include <XCAFPrs.hxx>
#include <XCAFPrs_IndexedDataMapOfShapeStyle.hxx>
#include <XCAFPrs_Style.hxx>

IMPLEMENT_STANDARD_RTTIEXT(XCAFPrs_AISObject,AIS_ColoredShape)
IMPLEMENT_STANDARD_RTTIEXT(XCAFPrs_AISDrawer,AIS_ColoredDrawer)
IMPLEMENT_STANDARD_RTTIEXT(XCAFPrs_BRepOwner,StdSelect_BRepOwner)

//=======================================================================
//function : XCAFPrs_AISObject
//purpose  :
//=======================================================================
XCAFPrs_AISObject::XCAFPrs_AISObject() : AIS_ColoredShape(TopoDS_Shape())
{
  // define plastic material by default for proper color reproduction
  setMaterial(myDrawer, Graphic3d_NameOfMaterial_Plastified, Standard_False, Standard_False);
  hasOwnMaterial = Standard_True;
}

//=======================================================================
//function : XCAFPrs_AISObject
//purpose  :
//=======================================================================
XCAFPrs_AISObject::XCAFPrs_AISObject(const TDF_Label& theLabel) : XCAFPrs_AISObject()
{
  SetLabel(theLabel);
}

//=======================================================================
//function : XCAFPrs_AISObject
//purpose  :
//=======================================================================
XCAFPrs_AISObject::XCAFPrs_AISObject(const NCollection_List<Handle(XCAFPrs_BRepOwner)>& theLabels) : XCAFPrs_AISObject()
{
  myLabels = theLabels;
}

//=======================================================================
//function : GetLabel
//purpose  :
//=======================================================================
const TDF_Label& XCAFPrs_AISObject::GetLabel() const
{
  if (myLabels.Size() != 1)
  {
    static const TDF_Label aNullLab;
    return aNullLab;
  }
  return myLabels.First()->ShapeLabel();
}

//=======================================================================
//function : SetLabels
//purpose  :
//=======================================================================
void XCAFPrs_AISObject::SetLabels(const NCollection_List<Handle(XCAFPrs_BRepOwner)>& theLabels)
{
  myLabels = theLabels;
}

//=======================================================================
//function : SetLabel
//purpose  :
//=======================================================================
void XCAFPrs_AISObject::SetLabel(const TDF_Label& theLabel)
{
  myLabels.Clear();

  Handle(XCAFPrs_BRepOwner) aBrepOwner = new XCAFPrs_BRepOwner(theLabel, TopLoc_Location());
  myLabels.Append(aBrepOwner);
}

//=======================================================================
//function : DisplayText
//purpose  : 
//=======================================================================

static void DisplayText (const TDF_Label& aLabel,
			 const Handle(Prs3d_Presentation)& aPrs,
			 const Handle(Prs3d_TextAspect)& anAspect,
			 const TopLoc_Location& aLocation)
{
  // first label itself
  Handle (TDataStd_Name) aName;
  if (aLabel.FindAttribute (TDataStd_Name::GetID(), aName)) {
    TopoDS_Shape aShape;
    if (XCAFDoc_ShapeTool::GetShape (aLabel, aShape)) {
      // find the position to display as middle of the bounding box
      aShape.Move (aLocation);
      Bnd_Box aBox;
      BRepBndLib::Add (aShape, aBox);
      if ( ! aBox.IsVoid() ) 
      {
	Standard_Real aXmin, aYmin, aZmin, aXmax, aYmax, aZmax;
	aBox.Get (aXmin, aYmin, aZmin, aXmax, aYmax, aZmax);
	gp_Pnt aPnt (0.5 * (aXmin + aXmax), 0.5 * (aYmin + aYmax), 0.5 * (aZmin + aZmax));
	Prs3d_Text::Draw (aPrs->CurrentGroup(), anAspect, aName->Get(), aPnt);
      }
    }
  }

  TDF_LabelSequence seq;
  
  // attributes of subshapes
  if (XCAFDoc_ShapeTool::GetSubShapes (aLabel, seq)) {
    Standard_Integer i = 1;
    for (i = 1; i <= seq.Length(); i++) {
      TDF_Label aL = seq.Value (i);
      DisplayText (aL, aPrs, anAspect, aLocation); //suppose that subshapes do not contain locations
    }
  }
  
  // attributes of components
  seq.Clear();
  if (XCAFDoc_ShapeTool::GetComponents (aLabel, seq)) {
    Standard_Integer i = 1;
    for (i = 1; i <= seq.Length(); i++) {
      TDF_Label aL = seq.Value (i);
      DisplayText (aL, aPrs, anAspect, aLocation);
      TDF_Label aRefLabel;
      
      // attributes of references
      TopLoc_Location aLoc = XCAFDoc_ShapeTool::GetLocation (aL);
      if (XCAFDoc_ShapeTool::GetReferredShape (aL, aRefLabel)) {
	DisplayText (aRefLabel, aPrs, anAspect, aLoc);
      }
    }
  }
}

// =======================================================================
// function : XCAFPrs_AISDrawer
// purpose  :
// =======================================================================
XCAFPrs_AISDrawer::XCAFPrs_AISDrawer(const Handle(Prs3d_Drawer)& theLink,
                                     const XCAFPrs_Style&        theStyle,
                                     const XCAFPrs_Style&        theDefStyle)
: AIS_ColoredDrawer(theLink),
  myStyle(theStyle)
{
  if (myStyle.Material().IsNull() && !theDefStyle.Material().IsNull())
    myStyle.SetMaterial(theDefStyle.Material());

  if (myStyle.Material().IsNull() && !myStyle.IsSetColorSurf() && theDefStyle.IsSetColorSurf())
    myStyle.SetColorSurf(theDefStyle.GetColorSurfRGBA());

  if (myStyle.Material().IsNull() && !myStyle.IsSetColorCurv() && theDefStyle.IsSetColorCurv())
    myStyle.SetColorCurv(theDefStyle.GetColorCurv());
}

// =======================================================================
// function : UpdateStyle
// purpose  :
// =======================================================================
void XCAFPrs_AISDrawer::UpdateStyle(Prs3d_Drawer&               theDrawer,
                                    const XCAFPrs_Style&        theStyle,
                                    const Handle(Prs3d_Drawer)& theHiDrawer)
{
  if (AIS_ColoredDrawer* aColDrawer = dynamic_cast<AIS_ColoredDrawer*>(&theDrawer))
  {
    aColDrawer->SetHidden(!theStyle.IsVisible());
    if (!theStyle.Material().IsNull() && !theStyle.Material()->IsEmpty())
    {
      aColDrawer->SetOwnMaterial();
    }
    if (theStyle.IsSetColorSurf() || theStyle.IsSetColorCurv())
    {
      aColDrawer->SetOwnColor(Quantity_Color());
    }
  }

  theDrawer.SetupOwnShadingAspect();
  theDrawer.SetOwnLineAspects();

  Quantity_ColorRGBA       aSurfColor = Quantity_ColorRGBA(Quantity_NOC_WHITE, 1.0f);
  Quantity_Color           aCurvColor = Quantity_NOC_WHITE;
  Graphic3d_MaterialAspect aMatFront = theDrawer.ShadingAspect()->Aspect()->FrontMaterial();

  const Handle(XCAFDoc_VisMaterial)& anXMat = theStyle.Material();
  if (!anXMat.IsNull() && !anXMat->IsEmpty())
  {
    anXMat->FillAspect(theDrawer.ShadingAspect()->Aspect());
    aMatFront = theDrawer.ShadingAspect()->Aspect()->FrontMaterial();
    aSurfColor = Quantity_ColorRGBA(aMatFront.Color(), aMatFront.Alpha());
    aCurvColor = aMatFront.Color();
  }
  if (theStyle.IsSetColorSurf())
  {
    aSurfColor = theStyle.GetColorSurfRGBA();
    aMatFront.SetColor(aSurfColor.GetRGB());
    aMatFront.SetAlpha(aSurfColor.Alpha());
  }
  if (theStyle.IsSetColorCurv())
    aCurvColor = theStyle.GetColorCurv();

  if (theStyle.IsHighlighted() && !theHiDrawer.IsNull())
  {
    aSurfColor = theHiDrawer->ColorRGBA();
    aCurvColor = theHiDrawer->Color();
    if (!theHiDrawer->BasicFillAreaAspect().IsNull())
      aMatFront = theHiDrawer->BasicFillAreaAspect()->FrontMaterial();
    else
      aMatFront.SetColor(aSurfColor.GetRGB());
  }

  theDrawer.UnFreeBoundaryAspect()->SetColor(aCurvColor);
  theDrawer.FreeBoundaryAspect()->SetColor(aCurvColor);
  theDrawer.WireAspect()->SetColor(aCurvColor);

  theDrawer.ShadingAspect()->Aspect()->SetInteriorColor(aSurfColor);
  theDrawer.ShadingAspect()->Aspect()->SetFrontMaterial(aMatFront);
  theDrawer.UIsoAspect()->SetColor(aSurfColor.GetRGB());
  theDrawer.VIsoAspect()->SetColor(aSurfColor.GetRGB());
}

//=======================================================================
//function : DispatchStyles
//purpose  :
//=======================================================================
void XCAFPrs_AISObject::DispatchStyles (const Standard_Boolean theToSyncStyles)
{
  myToSyncStyles = theToSyncStyles;
  myShapeColors.Clear();

  // Getting default colors
  XCAFPrs_Style aDefStyle;
  DefaultStyle(aDefStyle);
  XCAFPrs_AISDrawer::UpdateStyle(*myDrawer, aDefStyle, Handle(Prs3d_Drawer)());

  NCollection_IndexedDataMap<XCAFPrs_Style, TopoDS_Compound, XCAFPrs_Style> aStyleGroups;

  TopoDS_Compound aWholeComp;
  BRep_Builder().MakeCompound(aWholeComp);
  XCAFPrs_IndexedDataMapOfShapeStyle aSettings;
  for (const Handle(XCAFPrs_BRepOwner)& aLabIter : myLabels)
  {
    aLabIter->UpdateShape();
    if (aLabIter->Shape().IsNull())
      continue;

    BRep_Builder().Add(aWholeComp, aLabIter->Shape());

    // collect information on colored subshapes
    XCAFPrs::CollectStyleSettings(aLabIter->ShapeLabel(), aLabIter->ParentLocation(), aSettings);

    // collect sub-shapes with the same style into compounds
    bool hasLabStyle = false;
    for (const std::pair<const TopoDS_Shape, XCAFPrs_Style>& aStyledShapeIter : aSettings)
    {
      const TopoDS_Shape& aSubshape = aStyledShapeIter.first;
      hasLabStyle = hasLabStyle || aSubshape == aLabIter->Shape();

      XCAFPrs_Style aSubStyle = aStyledShapeIter.second;
      aSubStyle.SetHighlighted(aLabIter->IsSelected());

      TopoDS_Compound aComp;
      if (aStyleGroups.FindFromKey(aSubStyle, aComp))
      {
        BRep_Builder().Add(aComp, aSubshape);
        continue;
      }

      BRep_Builder().MakeCompound(aComp);
      BRep_Builder().Add(aComp, aSubshape);
      TopoDS_Compound* aMapShape = aStyleGroups.ChangeSeek(aSubStyle);
      if (aMapShape == nullptr)
        aStyleGroups.Add(aSubStyle, aComp);
      else
        *aMapShape = aComp;
    }
    aSettings.Clear(false);

    // map label to default style
    if (!hasLabStyle)
    {
      XCAFPrs_Style aLabStyle;
      aLabStyle.SetHighlighted(aLabIter->IsSelected());

      TopoDS_Compound aComp;
      if (aStyleGroups.FindFromKey(aLabStyle, aComp))
      {
        BRep_Builder().Add(aComp, aLabIter->Shape());
      }
      else
      {
        BRep_Builder().MakeCompound(aComp);
        BRep_Builder().Add(aComp, aLabIter->Shape());
        TopoDS_Compound* aMapShape = aStyleGroups.ChangeSeek(aLabStyle);
        if (aMapShape == nullptr)
          aStyleGroups.Add(aLabStyle, aComp);
        else
          *aMapShape = aComp;
      }
    }
  }

  // assign custom aspects
  for (const std::pair<const XCAFPrs_Style, TopoDS_Compound>& aStyleGroupIter : aStyleGroups)
  {
    const TopoDS_Compound& aComp = aStyleGroupIter.second;
    TopoDS_Iterator        aShapeIter(aComp);
    TopoDS_Shape           aShapeCur = aShapeIter.Value();
    aShapeIter.Next();
    if (aShapeIter.More())
      aShapeCur = aComp;

    Handle(XCAFPrs_AISDrawer) aDrawer = new XCAFPrs_AISDrawer(myDrawer, aStyleGroupIter.first, aDefStyle);
    myShapeColors.Bind(aShapeCur, aDrawer);
  }
  aStyleGroups.Clear();

  if (aWholeComp.NbChildren() == 0)
  {
    myshape = TopoDS_Shape();
    myCompBB = true;
    return;
  }
  else if (aWholeComp.NbChildren() == 1)
  {
    myshape = TopoDS_Iterator(aWholeComp).Value();
    myCompBB = true;
  }
  else
  {
    myshape = aWholeComp;
    myCompBB = true;
  }

  // synchronize highlighting state
  for (const Handle(XCAFPrs_BRepOwner)& aLabIter : myLabels)
    aLabIter->SetHighlightedInPrs(aLabIter->IsSelected());
}

//=======================================================================
//function : Compute
//purpose  :
//=======================================================================
void XCAFPrs_AISObject::Compute (const Handle(PrsMgr_PresentationManager)& thePresentationManager,
                                 const Handle(Prs3d_Presentation)& thePrs,
                                 const Standard_Integer theMode)
{
  // update shape and sub-shapes styles only on first compute, or on first recompute
  if (myToSyncStyles)
  {
    Standard_Boolean toMapStyles = myToSyncStyles;
    for (PrsMgr_Presentations::Iterator aPrsIter (myPresentations); aPrsIter.More(); aPrsIter.Next())
    {
      if (aPrsIter.Value() != thePrs && !aPrsIter.Value()->MustBeUpdated())
      {
        toMapStyles = Standard_False;
        break;
      }
    }
    if (toMapStyles)
      DispatchStyles (Standard_True);
  }
  if (myshape.IsNull())
    return;
  else if (myshape.ShapeType() == TopAbs_COMPOUND && myshape.NbChildren() == 0)
    return;

  SynchronizeAspects();
  AIS_ColoredShape::Compute (thePresentationManager, thePrs, theMode);

  if (XCAFPrs::GetViewNameMode())
  {
    // Displaying Name attributes
    thePrs->SetDisplayPriority (Graphic3d_DisplayPriority_Topmost);
    for (const Handle(XCAFPrs_BRepOwner)& aLabIter : myLabels)
      DisplayText (aLabIter->ShapeLabel(), thePrs, Attributes()->DimensionAspect()->TextAspect(), TopLoc_Location());//no location
  }
}

// ================================================================
// Function : ProcessRedraw
// ================================================================
AIS_RedrawProgressResult XCAFPrs_AISObject::ProcessRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                                          const Handle(V3d_View)&               theView,
                                                          const AIS_RedrawProgress              theRedrawProgress)
{
  AIS_RedrawProgressResult aRes = base_type::ProcessRedraw(theCtx, theView, theRedrawProgress);
  if (theRedrawProgress != AIS_RedrawProgress_BeforeRedraw)
    return aRes;
  else if (!myToRehighlight)
    return aRes;

  myToRehighlight = false;
  int  aNbSel = 0;
  bool toReHighlight = false;
  for (const Handle(XCAFPrs_BRepOwner)& aLabIter : myLabels)
  {
    if (aLabIter->IsSelected())
      ++aNbSel;

    if (aLabIter->IsSelected() != aLabIter->IsHighlightedInPrs())
    {
      aLabIter->SetHighlightedInPrs(aLabIter->IsSelected());
      toReHighlight = true;
    }
  }
  if (!toReHighlight)
    return aRes;

  if (aNbSel == 0 || aNbSel == myLabels.Size())
  {
    for (AIS_DataMapOfShapeDrawer::Iterator anIter(myShapeColors); anIter.More(); anIter.Next())
    {
      if (XCAFPrs_AISDrawer* aDrawer = dynamic_cast<XCAFPrs_AISDrawer*>(anIter.Value().get()))
        aDrawer->SetHighlighted(aNbSel != 0);
    }
    for (const Handle(XCAFPrs_BRepOwner)& aLabIter : myLabels)
      aLabIter->SetHighlightedInPrs(aLabIter->IsSelected());

    SynchronizeAspects();
    return aRes;
  }

  DispatchStyles(myToSyncStyles);
  return AIS_RedrawProgressResult_NeedRedisplay;
}

//=======================================================================
//function : GlobalSelOwner
//purpose  :
//=======================================================================
Handle(SelectMgr_EntityOwner) XCAFPrs_AISObject::GlobalSelOwner() const
{
  if (myLabels.Size() == 1)
  {
    Handle(XCAFPrs_BRepOwner) anOwner = myLabels.First();
    return anOwner;
  }
  if (!myLabels.IsEmpty() && !myLabels.First()->ComesFromDecomposition())
    return base_type::GlobalSelOwner();

  return Handle(SelectMgr_EntityOwner)();
}

//=======================================================================
//function : ComputeSelection
//purpose  :
//=======================================================================
void XCAFPrs_AISObject::ComputeSelection(const Handle(SelectMgr_Selection)& theSel, const Standard_Integer theMode)
{
  if (myshape.IsNull())
    return;

  const TopAbs_ShapeEnum aTypOfSel = AIS_Shape::SelectionType(theMode);
  const Standard_Integer aPriority = StdSelect_BRepSelectionTool::GetStandardPriority(myshape, aTypOfSel);
  if (aTypOfSel != TopAbs_SHAPE)
  {
    base_type::ComputeSelection(theSel, theMode);
    return;
  }

  if (myLabels.IsEmpty() || (myLabels.Size() != 1 && !myLabels.First()->ComesFromDecomposition()))
  {
    base_type::ComputeSelection(theSel, theMode);
    return;
  }

  const Standard_Real aDeflection = StdPrs_ToolTriangulatedShape::GetDeflection(myshape, myDrawer);
  const Standard_Real aDeviationAngle = myDrawer->DeviationAngle();
  if (myDrawer->IsAutoTriangulation() && !BRepTools::Triangulation(myshape, Precision::Infinite()))
    StdPrs_ToolTriangulatedShape::Tessellate(myshape, myDrawer, aDeflection);

  AIS_DataMapOfShapeDrawer aSubshapeDrawerMap;
  fillSubshapeDrawerMap(aSubshapeDrawerMap);

  Handle(AIS_ColoredDrawer) aBaseDrawer;
  myShapeColors.Find(myshape, aBaseDrawer);
  for (const Handle(XCAFPrs_BRepOwner)& aLabIter : myLabels)
  {
    if (aLabIter->Shape().IsNull())
      continue;

    computeSubshapeSelection(aBaseDrawer,
                             aSubshapeDrawerMap,
                             aLabIter->Shape(),
                             aLabIter,
                             theSel,
                             aTypOfSel,
                             aPriority,
                             aDeflection,
                             aDeviationAngle);
  }

  for (const Handle(SelectMgr_SensitiveEntity)& aSelEntIter : theSel->Entities())
  {
    const Handle(SelectMgr_EntityOwner)& anOwner = aSelEntIter->BaseSensitive()->OwnerId();
    anOwner->SetSelectablePointer(this);
  }
}

//=======================================================================
//function : DefaultStyle
//purpose  : DefaultStyle() can be redefined by subclasses in order to set custom default style
//=======================================================================
void XCAFPrs_AISObject::DefaultStyle (XCAFPrs_Style& theStyle) const
{
  theStyle.SetColorSurf (Quantity_NOC_WHITE);
  theStyle.SetColorCurv (Quantity_NOC_WHITE);
}

// =======================================================================
// function : SetMaterial
// purpose  :
// =======================================================================
void XCAFPrs_AISObject::SetMaterial (const Graphic3d_MaterialAspect& theMaterial)
{
  XCAFPrs_Style aDefStyle;
  DefaultStyle (aDefStyle);
  setMaterial (myDrawer, theMaterial, HasColor(), IsTransparent());
  XCAFPrs_AISDrawer::UpdateStyle(*myDrawer, aDefStyle, Handle(Prs3d_Drawer)());
  for (AIS_DataMapOfShapeDrawer::Iterator anIter (myShapeColors); anIter.More(); anIter.Next())
  {
    const Handle(AIS_ColoredDrawer)& aDrawer = anIter.Value();
    if (aDrawer->HasOwnMaterial())
    {
      continue;
    }

    if (aDrawer->HasOwnShadingAspect())
    {
      // take current color
      const Quantity_ColorRGBA aSurfColor = aDrawer->ShadingAspect()->Aspect()->InteriorColorRGBA();
      Graphic3d_MaterialAspect aMaterial = myDrawer->ShadingAspect()->Aspect()->FrontMaterial();
      aMaterial.SetColor (aSurfColor.GetRGB());
      aMaterial.SetAlpha (aSurfColor.Alpha());
      aDrawer->ShadingAspect()->Aspect()->SetInteriorColor (aSurfColor);
      aDrawer->ShadingAspect()->Aspect()->SetFrontMaterial (aMaterial);
    }
  }
  SynchronizeAspects();
}

// =======================================================================
// function : ToHighlightInMainPrs
// purpose  :
// =======================================================================
bool XCAFPrs_AISObject::ToHighlightInMainPrs() const
{
  const Handle(Prs3d_Drawer)& aSelDrawer = !myHilightDrawer.IsNull() || !HasInteractiveContext()
                                           ? myHilightDrawer
                                           : InteractiveContext()->HighlightStyle(Prs3d_TypeOfHighlight_Selected);
  const Standard_Integer      aDispMode = GetAcceptedDisplayMode();
  return !aSelDrawer.IsNull() && (aSelDrawer->DisplayMode() == -1 || aSelDrawer->DisplayMode() == aDispMode);
}

// =======================================================================
// function : SynchronizeAspects
// purpose  :
// =======================================================================
void XCAFPrs_AISObject::SynchronizeAspects()
{
  Handle(Prs3d_Drawer) aSelDrawer;
  if (ToHighlightInMainPrs())
    aSelDrawer = !myHilightDrawer.IsNull() || !HasInteractiveContext()
                 ? myHilightDrawer
                 : InteractiveContext()->HighlightStyle(Prs3d_TypeOfHighlight_Selected);

  for (AIS_DataMapOfShapeDrawer::Iterator anIter(myShapeColors); anIter.More(); anIter.Next())
  {
    if (XCAFPrs_AISDrawer* aDrawer = dynamic_cast<XCAFPrs_AISDrawer*>(anIter.Value().get()))
      aDrawer->UpdateStyle(aSelDrawer);
  }
  base_type::SynchronizeAspects();
}

// =======================================================================
// function : XCAFPrs_BRepOwner
// purpose :
// =======================================================================
XCAFPrs_BRepOwner::XCAFPrs_BRepOwner(const TDF_Label&       theLabel,
                                     const TopLoc_Location& theLoc,
                                     const XCAFPrs_Style&   theStyle)
: XCAFPrs_BRepOwner(XCAFPrs_LabelPath{theLabel}, theLoc, theStyle)
{
  //
}

// =======================================================================
// function : XCAFPrs_BRepOwner
// purpose :
// =======================================================================
XCAFPrs_BRepOwner::XCAFPrs_BRepOwner(const XCAFPrs_LabelPath& thePath,
                                     const TopLoc_Location&   theLoc,
                                     const XCAFPrs_Style&     theStyle)
: StdSelect_BRepOwner(0),
  myShapePath(thePath),
  myParentLoc(theLoc),
  myStyle(theStyle)
{
  UpdateShape();
  if (!myShape.IsNull())
    mypriority = StdSelect_BRepSelectionTool::GetStandardPriority(myShape, TopAbs_SHAPE);
}

// =======================================================================
// function : UpdateShape
// purpose :
// =======================================================================
void XCAFPrs_BRepOwner::UpdateShape()
{
  TopoDS_Shape aShape;
  if (!myShapePath.empty() && !myShapePath.back().IsNull())
    XCAFDoc_ShapeTool::GetShape(myShapePath.back(), aShape);

  if (!aShape.IsNull() && !myParentLoc.IsIdentity())
    aShape.Move(myParentLoc, false);

  myShape = aShape;
}

// =======================================================================
// function : HilightWithColor
// purpose :
// =======================================================================
void XCAFPrs_BRepOwner::HilightWithColor(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                         const Handle(Prs3d_Drawer)& theStyle,
                                         const Standard_Integer theMode)
{
  if (XCAFPrs_AISObject* aPrsObj = dynamic_cast<XCAFPrs_AISObject*>(mySelectable))
  {
    if (!thePrsMgr->IsImmediateModeOn() && aPrsObj->ToHighlightInMainPrs())
    {
      aPrsObj->InvalidateHighlightInMainPrs();
      return;
    }
  }

  base_type::HilightWithColor(thePrsMgr, theStyle, theMode);
}

// =======================================================================
// function : Unhilight
// purpose :
// =======================================================================
void XCAFPrs_BRepOwner::Unhilight(const Handle(PrsMgr_PresentationManager)& thePrsMgr, const Standard_Integer theMode)
{
  if (XCAFPrs_AISObject* aPrsObj = dynamic_cast<XCAFPrs_AISObject*>(mySelectable))
  {
    if (!thePrsMgr->IsImmediateModeOn() && aPrsObj->ToHighlightInMainPrs())
      aPrsObj->InvalidateHighlightInMainPrs();
  }
  base_type::Unhilight(thePrsMgr, theMode);
}

// =======================================================================
// function : Clear
// purpose :
// =======================================================================
void XCAFPrs_BRepOwner::Clear(const Handle(PrsMgr_PresentationManager)& thePrsMgr, const Standard_Integer theMode)
{
  if (XCAFPrs_AISObject* aPrsObj = dynamic_cast<XCAFPrs_AISObject*>(mySelectable))
  {
    if (!thePrsMgr->IsImmediateModeOn() && aPrsObj->ToHighlightInMainPrs())
      aPrsObj->InvalidateHighlightInMainPrs();
  }
  base_type::Clear(thePrsMgr, theMode);
}

// =======================================================================
// function : FormatName
// purpose :
// =======================================================================
TCollection_AsciiString XCAFPrs_BRepOwner::FormatName(RWMesh_NameFormat theFormat, bool theFullPath) const
{
  if (myShapePath.empty())
    return TCollection_AsciiString();

  TCollection_AsciiString aPath;
  for (size_t aLabIter = theFullPath ? 0 : myShapePath.size() - 1; aLabIter < myShapePath.size(); ++aLabIter)
  {
    const TDF_Label&        aLabel = myShapePath[aLabIter];
    TCollection_AsciiString aName = XCAFDoc_ShapeTool::FormatName(theFormat, aLabel);
    aPath += aName;
    if (aLabIter + 1 < myShapePath.size())
      aPath += "/";
  }
  return aPath;
}

// =======================================================================
// function : ToString
// purpose :
// =======================================================================
TCollection_AsciiString XCAFPrs_BRepOwner::ToString() const
{
  return !myName.IsEmpty() ? myName : base_type::ToString();
}
