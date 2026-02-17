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

#ifndef _AIS_ClippingPlanes_HeaderFile
#define _AIS_ClippingPlanes_HeaderFile

#include <AIS_InteractiveObject.hxx>
#include <Graphic3d_SequenceOfHClipPlane.hxx>
#include <Prs3d_DatumMode.hxx>
#include <Prs3d_LineAspect.hxx>
#include <Prs3d_ShadingAspect.hxx>
#include <SelectMgr_EntityOwner.hxx>

class Graphic3d_ArrayOfPolylines;
class Graphic3d_ArrayOfSegments;
class Graphic3d_ArrayOfTriangles;

//! Widget for managing clipping plane(s).
class AIS_ClippingPlanes : public AIS_InteractiveObject
{
  DEFINE_STANDARD_RTTIEXT(AIS_ClippingPlanes, AIS_InteractiveObject)
public:

  //! Return default presentation color.
  static Quantity_Color DefaultPlaneColor() { return Quantity_Color(0.0, 0.4, 0.6, Quantity_TOC_RGB); }

  //! Create clipping planes from bounding box.
  Standard_EXPORT static Handle(Graphic3d_SequenceOfHClipPlane) GetClippingBndBox(const Bnd_Box& theBox);

  //! Clip the polygon by clipping planes.
  //! @param[in] thePol        polygon defining clipping plane contour (e.g. quad)
  //! @param[in] theClipPlanes clipping planes configuration to clip polygon
  //! @param[in] theDispPlane  plane to display, used to define input polygon
  Standard_EXPORT static void ClipPolygonByPlanes(NCollection_Sequence<gp_Pnt>&                 thePol,
                                                  const Handle(Graphic3d_SequenceOfHClipPlane)& theClipPlanes,
                                                  const Handle(Graphic3d_ClipPlane)&            theDispPlane);

  //! Return sequence with 4 points defining clipping plane quad contour.
  //! @param[in] theBox   bounding box to clip/center plane
  //! @param[in] thePlane plane to display
  Standard_EXPORT static NCollection_Sequence<gp_Pnt> GetPlaneQuadNodes(const Bnd_Box& theBox, const gp_Pln& thePlane);

  //! Return sequence with 4 points defining clipping plane quad contour.
  //! @param[in] theBndPlanes  bounding box clipping planes
  //! @param[in] theBndSegs segments of bounding box to clip/center plane, defined by Prs3d_BndBox::FillSegments()
  //! @param[in] thePlane   plane to display
  Standard_EXPORT static NCollection_Sequence<gp_Pnt> GetPlaneQuadNodes(
    const Handle(Graphic3d_SequenceOfHClipPlane)& theBndPlanes,
    const Handle(Graphic3d_ArrayOfSegments)&      theBndSegs,
    const gp_Pln&                                 thePlane);

  //! Return triangulated plane, clipped by other clipping planes.
  //! @param[in] theBndPlanes  bounding box clipping planes
  //! @param[in] theBndSegs    segments of bounding box to clip/center plane, defined by Prs3d_BndBox::FillSegments()
  //! @param[in] theClipPlanes clipping planes configuration to clip polygon
  //! @param[in] theDispPlane  plane to display, used to define input polygon
  //! @param[in] theOffset     offset along plane normal
  Standard_EXPORT static Handle(Graphic3d_ArrayOfTriangles) GetPlaneTriangles(
    const Handle(Graphic3d_SequenceOfHClipPlane)& theBndPlanes,
    const Handle(Graphic3d_ArrayOfSegments)&      theBndSegs,
    const Handle(Graphic3d_SequenceOfHClipPlane)& theClipPlanes,
    const Handle(Graphic3d_ClipPlane)&            theDispPlane,
    const double                                  theOffset = 0.0);

  //! Return plane contour edges, clipped by other clipping planes.
  //! @param[in] theBndPlanes  bounding box clipping planes
  //! @param[in] theBndSegs    segments of bounding box to clip/center plane, defined by Prs3d_BndBox::FillSegments()
  //! @param[in] theClipPlanes clipping planes configuration to clip polygon
  //! @param[in] theDispPlane  plane to display, used to define input polygon
  //! @param[in] theOffset     offset along plane normal
  Standard_EXPORT static Handle(Graphic3d_ArrayOfPolylines) GetPlaneEdges(
    const Handle(Graphic3d_SequenceOfHClipPlane)& theBndPlanes,
    const Handle(Graphic3d_ArrayOfSegments)&      theBndSegs,
    const Handle(Graphic3d_SequenceOfHClipPlane)& theClipPlanes,
    const Handle(Graphic3d_ClipPlane)&            theDispPlane,
    const double                                  theOffset = 0.0);

public:

  //! Empty constructor.
  Standard_EXPORT AIS_ClippingPlanes();

  //! Return clipping planes to manage.
  const Handle(Graphic3d_SequenceOfHClipPlane)& ManagedPlanes() const { return myManagedPlanes; }

  //! Set clipping planes to manage.
  void SetManagedPlanes(const Handle(Graphic3d_SequenceOfHClipPlane)& thePlanes)
  {
    myManagedPlanes = thePlanes;
    SetToUpdate();
  }

  //! Return clipping plane to display or NULL if all managed planes in sequence should be displayed.
  const Handle(Graphic3d_ClipPlane)& DisplayedPlane() const { return myDispPlane; }

  //! Set clipping plane to display.
  void SetDisplayedPlane(const Handle(Graphic3d_ClipPlane)& thePlane)
  {
    myDispPlane = thePlane;
    SetToUpdate();
  }

  //! Return bounding box to clip presentation or void box for auto-sizing.
  const Bnd_Box& GetSceneBox() const { return mySceneBox; }

  //! Set bounding box to clip presentation or void box for auto-sizing.
  void SetSceneBox(const Bnd_Box& theBox)
  {
    mySceneBox = theBox;
    SetToUpdate();
  }

  //! Return clipping planes from scene bounding box.
  const Handle(Graphic3d_SequenceOfHClipPlane)& BndBoxPlanes() const { return myBndBoxPlanes; }

  //! Return to display shaded/edges presentation; Prs3d_DM_Shaded by default.
  Prs3d_DatumMode DatumDisplayMode() const { return myDatumDispMode; }

  //! Set to display shaded presentation.
  void SetDatumDisplayMode(Prs3d_DatumMode theMode)
  {
    if (myDatumDispMode != theMode)
    {
      myDatumDispMode = theMode;
      SetToUpdate();
    }
  }

  //! Return TRUE to display points presentation; FALSE by default.
  bool ToDisplayPoints() const { return myToDisplayPoints; }

  //! Set to display points presentation.
  void SetDisplayPoints(bool theToDisplay)
  {
    if (myToDisplayPoints != theToDisplay)
    {
      myToDisplayPoints = theToDisplay;
      SetToUpdate();
    }
  }

public:

  //! Returns the transparency.
  virtual Standard_Real Transparency() const override { return myDrawer->ShadingAspect()->Transparency(); }

  //! Sets global transparency.
  virtual void SetTransparency(const Standard_Real theValue) override
  {
    myDrawer->ShadingAspect()->SetTransparency(theValue);
  }

  //! Resets global transparency.
  virtual void UnsetTransparency() override { myDrawer->ShadingAspect()->SetTransparency(0.0f); }

  //! Returns global color.
  virtual void Color(Quantity_Color& theColor) const override { theColor = myDrawer->ShadingAspect()->Color(); }

  //! Sets global color.
  virtual void SetColor(const Quantity_Color& theColor) override
  {
    myDrawer->ShadingAspect()->SetColor(theColor);
    myDrawer->FaceBoundaryAspect()->SetColor(theColor);
  }

  //! Reset global color.
  virtual void UnsetColor() override { SetColor(DefaultPlaneColor()); }

public:

  //! Return cached clipping box.
  const Bnd_Box& CachedBox() const { return myCachedBox; }

public:

  //! Returns kind of the object.
  virtual AIS_KindOfInteractive Type() const override { return AIS_KindOfInteractive_Datum; }

  //! Returns true for 0 mode.
  virtual Standard_Boolean AcceptDisplayMode(const Standard_Integer theMode) const override { return theMode == 0; }

  //! Compute presentation.
  Standard_EXPORT virtual void Compute(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                       const Handle(Prs3d_Presentation)&         thePrs,
                                       const Standard_Integer                    theMode) override;

  //! Compute selection.
  Standard_EXPORT virtual void ComputeSelection(const Handle(SelectMgr_Selection)& theSel,
                                                const Standard_Integer             theMode) override;

  //! Global selection has no meaning for this class.
  virtual Handle(SelectMgr_EntityOwner) GlobalSelOwner() const override { return Handle(SelectMgr_EntityOwner)(); }

  //! Drag object in the viewer.
  Standard_EXPORT virtual Standard_Boolean ProcessDragging(const Handle(AIS_InteractiveContext)& theCtx,
                                                           const Handle(V3d_View)&               theView,
                                                           const Handle(SelectMgr_EntityOwner)&  theOwner,
                                                           const Graphic3d_Vec2i&                theDragFrom,
                                                           const Graphic3d_Vec2i&                theDragTo,
                                                           const AIS_DragAction                  theAction) override;

protected:

  //! Update bounding box of a scene for displaying clipping planes.
  Standard_EXPORT void updateSceneBox(const Handle(PrsMgr_PresentationManager)& thePrsMgr);

protected:

  Handle(Graphic3d_SequenceOfHClipPlane) myManagedPlanes;
  Handle(Graphic3d_ClipPlane)            myDispPlane;

  Bnd_Box mySceneBox;
  Bnd_Box myCachedBox;

  Handle(Graphic3d_SequenceOfHClipPlane) myBndBoxPlanes;

  Prs3d_DatumMode myDatumDispMode = Prs3d_DM_Shaded;
  bool            myToDisplayPoints = false;

  gp_Pnt myDragFromLoc;             //!< dragging initial position
  gp_Pnt myDragOppLoc;              //!< dragging initial position of opposite plane
  double myDragPointZ = RealLast(); //!< point on dragged object (at beginning position)
};

//! Owner of a clipping plane.
class AIS_ClippingPlanesOwner : public SelectMgr_EntityOwner
{
  DEFINE_STANDARD_RTTIEXT(AIS_ClippingPlanesOwner, SelectMgr_EntityOwner)
public:

  //! Main constructor.
  Standard_EXPORT AIS_ClippingPlanesOwner(const Handle(AIS_ClippingPlanes)&  theObject,
                                          const Handle(Graphic3d_ClipPlane)& thePlane,
                                          const Handle(Graphic3d_ClipPlane)& theOppPlane,
                                          Standard_Integer                   thePriority = 5);

  //! Return clipping plane.
  const Handle(Graphic3d_ClipPlane)& Plane() const { return myPlane; }

  //! Return opposite directed clipping plane in chain or NULL.
  const Handle(Graphic3d_ClipPlane)& OppositePlane() const { return myOppPlane; }

  //! Handle mouse button click event.
  Standard_EXPORT virtual Standard_Boolean HandleMouseClick(const Graphic3d_Vec2i& thePoint,
                                                            Aspect_VKeyMouse       theButton,
                                                            Aspect_VKeyFlags       theModifiers,
                                                            bool                   theIsDoubleClick) override;

  //! Handle highlighting.
  Standard_EXPORT void HilightWithColor(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                        const Handle(Prs3d_Drawer)&               theStyle,
                                        const Standard_Integer                    theMode) override;

  //! Clear highlighting.
  Standard_EXPORT virtual void Unhilight(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                         const Standard_Integer                    theMode) override;

protected:

  Handle(Graphic3d_ClipPlane) myPlane;
  Handle(Graphic3d_ClipPlane) myOppPlane;
};

#endif // _AIS_ClippingPlanes_HeaderFile
