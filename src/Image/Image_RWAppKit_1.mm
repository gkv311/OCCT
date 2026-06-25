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

#import <TargetConditionals.h>

#if defined(TARGET_OS_IPHONE) && TARGET_OS_IPHONE
  #import <UIKit/UIKit.h>
#else
  #import <Cocoa/Cocoa.h>
  #define HAVE_APPKIT
#endif

#include <Image_RWAppKit.hxx>

#include <Cocoa_LocalPool.hxx>
#include <Image_RWPPM.hxx>
#include <Message.hxx>
#include <TCollection_ExtendedString.hxx>

#if defined(HAVE_APPKIT)

#if !defined(MAC_OS_X_VERSION_10_14) || (MAC_OS_X_VERSION_MAX_ALLOWED < MAC_OS_X_VERSION_10_14)
  #define NSBitmapImageFileTypePNG  NSPNGFileType
  #define NSBitmapImageFileTypeBMP  NSBMPFileType
  #define NSBitmapImageFileTypeJPEG NSJPEGFileType
  #define NSBitmapImageFileTypeTIFF NSTIFFFileType
  #define NSBitmapImageFileTypeGIF  NSGIFFileType
#endif


//! Owner of image data.
class Image_RWAppKit::Owner : public NCollection_BaseAllocator
{
public:
  NSBitmapImageRep* LibImage = nullptr;

public:
  //! Initialize from existing image.
  explicit Owner(NSBitmapImageRep* theLibImage) : LibImage([theLibImage retain]) {}

  //! Destructor.
  ~Owner() { [LibImage release]; }

  //! Dummy placeholder.
  virtual void* Allocate(const size_t) override { return nullptr; }

  //! Dummy placeholder.
  virtual void Free(void*) override {}
};

// ================================================================
// Function : Read
// ================================================================
bool Image_RWAppKit::Read(Image_PixMap& thePixmap,
                          const Handle(NCollection_Buffer)& theData,
                          std::istream* theStream,
                          const TCollection_AsciiString& theFileName) const
{
  thePixmap.Clear();

  const TCollection_AsciiString aFormat = ProbeFormat(theData, theStream, theFileName);
  if (aFormat == IMAGE_TYPE_PPM)
  {
    Image_RWPPM aTool;
    return aTool.Read(thePixmap, theData, theStream, theFileName);
  }

  Cocoa_LocalPool aLocalPool;

  Handle(NCollection_Buffer) aBuffer = theData;
  if (theStream != nullptr && theData.IsNull())
  {
    // fallback copying stream data into transient buffer
    const std::streamoff aStart = theStream->tellg();
    theStream->seekg(0, std::ios::end);
    const size_t aLen = size_t(theStream->tellg() - aStart);
    theStream->seekg(aStart);
    if (aLen <= 0)
    {
      Message::SendFail() << "Error: empty stream";
      return false;
    }

    aBuffer = new NCollection_Buffer(Image_PixMap::DefaultAllocator());
    if (!aBuffer->Allocate(aLen))
    {
      Message::SendFail() << "Error: not enough memory";
      return false;
    }

    if (!theStream->read((char*)aBuffer->ChangeData(), aLen))
    {
      Message::SendFail() << "Error: unable to read stream";
      return false;
    }
  }

  NSImage* anNsImage = nullptr;
  NSData*  anNsData  = nullptr;
  if (!aBuffer.IsNull())
  {
    anNsData = [[[NSData alloc] initWithBytesNoCopy: aBuffer->ChangeData()
                                             length: aBuffer->Size()
                                       freeWhenDone: NO]  autorelease];
    anNsImage = [[[NSImage alloc] initWithData: anNsData] autorelease];
  }
  else
  {
    NSString* aNsFileName = [[[NSString alloc] initWithUTF8String: theFileName.ToCString()] autorelease];
    anNsImage = [[[NSImage alloc] initWithContentsOfFile: aNsFileName] autorelease];
  }

  if (anNsImage == nullptr)
  {
    Message::SendFail() << "Image_RWAppKit: unable to read from '" << theFileName << "'";
    return false;
  }

  NSBitmapImageRep* anNsImgRep = (NSBitmapImageRep*)[[anNsImage representations] objectAtIndex: 0];
  if (![anNsImgRep isKindOfClass: [NSBitmapImageRep class]])
  {
    Message::SendFail() << "Image_RWAppKit: unsupported image representation '" << theFileName << "'";
    return false;
  }
  return WrapNSBitmap(thePixmap, anNsImgRep, theFileName);
}

// ================================================================
// Function : WrapNSBitmap
// ================================================================
bool Image_RWAppKit::WrapNSBitmap(Image_PixMap& thePixmap,
                                  NSBitmapImageRep* theNsImgRep,
                                  const TCollection_AsciiString& theFileName)
{
  thePixmap.Clear();

  Cocoa_LocalPool aLocalPool;

  if ([theNsImgRep isPlanar])
  {
    Message::SendFail() << "Image_RWAppKit: unsupported planar format '" << theFileName << "'";
    return false;
  }

  const bool isFloat = ([theNsImgRep bitmapFormat] & NSBitmapFormatFloatingPointSamples) != 0;

  Image_Format anImgFormat = Image_Format_UNKNOWN;
  if ([theNsImgRep samplesPerPixel] == 1)
  {
    if ([theNsImgRep bitsPerPixel] == 8)
      anImgFormat = Image_Format_Gray;
    else if (isFloat && [theNsImgRep bitsPerPixel] == 16)
      anImgFormat = Image_Format_GrayF_half; // from EXR
    else if (isFloat && [theNsImgRep bitsPerPixel] == 32)
      anImgFormat = Image_Format_GrayF;
  }
  else if ([theNsImgRep samplesPerPixel] == 3)
  {
    if ([theNsImgRep bitsPerPixel] == 24)
      anImgFormat = Image_Format_RGB;
    else if ([theNsImgRep bitsPerPixel] == 32)
      anImgFormat = Image_Format_RGB32;
    else if (isFloat && [theNsImgRep bitsPerPixel] == 96)
      anImgFormat = Image_Format_RGBF;
    else if (isFloat && [theNsImgRep bitsPerPixel] == 128)
      anImgFormat = Image_Format_RGBAF; // nit: we don't have RGBF format with unused A
  }
  else if ([theNsImgRep samplesPerPixel] == 4)
  {
    if ([theNsImgRep bitsPerPixel] == 32)
      anImgFormat = Image_Format_RGBA;
    else if (isFloat && [theNsImgRep bitsPerPixel] == 64)
      anImgFormat = Image_Format_RGBAF_half; // from EXR
    else if (isFloat && [theNsImgRep bitsPerPixel] == 128)
      anImgFormat = Image_Format_RGBAF;
  }

  const auto formatImgRep = [](NSBitmapImageRep* theRep) -> std::string
  {
    std::stringstream aStr;
    aStr << "NSBitmapImageRep"
         << " samplesPerPixel: " << [theRep samplesPerPixel]
         << "; bitsPerPixel: "   << [theRep bitsPerPixel]
         << "; size: "           << [theRep pixelsWide] << "x" << [theRep pixelsHigh]
         << "; bytesPerRow: "    << [theRep bytesPerRow]
         << "; isPlanar: "       << [theRep isPlanar]
         << "; bitmapFormat: "   << [theRep bitmapFormat]
         << "; colorSpace: "     << [[[theRep colorSpace] localizedName] UTF8String];
    return aStr.str();
  };
  if (anImgFormat == Image_Format_UNKNOWN)
  {
    Message::SendFail() << "Image_RWAppKit: unsupported format [" << formatImgRep(theNsImgRep) << "]";
    return false;
  }

  //Message::SendInfo() << formatImgRep(theNsImgRep);

  Handle(Image_RWAppKit::Owner) anOwner = new Image_RWAppKit::Owner(theNsImgRep);

  NCollection_Vec3<size_t> aDims([theNsImgRep pixelsWide], [theNsImgRep pixelsHigh], 1);
  thePixmap.InitWrapper3D(anImgFormat, [theNsImgRep bitmapData], aDims, [theNsImgRep bytesPerRow], anOwner);
  thePixmap.SetTopDown(true);
  return true;
}

// ================================================================
// Function : Write
// ================================================================
bool Image_RWAppKit::Write(Image_PixMap& thePixmap,
                           const TCollection_AsciiString& theFileName,
                           const TCollection_AsciiString& theFormat) const
{
  if (thePixmap.IsEmpty() || theFileName.IsEmpty())
  {
    return false;
  }

  const TCollection_AsciiString aFormat = !theFormat.IsEmpty() ? theFormat : CommonFormatFromName(theFileName);
  if (aFormat.IsEmpty())
  {
    Message::SendFail() << "Error: Image_RWAppKit, unknown image format";
    return false;
  }

  if (aFormat == IMAGE_TYPE_PPM)
  {
    Image_RWPPM aTool;
    return aTool.Write(thePixmap, theFileName, IMAGE_TYPE_PPM);
  }

  Cocoa_LocalPool aLocalPool;

  NSBitmapImageFileType aFileType = NSBitmapImageFileTypePNG;
  if (aFormat == IMAGE_TYPE_BMP)
  {
    aFileType = NSBitmapImageFileTypeBMP;
  }
  else if (aFormat == IMAGE_TYPE_PNG)
  {
    aFileType = NSBitmapImageFileTypePNG;
  }
  else if (aFormat == IMAGE_TYPE_JPG)
  {
    aFileType = NSBitmapImageFileTypeJPEG;
  }
  else if (aFormat == IMAGE_TYPE_TIFF)
  {
    aFileType = NSBitmapImageFileTypeTIFF;
  }
  else if (aFormat == IMAGE_TYPE_GIF)
  {
    aFileType = NSBitmapImageFileTypeGIF;
  }
  else
  {
    Message::SendFail() << "Error: Image_RWAppKit unsupported image file format '" << theFileName << "'";
    return false;
  }

  if (thePixmap.Format() == Image_Format_BGR
   || thePixmap.Format() == Image_Format_BGRF
   || thePixmap.Format() == Image_Format_BGRA
   || thePixmap.Format() == Image_Format_BGRAF
   || thePixmap.Format() == Image_Format_BGR32)
  {
    const Image_Format aSwappedFormat = Image_PixMap::SwappedRgbaBgraFormat(thePixmap.Format());
    Image_PixMap::SwapRgbaBgra(thePixmap);
    thePixmap.SetFormat(aSwappedFormat);
  }

  const bool isFloat = thePixmap.Format() == Image_Format_RGBF
                    || thePixmap.Format() == Image_Format_RGBAF
                    || thePixmap.Format() == Image_Format_RGBAF_half
                    || thePixmap.Format() == Image_Format_AlphaF
                    || thePixmap.Format() == Image_Format_GrayF
                    || thePixmap.Format() == Image_Format_GrayF_half;
  if (thePixmap.Format() != Image_Format_RGB
   && thePixmap.Format() != Image_Format_RGBA
   && thePixmap.Format() != Image_Format_RGB32
   && thePixmap.Format() != Image_Format_Alpha
   && thePixmap.Format() != Image_Format_Gray
   && thePixmap.Format() != Image_Format_RGBF
   && thePixmap.Format() != Image_Format_RGBAF
   && thePixmap.Format() != Image_Format_RGBAF_half
   && thePixmap.Format() != Image_Format_AlphaF
   && thePixmap.Format() != Image_Format_GrayF
   && thePixmap.Format() != Image_Format_GrayF_half)
  {
    Message::SendFail() << "Error: Image_RWAppKit unsupported pixel format "
                        << Image_PixMap::ImageFormatToString(thePixmap.Format());
    return false;
  }

  if (!thePixmap.IsTopDown())
  {
    Image_PixMap::FlipY(thePixmap);
    thePixmap.SetTopDown(true);
  }

  NSData* aData = nullptr;
  {
    NSColorSpace* aNsColorspace = [NSColorSpace sRGBColorSpace];

    NSColorSpaceName anInitColorspace = NSCalibratedRGBColorSpace;
    if (Image_PixMap::ImageFormatNbComponents(thePixmap.Format()) == 1)
    {
      anInitColorspace = NSCalibratedWhiteColorSpace; // grayscale
      aNsColorspace = [NSColorSpace genericGamma22GrayColorSpace];
    }

    unsigned char* aPlanes[1] = { thePixmap.ChangeData() };
    const bool hasAlphaComp = Image_PixMap::ImageFormatNbComponents(thePixmap.Format()) == 4
                           || thePixmap.Format() == Image_Format_RGB32;

    NSBitmapImageRep* anImageRep = [[[NSBitmapImageRep alloc]
      initWithBitmapDataPlanes: aPlanes
                    pixelsWide: (NSInteger)thePixmap.Width()
                    pixelsHigh: (NSInteger)thePixmap.Height()
                 bitsPerSample: (thePixmap.SizePixelBytes() / Image_PixMap::ImageFormatNbComponents(thePixmap.Format())) * 8
               samplesPerPixel: Image_PixMap::ImageFormatNbComponents(thePixmap.Format())
                      hasAlpha: hasAlphaComp
                      isPlanar: NO
                colorSpaceName: anInitColorspace
                  bitmapFormat: isFloat ? NSBitmapFormatFloatingPointSamples : 0
                   bytesPerRow: (NSInteger)thePixmap.SizeRowBytes()
                  bitsPerPixel: (NSInteger)thePixmap.SizePixelBytes() * 8
    ] autorelease];

    // re-tag colorspace (without conversion)
    NSBitmapImageRep* anImageRepSrgb = [anImageRep bitmapImageRepByRetaggingWithColorSpace: aNsColorspace];
    if (anImageRepSrgb == nullptr)
      anImageRepSrgb = anImageRep;

    NSDictionary* anImgProps = [NSDictionary dictionaryWithObject: [NSNumber numberWithFloat: 0.5]
                                                           forKey: NSImageCompressionFactor];

    aData = [anImageRepSrgb representationUsingType: aFileType properties: anImgProps];
  }
  if (aData == nullptr)
  {
    Message::SendFail() << "Error: Image_RWAppKit failed to create an image '" << theFileName << "'";
    return false;
  }

  NSString* aFileName = [[[NSString alloc] initWithUTF8String: theFileName.ToCString()] autorelease];

  // [NSData writeToFile:aFileName atomically:YES] will create a temporary file first,
  // and documentation says that is unsafe, suggesting to use NSFileHandle instead.
  if (![[NSFileManager defaultManager] createFileAtPath: aFileName contents: aData attributes: nil])
  {
    Message::SendFail() << "Error: Image_RWAppKit failed to write output file '" << theFileName << "'";
    return false;
  }
  return true;
}

#endif
