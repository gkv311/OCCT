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

#include <Image_RWAVCodec.hxx>

#include <Image_RWPPM.hxx>
#include <Message.hxx>
#include <Media_BufferPool.hxx>
#include <Media_CodecContext.hxx>
#include <Media_FormatContext.hxx>
#include <Media_Frame.hxx>
#include <Media_Packet.hxx>
#include <Media_Scaler.hxx>
#include <OSD_FileSystem.hxx>
#include <Standard_NotImplemented.hxx>

#include <algorithm>

#ifdef HAVE_FFMPEG
extern "C"
{
#ifdef _MSC_VER
  // suppress some common warnings in FFmpeg headers
  #pragma warning(disable : 4244)
#endif

  #include <libavcodec/avcodec.h>
  #include <libavformat/avformat.h>
  #include <libswscale/swscale.h>
  #include <libavutil/imgutils.h>

#ifdef _MSC_VER
  #pragma warning(default : 4244)
#endif
}

//! Owner of image data.
class Image_RWAVCodec::Owner : public NCollection_BaseAllocator
{
public:
  Handle(Media_Frame) LibImage;

public:
  //! Initialize from existing image.
  explicit Owner(const Handle(Media_Frame)& theLibImage) : LibImage(theLibImage) {}

  //! Destructor.
  ~Owner() { LibImage.Nullify(); }

  //! Dummy placeholder.
  virtual void* Allocate(const size_t) override { return nullptr; }

  //! Dummy placeholder.
  virtual void Free(void*) override {}
};

//! Convert frame to another pixel format
static Handle(Media_Frame) convertFrameToPixelFormat(const Handle(Media_Frame)& theFrame,
                                                     const AVPixelFormat theFormat)
{
  Handle(Media_Frame) aFrameTmp = new Media_Frame();
  aFrameTmp->ChangeFrame()->format = theFormat;
  aFrameTmp->ChangeFrame()->width  = theFrame->SizeX();
  aFrameTmp->ChangeFrame()->height = theFrame->SizeY();
  const int anAvErr = av_frame_get_buffer(aFrameTmp->ChangeFrame(), 0);
  if (anAvErr < 0)
  {
    Message::SendFail() << "Error: Image_RWAVCodec is unable to allocate frame "
      << theFrame->SizeX() << "x" << theFrame->SizeY() << "@" << theFormat
      << " (" << Media_FormatContext::FormatAVErrorDescription(anAvErr) << ")";
    return Handle(Media_Frame)();
  }

  // flip target image to bottom-up layout
  int& aLineSize = aFrameTmp->ChangeFrame()->linesize[0];
  if (aLineSize > 0 && aFrameTmp->LineSize(1) == 0)
  {
    uint8_t*& aData = aFrameTmp->ChangeFrame()->data[0];
    aData = aData + intptr_t(aLineSize) * (aFrameTmp->SizeY() - 1);
    aLineSize = -aLineSize;
  }

  Handle(Media_Scaler) aScaler = new Media_Scaler();
  if (!aScaler->Convert(theFrame, aFrameTmp))
  {
    Message::SendFail() << "Error: Image_RWAVCodec is unable to convert frame "
      << theFrame->SizeX() << "x" << theFrame->SizeY() << "@" << theFrame->Format()
      << " to format " << theFormat;
    return Handle(Media_Frame)();
  }
  return aFrameTmp;
}
#endif

IMPLEMENT_STANDARD_RTTIEXT(Image_RWAVCodec, Image_RWPixMap)

// ================================================================
// Function : Image_RWAVCodec
// Purpose  :
// ================================================================
Image_RWAVCodec::Image_RWAVCodec()
{
  //
}

// ================================================================
// Function : IsAvailable
// Purpose  :
// ================================================================
bool Image_RWAVCodec::IsAvailable() const
{
#ifdef HAVE_FFMPEG
  return true;
#else
  return false;
#endif
}

// ================================================================
// Function : SupportsReading
// Purpose  :
// ================================================================
bool Image_RWAVCodec::SupportsReading(const TCollection_AsciiString& theName) const
{
#ifdef HAVE_FFMPEG
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
// Function : SupportsWriting
// Purpose  :
// ================================================================
bool Image_RWAVCodec::SupportsWriting(const TCollection_AsciiString& theName) const
{
#ifdef HAVE_FFMPEG
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
// Purpose  :
// ================================================================
bool Image_RWAVCodec::InitTrash(Image_PixMap& thePixmap,
                                Image_Format thePixelFormat,
                                const NCollection_Vec3<Standard_Size>& theDims,
                                const Standard_Size theSizeRowBytes) const
{
  thePixmap.Clear();
#ifdef HAVE_FFMPEG
  const AVPixelFormat anAvFormat = (AVPixelFormat)Media_Frame::FormatOcct2FFmpeg(thePixelFormat);
  if (anAvFormat == AV_PIX_FMT_NONE || theDims.z() != 1)
    return thePixmap.InitTrash3D(thePixelFormat, theDims, theSizeRowBytes);

  // allocate image using FFmpeg to ensure correct memory alignment (16 or 32 bytes depending on running CPU)
  Handle(Media_Frame) aFrame = new Media_Frame();
  aFrame->ChangeFrame()->format = anAvFormat;
  aFrame->ChangeFrame()->width  = int(theDims.x());
  aFrame->ChangeFrame()->height = int(theDims.y());
  const int anAvErr = av_frame_get_buffer(aFrame->ChangeFrame(), 0);
  if (anAvErr < 0)
  {
    Message::SendFail() << "Error: Image_RWAVCodec is unable to allocate "
      << theDims.x() << "x" << theDims.y() << "@" << anAvFormat << " frame ("
      << Media_FormatContext::FormatAVErrorDescription(anAvErr) << ")";
     return false;
  }

  Handle(Image_RWAVCodec::Owner) anOwner = new Image_RWAVCodec::Owner(aFrame);
  thePixmap.InitWrapper3D(thePixelFormat, (Standard_Byte*)aFrame->ChangeFrame()->data[0],
                          theDims, (Standard_Size)aFrame->ChangeFrame()->linesize[0], anOwner);
  // the natural order is top-down for FFmpeg (e.g. thePixmap.SetTopDown(true)),
  // but it is capable to flip rows by specifying negative linesize
  return true;
#else
  return thePixmap.InitTrash3D(thePixelFormat, theDims, theSizeRowBytes);
#endif
}

// ================================================================
// Function : Read
// Purpose  :
// ================================================================
bool Image_RWAVCodec::Read(Image_PixMap& thePixmap,
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

#ifdef HAVE_FFMPEG
  Handle(Media_AVIOContext) anAVInput;
  if (!theData.IsNull())
    anAVInput = new Media_AVIOMemContext(theData);
  else if (theStream != nullptr)
    anAVInput = new Media_AVIOFileContext(theStream);

  Handle(Media_FormatContext) aFormatCtx = new Media_FormatContext();
  if (!aFormatCtx->OpenImageInput(theFileName, anAVInput))
    return false;

  Handle(Media_CodecContext) aVideoCtx;
  for (unsigned int aStreamId = 0; aStreamId < aFormatCtx->NbSteams(); ++aStreamId)
  {
    const AVStream&   aStream    = aFormatCtx->Stream(aStreamId);
    const AVMediaType aCodecType = aStream.codecpar->codec_type;
    if (aCodecType == AVMEDIA_TYPE_VIDEO)
    {
      aVideoCtx = new Media_CodecContext();

      Media_CodecContext::CodecParams aParams;
      aParams.NbThreads   = 1;     // use only one thread (multi-threading is more suitable for video)
      aParams.ToCheckDims = false; // don't read frame in advance
      aParams.IsTopDown   = false; // flip pixmap layout to better fit Image_PixMap defaults
      if (!aVideoCtx->Init(aStream, aFormatCtx->PtsStartBase(), aParams))
        return false;

      break;
    }
  }

  if (aVideoCtx.IsNull())
  {
    Message::SendFail() << "FFmpeg: no video stream in '" << theFileName << "'";
    return false;
  }

  Handle(Media_Packet) aPacket = new Media_Packet();
  Handle(Media_Frame) aFrame = new Media_Frame();
  for (;;)
  {
    if (!aFormatCtx->ReadPacket(aPacket))
    {
      Message::SendFail() << "FFmpeg: unable to read from '" << theFileName << "'";
      return false;
    }
    if (!aVideoCtx->CanProcessPacket(aPacket))
      continue;

    if (aVideoCtx->SendPacket(aPacket) && aVideoCtx->ReceiveFrame(aFrame))
      break;
  }
  if (aFrame->IsEmpty() || aFrame->SizeX() < 1 || aFrame->SizeY() < 1)
  {
    Message::SendFail() << "FFmpeg: unable to decode one frame from '" << theFileName << "'";
     return false;
  }

  Image_Format aPixFormat = Media_Frame::FormatFFmpeg2Occt(aFrame->Format());
  if (aPixFormat == Image_Format_UNKNOWN)
  {
    // convert frame to compatible pixel format
    const AVPixFmtDescriptor* aDesc = av_pix_fmt_desc_get((AVPixelFormat)aFrame->Format());

    const bool hasAlpha      = aDesc != nullptr && (aDesc->flags & AV_PIX_FMT_FLAG_ALPHA) != 0;
    const bool isFloatFormat = aDesc != nullptr && (aDesc->flags & AV_PIX_FMT_FLAG_FLOAT) != 0;

    aPixFormat = hasAlpha ? Image_Format_RGBA : Image_Format_RGB;
    if (isFloatFormat)
      aPixFormat = hasAlpha ? Image_Format_RGBAF : Image_Format_RGBF;

    aFrame = convertFrameToPixelFormat(aFrame, (AVPixelFormat)Media_Frame::FormatOcct2FFmpeg(aPixFormat));
    if (aFrame.IsNull())
      return false;
  }

  const NCollection_Vec3<Standard_Size> aDims(aFrame->SizeX(), aFrame->SizeY(), 1);

  Handle(Image_RWAVCodec::Owner) anOwner = new Image_RWAVCodec::Owner(aFrame);

  const int aLineSize = aFrame->LineSize(0);
  uint8_t*  aData     = aFrame->Plane(0);
  if (aLineSize < 0)
    aData = aData + intptr_t(aLineSize) * (aFrame->SizeY() - 1);

  thePixmap.InitWrapper3D(aPixFormat, (Standard_Byte*)aData,
                          aDims, (Standard_Size)Abs(aLineSize), anOwner);
  thePixmap.SetTopDown(aLineSize > 0);
  return true;
#else
  (void)theData;
  (void)theStream;
  (void)theFileName;
  Message::SendFail() << "Error: image library was disabled during build (HAVE_FFMPEG undefined)";
  return false;
#endif
}

// ================================================================
// Function : Write
// Purpose  :
// ================================================================
bool Image_RWAVCodec::Write(Image_PixMap& thePixmap,
                            const TCollection_AsciiString& theFileName,
                            const TCollection_AsciiString& theFormat) const
{
  if (thePixmap.IsEmpty() || theFileName.IsEmpty())
    return false;

  const TCollection_AsciiString aFormat = !theFormat.IsEmpty() ? theFormat : CommonFormatFromName(theFileName);
  if (aFormat.IsEmpty())
  {
    Message::SendFail() << "Error: Image_RWAVCodec, unknown image format";
    return false;
  }

  if (aFormat == IMAGE_TYPE_PPM)
  {
    Image_RWPPM aTool;
    return aTool.Write(thePixmap, theFileName, IMAGE_TYPE_PPM);
  }

#ifdef HAVE_FFMPEG
  Handle(Media_Frame) aFrame = new Media_Frame();
  if (!aFrame->InitWrapper(thePixmap))
  {
    Message::SendFail() << "Error: unable to wrap Image_PixMap into Media_Frame for encoding";
    return false;
  }

  const AVCodec* aCodec = nullptr;
  if (aFormat == IMAGE_TYPE_JPG)
    aCodec = avcodec_find_encoder_by_name("mjpeg");
  else
    aCodec = avcodec_find_encoder_by_name(aFormat.ToCString());

  if (aCodec == nullptr)
  {
    Message::SendFail() << "Error: Image_RWAVCodec is unable to find codec '" << aFormat << "'";
    return false;
  }

  std::shared_ptr<AVCodecContext> aCodecCtx;
  aCodecCtx.reset(avcodec_alloc_context3(aCodec), [](AVCodecContext* theCtx)
  {
    AVCodecContext* aPtr = theCtx;
    avcodec_free_context(&aPtr);
  });

  // get pixel formats supported by encoder
  const AVPixelFormat* aPixFormats = nullptr;
#if LIBAVCODEC_VERSION_INT >= AV_VERSION_INT(61, 13, 100)
  int aNbPixFormats = 0;
  avcodec_get_supported_config(aCodecCtx.get(), aCodec, AV_CODEC_CONFIG_PIX_FORMAT, 0, (const void**)&aPixFormats, &aNbPixFormats);
#else
  aPixFormats = aCodec->pix_fmts;
#endif
  auto hasAvFormat = [aPixFormats](AVPixelFormat theFormat) -> bool
  {
    for (int aFormatIter = 0; aPixFormats != nullptr && aPixFormats[aFormatIter] != AV_PIX_FMT_NONE; ++aFormatIter)
      if (aPixFormats[aFormatIter] == theFormat)
        return true;

    return false;
  };

  AVPixelFormat aResFormat = aPixFormats != nullptr ? aPixFormats[0] : (AVPixelFormat)aFrame->Format();
  if (hasAvFormat((AVPixelFormat)aFrame->Format()))
    aResFormat = (AVPixelFormat)aFrame->Format();

  // swap rgba/bgra without memory copies when possible (PNG codec uses RGBA, BMP uses BGRA, etc.)
  if (aResFormat != aFrame->Format())
  {
    const Image_Format  aSwappedFormat   = Image_PixMap::SwappedRgbaBgraFormat(thePixmap.Format());
    const AVPixelFormat aSwappedAvFormat = (AVPixelFormat)Media_Frame::FormatOcct2FFmpeg(aSwappedFormat);
    if (aSwappedAvFormat != AV_PIX_FMT_NONE && hasAvFormat(aSwappedAvFormat))
    {
      aResFormat = aSwappedAvFormat;
      aFrame->ChangeFrame()->format = aSwappedAvFormat;
      Image_PixMap::SwapRgbaBgra(thePixmap);
      thePixmap.SetFormat(aSwappedFormat);
    }
  }

  // workaround RGB(A) export using EXR codec, which lists GRAY format prior to RGB(A)
  if (aResFormat != aFrame->Format() && aResFormat == AV_PIX_FMT_GRAYF32)
  {
    if (Image_PixMap::ImageFormatNbComponents(thePixmap.Format()) == 3 && hasAvFormat(AV_PIX_FMT_GBRPF32))
      aResFormat = AV_PIX_FMT_GBRPF32;
    else if (Image_PixMap::ImageFormatNbComponents(thePixmap.Format()) == 4 && hasAvFormat(AV_PIX_FMT_GBRAPF32))
      aResFormat = AV_PIX_FMT_GBRAPF32;
  }

  // convert frame to compatible pixel format
  if (aResFormat != aFrame->Format())
  {
    aFrame = convertFrameToPixelFormat(aFrame, aResFormat);
    if (aFrame.IsNull())
      return false;
  }

  // setup frame
  aCodecCtx->pix_fmt = (AVPixelFormat)aFrame->Format();
  aCodecCtx->width = aFrame->SizeX();
  aCodecCtx->height = aFrame->SizeY();
  aCodecCtx->time_base.num = 1;
  aCodecCtx->time_base.den = 1;

  // setup compression
  if (aFormat == IMAGE_TYPE_PNG) // PNG compression level is within [0..9] range
    aCodecCtx->compression_level = 5;
  else if (aFormat == IMAGE_TYPE_JPG) // quantization factor within [1..31] range, lesser is better
    aCodecCtx->qmin = aCodecCtx->qmax = 5;

  // open VIDEO codec
  const int anAvErr = avcodec_open2(aCodecCtx.get(), aCodec, nullptr);
  if (anAvErr < 0)
  {
    Message::SendFail() << "Error: Image_RWAVCodec is unable to open video codec '"
                        << aFormat << "' (" << Media_FormatContext::FormatAVErrorDescription(anAvErr) << ")";
    return false;
  }

  Media_Packet aPacket;
  int anEncSize = 0;
  if (avcodec_send_frame(aCodecCtx.get(), aFrame->Frame()) == 0
      && avcodec_receive_packet(aCodecCtx.get(), aPacket.ChangePacket()) == 0
      && aPacket.ChangePacket()->data != nullptr)
  {
    anEncSize = aPacket.Size();
  }

  if (anEncSize <= 0)
  {
    Message::SendFail() << "Error: Image_RWAVCodec failed to encode the image";
    return false;
  }

  const Handle(OSD_FileSystem)& aFileSystem = OSD_FileSystem::DefaultFileSystem();
  std::shared_ptr<std::ostream> aStream = aFileSystem->OpenOStream(theFileName, std::ios::out | std::ios::binary);
  if (aStream.get() == nullptr)
  {
    Message::SendFail() << "Error: Image_RWAVCodec failed to open output file '" << theFileName << "'";
    return false;
  }

  aStream->write((const char*)aPacket.Data(), anEncSize);
  aStream->flush();
  if (!aStream->good())
  {
    Message::SendFail() << "Error: Image_RWAVCodec failed to write output file '" << theFileName << "'";
    return false;
  }
  return true;
#else
  (void)theFormat;
  Message::SendFail() << "Error: image library was disabled during build (HAVE_FREEIMAGE undefined)";
  return false;
#endif
}
