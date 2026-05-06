// Created on: 2008-01-20
// Created by: Alexander A. BORODIN
// Copyright (c) 2008-2014 OPEN CASCADE SAS
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

#include <Font_SystemFont.hxx>

#include <Font_FontMgr.hxx>
#include <Font_FTFont.hxx>
#include <OSD_Path.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Font_SystemFont, Standard_Transient)

// =======================================================================
// function : Font_SystemFont
// purpose  :
// =======================================================================
Font_SystemFont::Font_SystemFont (const TCollection_AsciiString& theFontName)
: myFontKey (theFontName),
  myFontName (theFontName)
{
  if (theFontName.IsEmpty()) { throw Standard_ProgramError ("Font_SystemFont constructor called with empty font name"); }
  myFontKey.LowerCase();
}

// =======================================================================
// function : SetFontPath
// purpose  :
// =======================================================================
void Font_SystemFont::SetFontPath (Font_FontAspect theAspect,
                                   const TCollection_AsciiString& thePath,
                                   const Standard_Integer theFaceId)
{
  if (theAspect == Font_FontAspect_UNDEFINED) { throw Standard_ProgramError ("Font_SystemFont::SetFontPath() called with UNDEFINED aspect"); }
  myFilePaths[theAspect] = thePath;
  myFaceIds  [theAspect] = theFaceId;

  TCollection_AsciiString aDummy;
  OSD_Path::FolderAndFileFromPath(thePath, aDummy, myFileNames[theAspect]);
  myFileNames[theAspect].LowerCase();
}

// =======================================================================
// function : IsEqual
// purpose  :
// =======================================================================
Standard_Boolean Font_SystemFont::IsEqual (const Handle(Font_SystemFont)& theOtherFont) const
{
  return theOtherFont.get() == this
      || myFontKey.IsEqual (theOtherFont->myFontKey);
}

// =======================================================================
// function : ToString
// purpose  :
// =======================================================================
TCollection_AsciiString Font_SystemFont::ToString() const
{
  TCollection_AsciiString aDesc;
  aDesc += TCollection_AsciiString() + "'" + myFontName + "'";

  bool isFirstAspect = true;
  aDesc += " [aspects: ";
  for (int anAspectIter = 0; anAspectIter < Font_FontAspect_NB; ++anAspectIter)
  {
    if (!HasFontAspect ((Font_FontAspect )anAspectIter))
    {
      continue;
    }

    if (!isFirstAspect)
    {
      aDesc += ",";
    }
    else
    {
      isFirstAspect = false;
    }
    aDesc += Font_FontMgr::FontAspectToString ((Font_FontAspect )anAspectIter);
  }
  aDesc += "]";

  isFirstAspect = true;
  aDesc += " [paths: ";
  for (int anAspectIter = 0; anAspectIter < Font_FontAspect_NB; ++anAspectIter)
  {
    if (!HasFontAspect ((Font_FontAspect )anAspectIter))
    {
      continue;
    }

    if (!isFirstAspect)
    {
      aDesc += ";";
    }
    else
    {
      isFirstAspect = false;
    }
    aDesc += FontPath ((Font_FontAspect )anAspectIter);
    if (FontFaceId ((Font_FontAspect )anAspectIter) != 0)
    {
      aDesc = aDesc + "," + FontFaceId ((Font_FontAspect )anAspectIter);
    }
  }
  aDesc += "]";
  return aDesc;
}

// =======================================================================
// function : FontPathAny
// purpose  :
// =======================================================================
const TCollection_AsciiString& Font_SystemFont::FontPathAny (Font_FontAspect theAspect,
                                                             Font_FTFontParams& theParams,
                                                             Standard_Integer& theFaceId) const
{
  const Font_FontAspect anAspect = theAspect != Font_FontAspect_UNDEFINED ? theAspect : Font_FontAspect_Regular;
  const TCollection_AsciiString& aPath = myFilePaths[anAspect];
  theFaceId = myFaceIds[anAspect];
  if (!aPath.IsEmpty())
    return aPath;

  // synthesize aspects from closest available
  if (theAspect == Font_FontAspect_Italic
  && !myFilePaths[Font_FontAspect_Regular].IsEmpty())
  {
    theParams.ToSynthesizeItalic = true;
    theFaceId = myFaceIds[Font_FontAspect_Regular];
    return myFilePaths[Font_FontAspect_Regular];
  }
  else if (theAspect == Font_FontAspect_Bold
        && !myFilePaths[Font_FontAspect_Regular].IsEmpty())
  {
    theParams.ToSynthesizeBold = true;
    theFaceId = myFaceIds[Font_FontAspect_Regular];
    return myFilePaths[Font_FontAspect_Regular];
  }
  else if (theAspect == Font_FontAspect_BoldItalic
        && !myFilePaths[Font_FontAspect_Bold].IsEmpty())
  {
    theParams.ToSynthesizeItalic = true;
    theFaceId = myFaceIds[Font_FontAspect_Bold];
    return myFilePaths[Font_FontAspect_Bold];
  }
  else if (theAspect == Font_FontAspect_BoldItalic
        && !myFilePaths[Font_FontAspect_Italic].IsEmpty())
  {
    theParams.ToSynthesizeBold = true;
    theFaceId = myFaceIds[Font_FontAspect_Italic];
    return myFilePaths[Font_FontAspect_Italic];
  }
  else if (theAspect == Font_FontAspect_BoldItalic
        && !myFilePaths[Font_FontAspect_Regular].IsEmpty())
  {
    theParams.ToSynthesizeBold = true;
    theParams.ToSynthesizeItalic = true;
    theFaceId = myFaceIds[Font_FontAspect_Regular];
    return myFilePaths[Font_FontAspect_Regular];
  }

  // return anything at the end
  if (!myFilePaths[Font_FontAspect_Regular].IsEmpty())
  {
    theFaceId = myFaceIds[Font_FontAspect_Regular];
    return myFilePaths[Font_FontAspect_Regular];
  }

  for (int anAspectIter = 0; anAspectIter < Font_FontAspect_NB; ++anAspectIter)
  {
    if (!myFilePaths[anAspectIter].IsEmpty())
    {
      theFaceId = myFaceIds[anAspectIter];
      return myFilePaths[anAspectIter];
    }
  }
  theFaceId = myFaceIds[Font_FontAspect_Regular];
  return myFilePaths[Font_FontAspect_Regular];
}
