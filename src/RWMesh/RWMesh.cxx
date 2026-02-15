// Copyright (c) 2021 OPEN CASCADE SAS
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

#include <RWMesh.hxx>

#include <TDataStd_Name.hxx>
#include <TDF_Tool.hxx>
#include <XCAFDoc_ShapeTool.hxx>

// ================================================================
// Function : ReadNameAttribute
// Purpose  :
// ================================================================
TCollection_AsciiString RWMesh::ReadNameAttribute (const TDF_Label& theLabel)
{
  return TCollection_AsciiString(TDataStd_Name::GetString(theLabel));
}

// ================================================================
// Function : FormatName
// Purpose  :
// ================================================================
TCollection_AsciiString RWMesh::FormatName (RWMesh_NameFormat theFormat,
                                            const TDF_Label& theLabel,
                                            const TDF_Label& theRefLabel)
{
  return XCAFDoc_ShapeTool::FormatName(theFormat, theLabel, theRefLabel);
}
