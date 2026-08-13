// Created on: 1995-12-01
// Created by: EXPRESS->CDL V0.2 Translator
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

#ifndef _StepGeom_BSplineSurfaceForm_HeaderFile
#define _StepGeom_BSplineSurfaceForm_HeaderFile

#include <StepData_EnumTool.hxx>

enum StepGeom_BSplineSurfaceForm
{
StepGeom_bssfPlaneSurf,
StepGeom_bssfCylindricalSurf,
StepGeom_bssfConicalSurf,
StepGeom_bssfSphericalSurf,
StepGeom_bssfToroidalSurf,
StepGeom_bssfSurfOfRevolution,
StepGeom_bssfRuledSurf,
StepGeom_bssfGeneralisedCone,
StepGeom_bssfQuadricSurf,
StepGeom_bssfSurfOfLinearExtrusion,
StepGeom_bssfUnspecified
};

//! StepGeom_BSplineSurfaceForm text values.
static constexpr StepData_EnumTool::StringView StepGeom_BSplineSurfaceFormEnumValues[] =
{
  ".PLANE_SURF.",               // StepGeom_bssfPlaneSurf
  ".CYLINDRICAL_SURF.",         // StepGeom_bssfCylindricalSurf
  ".CONICAL_SURF.",             // StepGeom_bssfConicalSurf
  ".SPHERICAL_SURF.",           // StepGeom_bssfSphericalSurf
  ".TOROIDAL_SURF.",            // StepGeom_bssfToroidalSurf
  ".SURF_OF_REVOLUTION.",       // StepGeom_bssfSurfOfRevolution
  ".RULED_SURF.",               // StepGeom_bssfRuledSurf
  ".GENERALISED_CONE.",         // StepGeom_bssfGeneralisedCone
  ".QUADRIC_SURF.",             // StepGeom_bssfQuadricSurf
  ".SURF_OF_LINEAR_EXTRUSION.", // StepGeom_bssfSurfOfLinearExtrusion
  ".UNSPECIFIED.",              // StepGeom_bssfUnspecifieded
};

//! StepGeom_BSplineSurfaceForm enumeration conversion tool.
static constexpr StepData_EnumTool StepGeom_BSplineSurfaceFormEnumTool(StepGeom_BSplineSurfaceFormEnumValues);


#endif // _StepGeom_BSplineSurfaceForm_HeaderFile
