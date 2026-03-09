// Copyright (c) 2019 OPEN CASCADE SAS
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

#ifndef Image_RWAVCodec_HeaderFile
#define Image_RWAVCodec_HeaderFile

#include <Image_RWPixMap.hxx>

//! Class for loading/storing image pixmap from/into external file using FFmpeg libraries.
//! Supported image formats: BMP, PPM, PNG, JPEG, TIFF, TGA, GIF, EXR and others
//! (depending on FFmpeg building options).
//!
//! FFmpeg is an open-source framework for video encoding and decoding,
//! known for its universality (implements a tremendous amount of codecs and formats,
//! or wraps external codec-specific libraries) and efficiency
//! (performance-wise and size-wise as it tries to reuse common algorithms across codecs).
//!
//! However, as a video framework, its API has some flaws, when used for still image processing:
//! - avformat probing of decoder and demuxer might result in excessive reads from the input file;
//! - detection of videostream information (like dimension) would likely result
//!   in reading and decoding of the first frame - e.g. entire image;
//! - reading a video frame packet using avformat would likely
//!   result in reading the entire image file into memory;
//! - avcodec decoder from avpacket will decode the image into avframe,
//!   so that when image uses uncompressed file format, it will be duplicated in memory;
//!   there is practically no way to read a frame via partial reads from a stream;
//! - codecs tend to use pixel formats, optimal for video processing
//!   like YUV for JPEG and planar RGB (instead of packed) from EXR;
//!   which require an extra conversion step to be suitable for rending
//!   (e.g. without writing custom GLSL programs for each special pixel format);
//! - there are some legacy restrictions on image dimensions in FFmpeg (16384 pixels).
//!
//! The same applies to the images encoding process (extra conversion into suitable pixel format,
//! writing the entire encoded frame into memory prior to writing it into a file, etc.).
//!
//! Even considering these limitations, FFmpeg might be considered a pretty good choice
//! providing good performance and good support level compared to handling per-format low-level
//! image libraries yourself or using some other wrapping multi-format libraries.
class Image_RWAVCodec : public Image_RWPixMap
{
  DEFINE_STANDARD_RTTIEXT(Image_RWAVCodec, Image_RWPixMap)
public:

  //! Constructor.
  Standard_EXPORT Image_RWAVCodec();

  //! Return TRUE if the given image library is available.
  Standard_EXPORT virtual bool IsAvailable() const override;

  //! Return TRUE if the given image format could be read.
  Standard_EXPORT virtual bool SupportsReading(const TCollection_AsciiString& theName) const override;

  //! Return TRUE if the given image format could be written.
  Standard_EXPORT virtual bool SupportsWriting(const TCollection_AsciiString& theName) const override;

  //! Read image data.
  //!
  //! @warning FFmpeg codecs require aligned memory (16 or even 32 bytes) for utilizing SIMD instructions;
  //!          when input image doesn't meet alignment requirements,
  //!          the codec might work either slower, or lead to crash;
  //!          use InitTrash() method to ensure buffer is properly aligned.
  Standard_EXPORT virtual bool Read(Image_PixMap& thePixmap,
                                    const Handle(NCollection_Buffer)& theData,
                                    std::istream* theStream,
                                    const TCollection_AsciiString& theFileName) const override;

  //! Return default rows order used by underlying image library.
  virtual bool IsTopDownDefault() const override { return false; }

  //! Initialize image plane with required dimensions.
  Standard_EXPORT virtual bool InitTrash(Image_PixMap& thePixmap,
                                         Image_Format thePixelFormat,
                                         const NCollection_Vec3<Standard_Size>& theDims,
                                         const Standard_Size theSizeRowBytes) const override;

  //! Write image data to file using file extension to determine compression format.
  //!
  //! @warning FFmpeg codecs require aligned memory (16 or even 32 bytes) for utilizing SIMD instructions;
  //!          when input image doesn't meet alignment requirements,
  //!          the codec might work either slower, or lead to crash;
  //!          use InitTrash() method to ensure buffer is properly aligned.
  Standard_EXPORT virtual bool Write(Image_PixMap& thePixmap,
                                     const TCollection_AsciiString& theFileName,
                                     const TCollection_AsciiString& theFormat) const override;

private:
  class Owner;

};

#endif // Image_RWAVCodec_HeaderFile
