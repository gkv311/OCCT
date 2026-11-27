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

#ifndef _TopOpeBRepTool_2d_HeaderFile
#define _TopOpeBRepTool_2d_HeaderFile

#include <Geom2d_Curve.hxx>
#include <TopoDS_Edge.hxx>
#include <TopOpeBRepTool_DataMapOfShapeListOfC2DF.hxx>
#include <TopTools_IndexedDataMapOfShapeListOfShape.hxx>

//! Computes pcurves of the edges on the face,
//! caching the computed pcurves for their re-using by the subsequent calls.
class TopOpeBRepTool_2d
{
public:
  DEFINE_STANDARD_ALLOC

  //! Returns TRUE if the edge has 3d curve.
  Standard_EXPORT static Standard_Boolean HasC3D(const TopoDS_Edge& E);

  //! Returns TRUE if the edge has a pcurve on the face defined in the shape,
  //! and outputs this pcurve.
  Standard_EXPORT static Standard_Boolean HasOldCurveOnSurface(const TopoDS_Edge& E, const TopoDS_Face& F,
                                                               Handle(Geom2d_Curve)& C2D,
                                                               Standard_Real& f, Standard_Real& l,
                                                               Standard_Real& tol);

  //! Returns TRUE if the edge has a pcurve on the face defined in the shape.
  static Standard_Boolean HasOldCurveOnSurface(const TopoDS_Edge& E, const TopoDS_Face& F,
                                               Handle(Geom2d_Curve)& C2D)
  {
    Standard_Real f2d = 0.0, l2d = 0.0, tol = 0.0;
    return HasOldCurveOnSurface(E, F, C2D, f2d, l2d, tol);
  }

public:

  //! Empty constructor.
  Standard_EXPORT TopOpeBRepTool_2d();

  //! Destructor.
  Standard_EXPORT ~TopOpeBRepTool_2d();

  //! Clears the cach and collects the ancestor shapes for the given shapes.
  Standard_EXPORT Standard_Integer Prepare(const TopoDS_Shape& S1,
                                           const TopoDS_Shape& S2);


  //! Returns TRUE if the pcurve has been computed and cached and outputs this curve.
  Standard_EXPORT Standard_Boolean HasNewCurveOnSurface(const TopoDS_Edge& E, const TopoDS_Face& F,
                                                        Handle(Geom2d_Curve)& C2D,
                                                        Standard_Real& f, Standard_Real& l,
                                                        Standard_Real& tol) const;

  //! Returns TRUE if the pcurve has been computed and cached and outputs this curve.
  Standard_Boolean HasNewCurveOnSurface(const TopoDS_Edge& E, const TopoDS_Face& F,
                                        Handle(Geom2d_Curve)& C2D) const
  {
    Standard_Real f2d = 0.0, l2d = 0.0, tol = 0.0;
    return HasNewCurveOnSurface(E, F, C2D, f2d, l2d, tol);
  }

  //! Return TRUE if the edge has a pcurve on the face defined in the edge itself
  //! or computed and cached by this tool.
  Standard_EXPORT Standard_Boolean HasCurveOnSurface(const TopoDS_Edge& E, const TopoDS_Face& F) const;

  //! Returns pcurve of the edge on the face, taking pcurve in the edge itself (if exists),
  //! cached by this tool or computing and caching the new pcurve.
  Standard_EXPORT Handle(Geom2d_Curve) CurveOnSurface(const TopoDS_Edge& E, const TopoDS_Face& F,
                                                      Standard_Real& f, Standard_Real& l,
                                                      Standard_Real& tol,
                                                      const Standard_Boolean trim3d = Standard_False);

  //! Returns pcurve of the edge on the face, taking pcurve in the edge itself (if exists),
  //! cached by this tool or computing and caching the new pcurve.
  //! The extra edge on the same face is used for aligning the origin of the computed iso-pcurve.
  Standard_EXPORT Handle(Geom2d_Curve) CurveOnSurface(const TopoDS_Edge& E, const TopoDS_Face& F,
                                                      const TopoDS_Edge& EF,
                                                      Standard_Real& f, Standard_Real& l,
                                                      Standard_Real& tol,
                                                      const Standard_Boolean trim3d = Standard_False);

  //! Computes and caches the pcurcve of the edge on the face without looking
  //! for the pcurve defined in the shape or cached by this tool.
  Standard_EXPORT Handle(Geom2d_Curve) MakeCurveOnSurface(const TopoDS_Edge& E, const TopoDS_Face& F,
                                                          Standard_Real& f, Standard_Real& l,
                                                          Standard_Real& tol,
                                                          const Standard_Boolean trim3d = Standard_False);

  //! Returns pcurve of the edge on the face; the pcurve defined in the shape
  //! is returned as a copy, so that it can be edited withput affecting the shape.
  Standard_EXPORT Handle(Geom2d_Curve) EditableCurveOnSurface(const TopoDS_Edge& E, const TopoDS_Face& F,
                                                              Standard_Real& f, Standard_Real& l,
                                                              Standard_Real& tol,
                                                              const Standard_Boolean trim3d = Standard_False);

  //! Adds the pcurve to the cache as newly computed for the edge on the face.
  Standard_EXPORT void AddNewCurveOnSurface(const Handle(Geom2d_Curve)& PC,
                                            const TopoDS_Edge& E, const TopoDS_Face& F,
                                            const Standard_Real f, const Standard_Real l,
                                            const Standard_Real tol);

private:

  //! Returns one of the faces sharing the edge within the ancestor shapes, or NULL face if not found.
  const TopoDS_Face& FancestorE(const TopoDS_Edge& E) const;

  //! Computes pcurve of the edge on the face.
  Handle(Geom2d_Curve) make2d(const TopoDS_Edge& E, const TopoDS_Face& F,
                              Standard_Real& f2d, Standard_Real& l2d,
                              Standard_Real& tol,
                              const Standard_Boolean trim3d = Standard_False);


  //! Computes pcurve of the edge on the face taking the neigbor edge on the same face
  //! for aligning the origin of the computed iso-pcurve.
  Handle(Geom2d_Curve) make2d(const TopoDS_Edge& E, const TopoDS_Face& F,
                              const TopoDS_Edge& EF,
                              Standard_Real& f2d, Standard_Real& l2d,
                              Standard_Real& tol,
                              const Standard_Boolean trim3d = Standard_False);

  //! Translates iso-pcurve of the edge so that its origin in the face matches
  //! the origin of pcurve of the neighbor edge.
  static void translate(Handle(Geom2d_Curve) C2D,
                        const TopoDS_Edge& E, const TopoDS_Face& F,
                        const TopoDS_Edge& EF);

  //! Returns the cached pcurve on the edge on the face, or NULL if not cached.
  TopOpeBRepTool_C2DF* PNewCurveOnSurface(const TopoDS_Edge& E, const TopoDS_Face& F) const;

private:

  TopOpeBRepTool_DataMapOfShapeListOfC2DF myMapOfC2DF; //!< map of computed p-curves

  TopTools_IndexedDataMapOfShapeListOfShape myMapEdgeFaces; //!< map of ancestors
  TopoDS_Shape myS1; //!< first  input shape
  TopoDS_Shape myS2; //!< second input shape

  Standard_Integer myNbC2d = 0; //!< debugging counter

};

#endif
