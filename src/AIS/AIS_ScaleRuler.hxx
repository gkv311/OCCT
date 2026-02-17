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

#ifndef _AIS_ScaleRuler_HeaderFile
#define _AIS_ScaleRuler_HeaderFile

#include <AIS_InteractiveObject.hxx>
#include <Prs3d_TextAspect.hxx>

class Graphic3d_AspectLine3d;

//! Auxiliary widget for measuring distances in the viewer visually by displaying a ruler near the object.
//! Make sure to set Prs3d_Drawer::DimLengthModelUnits() within AIS_InteractiveContext::DefaultDrawer() for proper scaling.
//! The ruler will display length in SI units (meters) with prefix.
class AIS_ScaleRuler : public AIS_InteractiveObject
{
  DEFINE_STANDARD_RTTIEXT(AIS_ScaleRuler, AIS_InteractiveObject)

public:

  //! Public constructor
  Standard_EXPORT AIS_ScaleRuler();

  //! Constructor with configuration options
  //! @param[in] theCorner the corner of the view in which the ruler is displayed
  //! @param[in] theOffset the offset of the ruler from the corner in device-independent units
  //! @param[in] theBaseSize the base size of the ruler in device-independent units
  Standard_EXPORT AIS_ScaleRuler(const Aspect_TypeOfTriedronPosition theCorner,
                                 const Graphic3d_Vec2i&              theOffset,
                                 const Graphic3d_Vec2&               theBaseSize);

public:

  //! Public destructor
  Standard_EXPORT virtual ~AIS_ScaleRuler();

public:

  //! Callback called by Viewer before redrawing to adjust distance displayed by this ruler.
  Standard_EXPORT virtual AIS_RedrawProgressResult ProcessRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                                                 const Handle(V3d_View)&               theView,
                                                                 const AIS_RedrawProgress theRedrawProgress) override;

  //! Get the label font height in device-independent units; 20 by default.
  //! Alias for Prs3d_TextAspect::Height().
  double FontHeight() const { return myDrawer->TextAspect()->Height(); }

  //! Set the label font height in device-independent units.
  Standard_EXPORT void SetFontHeight(double theHeight);

  //! Get the offset from the corner in device-independent units.
  //! Alias for Graphic3d_TransformPers::Offset2d().
  Graphic3d_Vec2i Offset() const { return myTransformPersistence->Offset2d(); }

  //! Set the offset from the corner in device-independent units. Default value is 10.
  Standard_EXPORT void SetOffset(const Graphic3d_Vec2i& theOffset);

  //! Get the corner of the view where the ruler will be displayed.
  //! Alias for Graphic3d_TransformPers::Corner2d().
  Aspect_TypeOfTriedronPosition Corner() const { return myTransformPersistence->Corner2d(); }

  //! Set the corner of the view where the ruler will be displayed. Default is lower right.
  Standard_EXPORT void SetCorner(Aspect_TypeOfTriedronPosition theCorner);

  //! Return base size (in device-independent units).
  const Graphic3d_Vec2& BaseSize() const { return myBaseSize; }

  //! Set base size (in device-independent units). Default is 200 wide, 6 high.
  Standard_EXPORT void SetBaseSize(const Graphic3d_Vec2& theSize);

private:

  //! Return TRUE for supported display modes (only mode 0 is supported).
  virtual Standard_Boolean AcceptDisplayMode(const Standard_Integer theMode) const override { return theMode == 0; }

  //! Compute presentation.
  Standard_EXPORT virtual void Compute(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                       const Handle(Prs3d_Presentation)&         thePrs,
                                       const Standard_Integer                    theMode) override;

  //! Compute selection.
  Standard_EXPORT virtual void ComputeSelection(const Handle(SelectMgr_Selection)& theSel,
                                                const Standard_Integer             theMode) override;

  //! Handle dragging to temporarily display ruler and distance at point under mouse cursor.
  //! The main object will remain in the corner after dragging is finished.
  Standard_EXPORT virtual Standard_Boolean ProcessDragging(const Handle(AIS_InteractiveContext)& theContext,
                                                           const Handle(V3d_View)&               theView,
                                                           const Handle(SelectMgr_EntityOwner)&  theOwner,
                                                           const Graphic3d_Vec2i&                theDragFrom,
                                                           const Graphic3d_Vec2i&                theDragTo,
                                                           const AIS_DragAction                  theAction) override;

private:

  //! Calculate presentation corner.
  Graphic3d_Vec2i getPrsCorner(bool theIsFullLen) const;

private:

  static constexpr int   myDefaultFontHeight = 16; //!< Default font height for unit label text
  static constexpr int   myDefaultOffset = 10;     //!< Default offset from corner
  static constexpr float myDefaultWidth  = 200.0f;
  static constexpr float myDefaultHeight = 6.0f;

private:

  Handle(Graphic3d_AspectLine3d) myShadowAsp;

  Graphic3d_Vec2 myBaseSize = Graphic3d_Vec2(myDefaultWidth, myDefaultHeight);
  double         myBaseThick = 1.0;

  TCollection_AsciiString myDispLenText;       //!< The current state of the displayed scale label
  double                  myDispLenSys = 0.0;  //!< The scaled length proportion of the ruler
  double                  myDispLenBase = 0.0; //!< The current length of the ruler
                                               //!< (in device-independent units, that is in pixels at 100% scale)
  double myDispResRatio = 1.0;
};

#endif // _AIS_ScaleRuler_HeaderFile
