// Created on: 1997-11-25
// Created by: Jean Yves LEBEY
// Copyright (c) 1997-1999 Matra Datavision
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

#ifndef _TopOpeBRepDS_connex_HeaderFile
#define _TopOpeBRepDS_connex_HeaderFile

#include <TopTools_DataMapOfShapeListOfShape.hxx>

class TopOpeBRepDS_HDataStructure;

//! Tool for the exploration of the connexity of the shapes
//! (edges and faces) stored in the data structure.
class TopOpeBRepDS_connex
{
public:
  DEFINE_STANDARD_ALLOC

  //! Empty constructor.
  Standard_EXPORT TopOpeBRepDS_connex();

  //! Destructor.
  Standard_EXPORT ~TopOpeBRepDS_connex();

  //! Computes the connexity maps of the shapes of the given data structure,
  //! clearing the previously computed maps.
  Standard_EXPORT void Prepare(const TopoDS_Shape& theS1,
                               const TopoDS_Shape& theS2,
                               const Handle(TopOpeBRepDS_HDataStructure)& theHDS);

  //! Clears the connexity maps.
  Standard_EXPORT void Close();

  //! Return the list of the faces connex to the edge @p theE of the rank @p theSI (1 or 2),
  //! or na empty list if the connexity is not avilable.
  Standard_EXPORT const TopTools_ListOfShape& EdgeConnexityShapeIndex(const TopoDS_Shape& theE,
                                                                      const Handle(TopOpeBRepDS_HDataStructure)& theHDS,
                                                                      const Standard_Integer theSI) const;

  //! Returns the connexity of the shape @p theS:
  //! - for the edge, the list of the connex faces;
  //! - for the face, the list of its edges having at least one other connext face.
  Standard_EXPORT const TopTools_ListOfShape& EdgeConnexitySameShape(const TopoDS_Shape& theS,
                                                                     const Handle(TopOpeBRepDS_HDataStructure)& theHDS) const;

  //! Returns TRUE if the face or edge @p theS of the rank 1 or 2 is stored in the connexity maps.
  Standard_EXPORT Standard_Boolean HasConnexFace(const TopoDS_Shape& theS,
                                                 const Handle(TopOpeBRepDS_HDataStructure)& theHDS) const;

  //! Returns the faces in @p theLF sharing with the face @p theF the edge @p theE of the connexity.
  Standard_EXPORT void FaceEdgeConnexFaces(const TopoDS_Shape& theF,
                                           const TopoDS_Shape& theE,
                                           const Handle(TopOpeBRepDS_HDataStructure)& theHDS,
                                           TopTools_ListOfShape& theLF) const;

  //! Prints the connexity of all shapes.
  Standard_EXPORT void Dump(const Handle(TopOpeBRepDS_HDataStructure)& theHDS) const;

  //! Prints the connexity of the shape at the given index in the DRAW format.
  Standard_EXPORT void Dump(const Handle(TopOpeBRepDS_HDataStructure)& theHDS,
                            const Standard_Integer theI) const;

  //! Prints the connexity of the shape at the given index.
  Standard_EXPORT void DumpIndex(const Handle(TopOpeBRepDS_HDataStructure)& theHDS,
                                 const Standard_Integer theI) const;

private:

  TopTools_DataMapOfShapeListOfShape myElf1;
  TopTools_DataMapOfShapeListOfShape myElf2;
  TopTools_DataMapOfShapeListOfShape myFle;
  Standard_Boolean myPrepared = Standard_False;

};

#endif
