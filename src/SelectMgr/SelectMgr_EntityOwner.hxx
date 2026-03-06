// Created on: 1995-05-23
// Created by: Robert COUBLANC
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

#ifndef _SelectMgr_EntityOwner_HeaderFile
#define _SelectMgr_EntityOwner_HeaderFile

#include <Aspect_VKey.hxx>
#include <PrsMgr_PresentationManager.hxx>
#include <SelectMgr_SelectableObject.hxx>
#include <TopLoc_Location.hxx>

class V3d_Viewer;

//! Interface defining transient picking details for SelectMgr_EntityOwner
//! (e.g. information about last picked sub-element to be re-highlighted).
class SelectMgr_LocalPickInfo : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(SelectMgr_LocalPickInfo, Standard_Transient)
public:

  //! Interface to override - compare two objects for equality.
  virtual bool IsEqual(const Handle(SelectMgr_LocalPickInfo)& theOther) const = 0;

protected:

  //! Empty constructor
  Standard_EXPORT SelectMgr_LocalPickInfo();

  //! Destructor.
  Standard_EXPORT virtual ~SelectMgr_LocalPickInfo();
};

//! Interface defining transient picking index.
class SelectMgr_LocalPickIndex : public SelectMgr_LocalPickInfo
{
  DEFINE_STANDARD_RTTIEXT(SelectMgr_LocalPickIndex, SelectMgr_LocalPickInfo)
public:

  //! Empty constructor
  Standard_EXPORT SelectMgr_LocalPickIndex();

  //! Destructor.
  Standard_EXPORT virtual ~SelectMgr_LocalPickIndex();

  //! Return index.
  Standard_Integer Index() const { return myIndex; }

  //! Set index.
  void SetIndex(Standard_Integer theIndex) { myIndex = theIndex; }

  //! Compare two indices.
  virtual bool IsEqual(const Handle(SelectMgr_LocalPickInfo)& theOther) const Standard_OVERRIDE
  {
    if (theOther == this)
      return true;

    SelectMgr_LocalPickIndex* anOther = dynamic_cast<SelectMgr_LocalPickIndex*>(theOther.get());
    return anOther != nullptr && anOther->myIndex == myIndex;
  }

protected:

  Standard_Integer myIndex = -1;
};

//! A framework to define classes of owners of sensitive primitives.
//! The owner is the link between application and selection data structures.
//! For the application to make its own objects selectable, it must define owner classes inheriting this framework.
class SelectMgr_EntityOwner : public Standard_Transient
{
  DEFINE_STANDARD_RTTIEXT(SelectMgr_EntityOwner, Standard_Transient)
public:

  //! Fill in local pick information to perform dynamic re-highlighting.
  static void FillLocalPickInfo (Handle(SelectMgr_LocalPickInfo)&        theInfo,
                                 const Handle(Select3D_SensitiveEntity)& theEntity)
  {
    if (!theEntity.IsNull())
      theEntity->OwnerId()->LocalPickInfo(theInfo, theEntity);
    else
      theInfo.Nullify();
  }

public:

  //! Initializes the selection priority aPriority.
  Standard_EXPORT SelectMgr_EntityOwner(const Standard_Integer aPriority = 0);

  //! Constructs a framework with the selectable object
  //! anSO being attributed the selection priority aPriority.
  Standard_EXPORT SelectMgr_EntityOwner(const Handle(SelectMgr_SelectableObject)& aSO, const Standard_Integer aPriority = 0);

  //! Constructs a framework from existing one
  //! anSO being attributed the selection priority aPriority.
  Standard_EXPORT SelectMgr_EntityOwner(const Handle(SelectMgr_EntityOwner)& theOwner, const Standard_Integer aPriority = 0);

  //! Return selection priority (within range [0-9]) for results with the same depth; 0 by default.
  //! Example - selection of shapes:
  //! the owners are selectable objects (presentations) a user can give vertex priority [3], edges [2] faces [1] shape [0],
  //! so that if during selection one vertex one edge and one face are simultaneously detected, the vertex will only be hilighted.
  Standard_Integer Priority() const { return mypriority; }

  //! Sets the selectable priority of the owner within range [0-9].
  void SetPriority (Standard_Integer thePriority) { mypriority = thePriority; }

  //! Returns true if there is a selectable object to serve as an owner.
  Standard_Boolean HasSelectable() const { return mySelectable != NULL; }

  //! Returns a selectable object detected in the working context.
  virtual Handle(SelectMgr_SelectableObject) Selectable() const { return mySelectable; }

  //! Sets the selectable object.
  virtual void SetSelectable (const Handle(SelectMgr_SelectableObject)& theSelObj) { mySelectable = theSelObj.get(); }

  //! Returns a selectable object.
  SelectMgr_SelectableObject* SelectablePointer() const { return mySelectable; }

  //! Sets the selectable object.
  void SetSelectablePointer (SelectMgr_SelectableObject* theSelObj) { mySelectable = theSelObj; }

  //! Handle mouse button click event.
  //! Does nothing by default and returns FALSE.
  //! @param thePoint      mouse cursor position
  //! @param theButton     clicked button
  //! @param theModifiers  key modifiers
  //! @param theIsDoubleClick flag indicating double mouse click
  //! @return TRUE if object handled click
  virtual Standard_Boolean HandleMouseClick (const Graphic3d_Vec2i& thePoint,
                                             Aspect_VKeyMouse theButton,
                                             Aspect_VKeyFlags theModifiers,
                                             bool theIsDoubleClick)
  {
    (void )thePoint; (void )theButton; (void )theModifiers; (void )theIsDoubleClick;
    return Standard_False;
  }

  //! Returns true if the presentation manager highlights selections corresponding to the selection mode.
  virtual Standard_Boolean IsHilighted (const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                        const Standard_Integer theMode = 0) const
  {
    return mySelectable != NULL
        && thePrsMgr->IsHighlighted (mySelectable, theMode);
  }
  
  //! Highlights selectable object's presentation with display mode in presentation manager with given highlight style.
  //! Also a check for auto-highlight is performed - if selectable object manages highlighting on its own,
  //! execution will be passed to SelectMgr_SelectableObject::HilightOwnerWithColor method.
  Standard_EXPORT virtual void HilightWithColor (const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                                                 const Handle(Prs3d_Drawer)& theStyle,
                                                 const Standard_Integer theMode = 0);

  //! Removes highlighting from the owner of a detected selectable object in the presentation manager.
  //! This object could be the owner of a sensitive primitive.
  //! @param thePrsMgr presentation manager
  //! @param theMode   obsolete argument for compatibility, should be ignored by implementations
  virtual void Unhilight (const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                          const Standard_Integer theMode = 0)
  {
    (void )theMode;
    if (mySelectable != NULL)
    {
      thePrsMgr->Unhighlight (mySelectable);
    }
  }

  //! Clears the owners matching the value of the selection
  //! mode aMode from the presentation manager object aPM.
  virtual void Clear (const Handle(PrsMgr_PresentationManager)& thePrsMgr,
                      const Standard_Integer theMode = 0) /// TODO
  {
    (void )thePrsMgr;
    (void )theMode;
  }

  //! Returns TRUE if selectable has transformation.
  virtual Standard_Boolean HasLocation() const { return mySelectable != NULL && mySelectable->HasTransformation(); }

  //! Returns transformation of selectable.
  virtual Handle(Graphic3d_HGTrsf) Location() const
  {
    return mySelectable != NULL && mySelectable->HasTransformation()
         ? mySelectable->TransformationGeom()
         : Handle(Graphic3d_HGTrsf)();
  }

  //! Change owner location (callback for handling change of location of selectable object).
  virtual void SetLocation (const Handle(Graphic3d_HGTrsf)& theLocation)
  {
    (void )theLocation;
  }

  //! @return Standard_True if the owner is selected.
  Standard_Boolean IsSelected() const { return myIsSelected; }

  //! Set the state of the owner.
  //! @param[in] theIsSelected  shows if owner is selected.
  void SetSelected (const Standard_Boolean theIsSelected) { myIsSelected = theIsSelected; }

  //! Returns selection state.
  Standard_DEPRECATED ("Deprecated method - IsSelected() should be used instead")
  Standard_Integer State() const { return myIsSelected ? 1 : 0; }

  //! Set the state of the owner.
  //! The method is deprecated. Use SetSelected() instead.
  void State (const Standard_Integer theStatus) { myIsSelected = (theStatus == 1); }

  //! if owner is not auto hilighted, for group contains many such owners will be called one method HilightSelected of SelectableObject
  virtual Standard_Boolean IsAutoHilight() const
  {
    return mySelectable == NULL
        || mySelectable->IsAutoHilight();
  }

  //! if this method returns TRUE the owner will always call method Hilight for SelectableObject when the owner is detected.
  //! By default it always return FALSE.
  virtual Standard_Boolean IsForcedHilight() const { return Standard_False; }

  //! If this method returns TRUE, the dynamic highlighting of this owner should be recomputed.
  virtual Standard_Boolean IsForcedDynamicHilight (const Handle(SelectMgr_LocalPickInfo)& thePrev,
                                                   const Handle(SelectMgr_LocalPickInfo)& theNew) const
  {
    if (!theNew.IsNull())
      return !theNew->IsEqual(thePrev);

    return IsForcedHilight();
  }

  //! Fill in local pick information to perform dynamic re-highlighting.
  virtual void LocalPickInfo (Handle(SelectMgr_LocalPickInfo)&        theInfo,
                              const Handle(Select3D_SensitiveEntity)& theEntity) const
  {
    theInfo.Nullify();
    (void)theEntity;
  }

  //! Set Z layer ID and update all presentations.
  virtual void SetZLayer (const Graphic3d_ZLayerId theLayerId)
  {
    (void )theLayerId;
  }

  //! Implements immediate application of location transformation of parent object to dynamic highlight structure
  virtual void UpdateHighlightTrsf (const Handle(V3d_Viewer)& theViewer,
                                    const Handle(PrsMgr_PresentationManager)& theManager,
                                    const Standard_Integer theDispMode)
  {
    if (mySelectable != NULL)
    {
      theManager->UpdateHighlightTrsf (theViewer, mySelectable, theDispMode);
    }
  }

  //! Returns true if pointer to selectable object of this owner is equal to the given one
  Standard_Boolean IsSameSelectable (const Handle(SelectMgr_SelectableObject)& theOther) const
  {
    return mySelectable == theOther.get();
  }

  //! Returns TRUE if this owner points to a part of object and FALSE for entire object.
  Standard_Boolean ComesFromDecomposition() const { return myFromDecomposition; }

  //! Sets flag indicating this owner points to a part of object (TRUE) or to entire object (FALSE).
  void SetComesFromDecomposition (const Standard_Boolean theIsFromDecomposition) { myFromDecomposition = theIsFromDecomposition; }

  //! Return string description of the owner (for tracing purposes).
  Standard_EXPORT virtual TCollection_AsciiString ToString() const;

  //! Dumps the content of me into the stream
  Standard_EXPORT virtual void DumpJson (Standard_OStream& theOStream, Standard_Integer theDepth = -1) const;

public:

  //! Sets the selectable object.
  Standard_DEPRECATED ("Deprecated method - SetSelectable() should be used instead")
  void Set (const Handle(SelectMgr_SelectableObject)& theSelObj) { SetSelectable (theSelObj); }

  //! sets the selectable priority of the owner
  Standard_DEPRECATED ("Deprecated method - SetPriority() should be used instead")
  void Set (const Standard_Integer thePriority) { SetPriority (thePriority); }

protected:

  SelectMgr_SelectableObject* mySelectable;        //!< raw pointer to selectable object
  Standard_Integer            mypriority;          //!< selection priority (for result with the same depth)
  Standard_Boolean            myIsSelected;        //!< flag indicating selected state
  Standard_Boolean            myFromDecomposition; //!< flag indicating this owner points to a part of object (TRUE) or to entire object (FALSE)

};

DEFINE_STANDARD_HANDLE(SelectMgr_EntityOwner, Standard_Transient)

Standard_DEPRECATED("Deprecated alias - SelectMgr_EntityOwner should be used instead")
typedef SelectMgr_EntityOwner SelectBasics_EntityOwner;

#endif // _SelectMgr_EntityOwner_HeaderFile
