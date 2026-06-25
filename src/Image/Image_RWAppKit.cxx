// Copyright (c) 2010-2014 OPEN CASCADE SAS
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

#if defined(__APPLE__)
  #import <TargetConditionals.h>

  #if !(defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE)
    #define HAVE_APPKIT
  #endif
#endif

#include <Image_RWAppKit.hxx>

#include <Message.hxx>
#include <Standard_NotImplemented.hxx>
#include <TCollection_ExtendedString.hxx>

IMPLEMENT_STANDARD_RTTIEXT(Image_RWAppKit, Image_RWPixMap)

// ================================================================
// Function : Image_RWAppKit
// ================================================================
Image_RWAppKit::Image_RWAppKit()
{
  //
}

// ================================================================
// Function : IsAvailable
// ================================================================
bool Image_RWAppKit::IsAvailable() const
{
#if defined(HAVE_APPKIT)
  return true;
#else
  return false;
#endif
}

// ================================================================
// Function : SupportsReading
// ================================================================
bool Image_RWAppKit::SupportsReading(const TCollection_AsciiString& theName) const
{
#if defined(HAVE_APPKIT)
  return theName == IMAGE_TYPE_BMP
      || theName == IMAGE_TYPE_PNG
      || theName == IMAGE_TYPE_JPG
      || theName == IMAGE_TYPE_TIFF
      || theName == IMAGE_TYPE_GIF
      || theName == IMAGE_TYPE_PPM
      // extended list
      || theName == IMAGE_TYPE_HDR
      || theName == IMAGE_TYPE_EXR
      || theName == IMAGE_TYPE_PSD
      || theName == IMAGE_TYPE_ICO;
#else
  (void)theName;
  return false;
#endif
}

// ================================================================
// Function : SupportsWriting
// ================================================================
bool Image_RWAppKit::SupportsWriting(const TCollection_AsciiString& theName) const
{
#if defined(HAVE_APPKIT)
  return theName == IMAGE_TYPE_BMP
      || theName == IMAGE_TYPE_PNG
      || theName == IMAGE_TYPE_JPG
      || theName == IMAGE_TYPE_TIFF
      || theName == IMAGE_TYPE_GIF
      || theName == IMAGE_TYPE_PPM;
#else
  (void)theName;
  return false;
#endif
}

// ================================================================
// Function : InitTrash
// ================================================================
bool Image_RWAppKit::InitTrash(Image_PixMap& thePixmap,
                               Image_Format thePixelFormat,
                               const NCollection_Vec3<Standard_Size>& theDims,
                               const Standard_Size theSizeRowBytes) const
{
  thePixmap.Clear();

  Image_Format aFormat = thePixelFormat;
  switch (aFormat)
  {
    case Image_Format_BGR:
      aFormat = Image_Format_RGB;
      break;
    case Image_Format_BGR32:
      aFormat = Image_Format_RGB32;
      break;
    case Image_Format_BGRA:
      aFormat = Image_Format_RGBA;
      break;
    default:
      break;
  }

  if (!thePixmap.InitTrash3D(aFormat, theDims, theSizeRowBytes))
  {
    return false;
  }
  thePixmap.SetTopDown(true);
  return true;
}

#if !defined(HAVE_APPKIT)
// ================================================================
// Function : Read
// ================================================================
bool Image_RWAppKit::Read(Image_PixMap&                     thePixmap,
                          const Handle(NCollection_Buffer)& theData,
                          std::istream*                     theStream,
                          const TCollection_AsciiString&    theFileName) const
{
  thePixmap.Clear();
  (void)theData;
  (void)theStream;
  (void)theFileName;
  Message::SendFail() << "Error: image library is unavailable on this platform";
  return false;
}

// ================================================================
// Function : Write
// ================================================================
bool Image_RWAppKit::Write(Image_PixMap&                  thePixmap,
                           const TCollection_AsciiString& theFileName,
                           const TCollection_AsciiString& theFormat) const
{
  if (thePixmap.IsEmpty() || theFileName.IsEmpty())
  {
    return false;
  }

  (void)theFormat;
  Message::SendFail() << "Error: image library is unavailable on this platform";
  return false;
}
#endif
