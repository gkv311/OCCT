// Copyright (c) 2014 OPEN CASCADE SAS
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

#ifndef _XCAFPrs_AISObject_HeaderFile
#define _XCAFPrs_AISObject_HeaderFile

#include <AIS_ColoredShape.hxx>
#include <NCollection_StdAllocator.hxx>
#include <RWMesh_NameFormat.hxx>
#include <StdSelect_BRepOwner.hxx>
#include <TDF_Label.hxx>
#include <XCAFPrs_Style.hxx>

class XCAFPrs_BRepOwner;
class XCAFPrs_Style;

//! Implements AIS_InteractiveObject functionality for shape in DECAF document.
class XCAFPrs_AISObject : public AIS_ColoredShape
{
public:

  //! Creates an object to visualise the shape label.
  Standard_EXPORT XCAFPrs_AISObject (const TDF_Label& theLabel);

  //! Creates an object to visualize the shape labels.
  Standard_EXPORT XCAFPrs_AISObject(const NCollection_List<Handle(XCAFPrs_BRepOwner)>& theLabels);

  //! Returns the labels.
  const NCollection_List<Handle(XCAFPrs_BRepOwner)>& GetLabels() const { return myLabels; }

  //! Assign the labels to this presentation
  //! (but does not mark it outdated with SetToUpdate()).
  Standard_EXPORT void SetLabels(const NCollection_List<Handle(XCAFPrs_BRepOwner)>& theLabel);

  //! Returns the label which was visualized by this presentation
  Standard_EXPORT const TDF_Label& GetLabel() const;

  //! Assign the label to this presentation
  //! (but does not mark it outdated with SetToUpdate()).
  Standard_EXPORT void SetLabel(const TDF_Label& theLabel);

  //! Fetch the Shape from associated Label and fill the map of sub-shapes styles.
  //! By default, this method is called implicitly within first ::Compute().
  //! Application might call this method explicitly to manipulate styles afterwards.
  //! @param theToSyncStyles flag indicating if method ::Compute() should call this method again
  //!                        on first compute or re-compute
  Standard_EXPORT virtual void DispatchStyles (const Standard_Boolean theToSyncStyles = Standard_False);

  //! Sets the material aspect.
  //! This method assigns the new default material without overriding XDE styles.
  //! Re-computation of existing presentation is not required after calling this method.
  Standard_EXPORT virtual void SetMaterial(const Graphic3d_MaterialAspect& theMaterial) Standard_OVERRIDE;

  //! Returns the owner of mode for selection of object as a whole
  Standard_EXPORT virtual Handle(SelectMgr_EntityOwner) GlobalSelOwner() const Standard_OVERRIDE;

  //! Handle updates.
  Standard_EXPORT virtual AIS_RedrawProgressResult ProcessRedraw(const Handle(AIS_InteractiveContext)& theCtx,
                                                                 const Handle(V3d_View)&               theView,
                                                                 const AIS_RedrawProgress theRedrawProgress)
    Standard_OVERRIDE;

  //! Synchronize aspects.
  Standard_EXPORT virtual void SynchronizeAspects() Standard_OVERRIDE;

  //! Check if selected parts should be highlighted within main presentation.
  Standard_EXPORT bool ToHighlightInMainPrs() const;

  //! Invalidate highlgihting aspects within main presentation.
  void InvalidateHighlightInMainPrs() { myToRehighlight = true; }

protected:

  //! Redefined method to compute presentation.
  Standard_EXPORT virtual void Compute (const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                        const Handle(Prs3d_Presentation)& thePrs,
                                        const Standard_Integer theMode) Standard_OVERRIDE;

  //! Compute selection considering sub-shape hidden state.
  Standard_EXPORT virtual void ComputeSelection (const Handle(SelectMgr_Selection)& theSel,
                                                 const Standard_Integer theMode) Standard_OVERRIDE;

  //! Fills out a default style object which is used when styles are
  //! not explicitly defined in the document.
  //! By default, the style uses white color for curves and surfaces.
  Standard_EXPORT virtual void DefaultStyle (XCAFPrs_Style& theStyle) const;

protected:

  //! Empty constructor.
  Standard_EXPORT XCAFPrs_AISObject();

  //! This method shouldn't be used to change the shape externally.
  virtual bool SetShape(const TopoDS_Shape&) Standard_OVERRIDE { return false; }

  //! This method shouldn't be used to change the shape externally.
  using AIS_ColoredShape::Set;

protected:

  //! labels pointing onto the shapes
  NCollection_List<Handle(XCAFPrs_BRepOwner)> myLabels;
  //! flag indicating that shape and sub-shapes should be updates within Compute()
  Standard_Boolean myToSyncStyles = true;
  //! flag indicating that highlighting should be re-computed
  Standard_Boolean myToRehighlight = false;

public:

  DEFINE_STANDARD_RTTIEXT(XCAFPrs_AISObject,AIS_ColoredShape)

};

typedef std::vector<TDF_Label, NCollection_StdAllocator<TDF_Label>> XCAFPrs_LabelPath;

//! Owner of a shape label within XCAF document.
class XCAFPrs_BRepOwner : public StdSelect_BRepOwner
{
  DEFINE_STANDARD_RTTIEXT(XCAFPrs_BRepOwner, StdSelect_BRepOwner)
public:

  //! Constructor.
  //! @param[in] theLabel shape label
  //! @param[in] theLoc  parent location within assembly structure
  //! @param[in] theStyle optional base style
  Standard_EXPORT XCAFPrs_BRepOwner(const TDF_Label&       theLabel,
                                    const TopLoc_Location& theLoc,
                                    const XCAFPrs_Style&   theStyle = XCAFPrs_Style());

  //! Constructor.
  //! @param[in] thePath shape label path
  //! @param[in] theLoc  parent location within assembly structure
  //! @param[in] theStyle optional base style
  Standard_EXPORT XCAFPrs_BRepOwner(const XCAFPrs_LabelPath& thePath,
                                    const TopLoc_Location&   theLoc,
                                    const XCAFPrs_Style&     theStyle = XCAFPrs_Style());

  //! Return shape label within the XCAF document.
  const TDF_Label& ShapeLabel() const { return myShapePath.back(); }

  //! Return shape label within the XCAF document.
  const XCAFPrs_LabelPath& ShapePath() const { return myShapePath; }

  //! Return parent location within assembly structure (excluding location of shape label itself).
  const TopLoc_Location& ParentLocation() const { return myParentLoc; }

  //! Return optional name.
  const TCollection_AsciiString& Name() const { return myName; }

  //! Set name.
  void SetName(const TCollection_AsciiString& theName) { myName = theName; }

  //! Fetch name from shape labels.
  Standard_EXPORT TCollection_AsciiString FormatName(RWMesh_NameFormat theFormat, bool theFullPath) const;

  //! Fetch shape from XCAF label and update it's stats.
  Standard_EXPORT void UpdateShape();

  //! Return string description of the owner (for tracing purposes).
  Standard_EXPORT virtual TCollection_AsciiString ToString() const Standard_OVERRIDE;

  //! Return transient highlighting state.
  bool IsHighlightedInPrs() const { return myIsHighlighted; }

  //! Set transient highlighting state.
  void SetHighlightedInPrs(bool theIsHighlighted) { myIsHighlighted = theIsHighlighted; }

  //! Handle highlighting.
  Standard_EXPORT virtual void HilightWithColor(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                                const Handle(Prs3d_Drawer)&               theStyle,
                                                const Standard_Integer                    theMode) Standard_OVERRIDE;

  //! Handle unhighlighting.
  Standard_EXPORT virtual void Unhilight(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                         const Standard_Integer                    theMode) Standard_OVERRIDE;

  //! Handle clearing.
  Standard_EXPORT virtual void Clear(const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                     const Standard_Integer                    theMode) Standard_OVERRIDE;

  //! Return optional base style.
  const XCAFPrs_Style& BaseStyle() const { return myStyle; }

  //! Return optional base style.
  XCAFPrs_Style& ChangeBaseStyle() { return myStyle; }

  //! Set optional base style.
  void SetBaseStyle(const XCAFPrs_Style& theStyle) { myStyle = theStyle; }

protected:

  XCAFPrs_LabelPath       myShapePath; //!< shape label path within the XCAF document
  TopLoc_Location         myParentLoc; //!< parent location within assembly structure
  TCollection_AsciiString myName;      //!< optional name

  XCAFPrs_Style    myStyle;                 //!< base style
  Standard_Boolean myIsHighlighted = false; //!< transient highlighting state
};

//! AIS drawer initialized from XCAFPrs_Style.
class XCAFPrs_AISDrawer : public AIS_ColoredDrawer
{
  DEFINE_STANDARD_RTTIEXT(XCAFPrs_AISDrawer, AIS_ColoredDrawer)
public:

  //! Update style.
  Standard_EXPORT static void UpdateStyle(Prs3d_Drawer&               theDrawer,
                                          const XCAFPrs_Style&        theStyle,
                                          const Handle(Prs3d_Drawer)& theHiDrawer);

public:

  //! Constructor.
  Standard_EXPORT XCAFPrs_AISDrawer(const Handle(Prs3d_Drawer)& theLink,
                                    const XCAFPrs_Style&        theStyle,
                                    const XCAFPrs_Style&        theDefStyle = XCAFPrs_Style());

  //! Update style on highlighting.
  void SetHighlighted(bool theToHighlight) { myStyle.SetHighlighted(theToHighlight); }

  //! Update style.
  void UpdateStyle(const Handle(Prs3d_Drawer)& theHiDrawer) { UpdateStyle(*this, myStyle, theHiDrawer); }

protected:

  XCAFPrs_Style myStyle;
};

#endif // _XCAFPrs_AISObject_HeaderFile
