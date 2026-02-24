// Created by: Kirill GAVRILOV
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

// activate some C99 macros like UINT64_C in "stdint.h" which used by FFmpeg
#ifndef __STDC_CONSTANT_MACROS
  #define __STDC_CONSTANT_MACROS
#endif

#include <Media_FormatContext.hxx>

#include <Message.hxx>
#include <Message_Messenger.hxx>

#ifdef HAVE_FFMPEG
#include <Standard_WarningsDisable.hxx>
extern "C"
{
  #include <libavformat/avformat.h>
  #include <libavcodec/avcodec.h>
}
#include <Standard_WarningsRestore.hxx>
#endif

#include <cstring>

IMPLEMENT_STANDARD_RTTIEXT(Media_FormatContext, Standard_Transient)

namespace
{
  static const double THE_SECONDS_IN_HOUR = 3600.0;
  static const double THE_SECONDS_IN_MINUTE = 60.0;
  static const double THE_SECOND_IN_HOUR   = 1.0 / THE_SECONDS_IN_HOUR;
  static const double THE_SECOND_IN_MINUTE = 1.0 / THE_SECONDS_IN_MINUTE;

#ifdef HAVE_FFMPEG
  static const AVRational ST_AV_TIME_BASE_Q = {1, AV_TIME_BASE};
  static const double     ST_AV_TIME_BASE_D = av_q2d (ST_AV_TIME_BASE_Q);

  //! Format framerate value.
  static TCollection_AsciiString formatFps (double theVal)
  {
    const uint64_t aVal = uint64_t(theVal * 100.0 + 0.5);
    char aBuff[256];
    if(aVal == 0)
    {
      Snprintf (aBuff, "%1.4f", theVal);
    }
    else if (aVal % 100)
    {
      Snprintf (aBuff, "%3.2f", theVal);
    }
    else if (aVal % (100 * 1000))
    {
      Snprintf (aBuff, "%1.0f", theVal);
    }
    else
    {
      Snprintf (aBuff, "%1.0fk", theVal / 1000);
    }
    return aBuff;
  }
#endif
}

// =======================================================================
// function : Media_AVIOContext
// =======================================================================
Media_AVIOContext::Media_AVIOContext()
{
#ifdef HAVE_FFMPEG
  auto readCallback = [](void* theOpaque, uint8_t* theBuf, int theBufSize) -> int
  {
    return theOpaque != nullptr ? ((Media_AVIOContext*)theOpaque)->Read(theBuf, theBufSize) : 0;
  };
  auto seekCallback = [](void* theOpaque, int64_t theOffset, int theWhence) -> int64_t
  {
    return theOpaque != nullptr ? ((Media_AVIOContext*)theOpaque)->Seek(theOffset, theWhence) : -1;
  };
#if(LIBAVFORMAT_VERSION_MAJOR >= 61)
  auto writeCallback = [](void* theOpaque, const uint8_t* theBuf, int theBufSize) -> int
#else
  auto writeCallback = [](void* theOpaque, uint8_t* theBuf, int theBufSize) -> int
#endif
  {
    return theOpaque != nullptr ? ((Media_AVIOContext*)theOpaque)->Write(theBuf, theBufSize) : 0;
  };
  auto freeCallback = [](AVIOContext* thePtr)
  {
    if (thePtr != nullptr)
      av_free(thePtr);
  };

  constexpr int  aBufferSize = 32768;
  unsigned char* aBufferIO = (unsigned char*)av_malloc(aBufferSize + AV_INPUT_BUFFER_PADDING_SIZE);
  myAvioCtx.reset(avio_alloc_context(aBufferIO, aBufferSize, 0, this, readCallback, writeCallback, seekCallback),
                  freeCallback);
#endif
}

// =======================================================================
// function : ~Media_AVIOContext
// =======================================================================
Media_AVIOContext::~Media_AVIOContext()
{
  //
}

// =======================================================================
// function : Media_AVIOMemContext
// =======================================================================
Media_AVIOMemContext::Media_AVIOMemContext(const Handle(NCollection_Buffer)& theData)
{
  mySrcBuffer = (uint8_t*)theData->ChangeData();
  mySrcSize = theData->Size();
}

// =======================================================================
// function : Media_AVIOMemContext::Read
// =======================================================================
int Media_AVIOMemContext::Read(uint8_t* theBuf, int theBufSize)
{
  if (theBuf == nullptr || theBufSize <= 0 || mySrcBuffer == nullptr || mySrcSize == 0)
    return -1;

  int aNbRead = theBufSize;
  if (myPosition + theBufSize > mySrcSize)
    aNbRead = int(mySrcSize - myPosition);

#ifdef HAVE_FFMPEG
  if (aNbRead == 0)
    return AVERROR_EOF;
#endif

  std::memcpy(theBuf, mySrcBuffer + myPosition, aNbRead);
  myPosition += aNbRead;
  return aNbRead;
}

// =======================================================================
// function : Media_AVIOMemContext::Write
// =======================================================================
int Media_AVIOMemContext::Write(const uint8_t* theBuf, const int theBufSize)
{
  if (theBuf == nullptr || theBufSize <= 0 || mySrcBuffer == nullptr || mySrcSize == 0)
    return -1;

  int aNbWritten = theBufSize;
  if (myPosition + theBufSize > mySrcSize)
    aNbWritten = int(mySrcSize - myPosition);

#ifdef HAVE_FFMPEG
  if (aNbWritten == 0)
    return AVERROR_EOF;
#endif

  std::memcpy(mySrcBuffer + myPosition, theBuf, aNbWritten);
  myPosition += aNbWritten;
  return aNbWritten;
}

// =======================================================================
// function : Media_AVIOMemContext::Seek
// purpose  :
// =======================================================================
int64_t Media_AVIOMemContext::Seek(int64_t theOffset, int theWhence)
{
#ifdef HAVE_FFMPEG
  if (theWhence == AVSEEK_SIZE)
    return mySrcSize;
#endif

  if (mySrcBuffer == nullptr || mySrcSize == 0)
    return -1;

  if (theWhence == SEEK_SET)
    myPosition = theOffset;
  else if (theWhence == SEEK_CUR)
    myPosition += theOffset;
  else if (theWhence == SEEK_END)
    myPosition = mySrcSize + theOffset;

  if (myPosition < 0)
    myPosition = 0;
  else if (myPosition > mySrcSize)
    myPosition = mySrcSize;

  return myPosition;
}

// =======================================================================
// function : Media_AVIOFileContext
// =======================================================================
Media_AVIOFileContext::Media_AVIOFileContext(std::istream* theStream)
: myStream(theStream)
{
  myStartPos = (int64_t)myStream->tellg();
}

// =======================================================================
// function : Media_AVIOFileContext::Read
// =======================================================================
int Media_AVIOFileContext::Read(uint8_t* theBuf, int theBufSize)
{
  if (myStream == nullptr)
    return -1;

  myStream->read((char*)theBuf, theBufSize);

  const int aNbRead = (int)myStream->gcount();
#ifdef HAVE_FFMPEG
  if (aNbRead == 0 && myStream->eof())
    return AVERROR_EOF;
#endif

  return aNbRead;
}

// =======================================================================
// function : Media_AVIOFileContext::Seek
// =======================================================================
int64_t Media_AVIOFileContext::Seek(int64_t theOffset, int theWhence)
{
#ifdef HAVE_FFMPEG
  if (theWhence == AVSEEK_SIZE)
    return -1;
#endif

  if (myStream == nullptr)
    return -1;

  std::ios_base::seekdir aDir = std::ios_base::cur;
  switch (theWhence)
  {
    case SEEK_CUR: aDir = std::ios_base::cur; break;
    case SEEK_SET: aDir = std::ios_base::beg; break;
    case SEEK_END: aDir = std::ios_base::end; break;
    default: return -1;
  }
  if (theWhence == SEEK_SET)
    theOffset += myStartPos;

  myStream->clear();
  myStream->seekg((std::streampos)theOffset, aDir);
  if (!myStream->good())
    return -1;

  return (int64_t)myStream->tellg() - myStartPos;
}

// =======================================================================
// function : FormatAVErrorDescription
// purpose  :
// =======================================================================
TCollection_AsciiString Media_FormatContext::FormatAVErrorDescription (int theErrCodeAV)
{
#ifdef HAVE_FFMPEG
  char aBuff[4096] = {};
  if (av_strerror (theErrCodeAV, aBuff, 4096) != -1)
  {
    return TCollection_AsciiString (aBuff);
  }

#ifdef _MSC_VER
  wchar_t aBuffW[4096] = {};
  if (_wcserror_s (aBuffW, 4096, AVUNERROR(theErrCodeAV)) == 0)
  {
    return TCollection_AsciiString (aBuffW);
  }
#elif defined(_WIN32)
  // MinGW has only thread-unsafe variant
  char* anErrDesc = strerror (AVUNERROR(theErrCodeAV));
  if (anErrDesc != NULL)
  {
    return TCollection_AsciiString (anErrDesc);
  }
#endif
  return TCollection_AsciiString (aBuff);
#else
  return TCollection_AsciiString ("AVError #") + theErrCodeAV;
#endif
}

// =======================================================================
// function : FormatUnitsToSeconds
// purpose  :
// =======================================================================
double Media_FormatContext::FormatUnitsToSeconds (int64_t theTimeUnits)
{
#ifdef HAVE_FFMPEG
  return (theTimeUnits != AV_NOPTS_VALUE)
        ? (ST_AV_TIME_BASE_D * theTimeUnits) : 0.0;
#else
  (void )theTimeUnits;
  return 0.0;
#endif
}

// =======================================================================
// function : UnitsToSeconds
// purpose  :
// =======================================================================
double Media_FormatContext::UnitsToSeconds (const AVRational& theTimeBase,
                                            int64_t theTimeUnits)
{
#ifdef HAVE_FFMPEG
  return (theTimeUnits != AV_NOPTS_VALUE)
        ? (av_q2d (theTimeBase) * theTimeUnits) : 0.0;
#else
  (void )&theTimeBase;
  (void )theTimeUnits;
  return 0.0;
#endif
}

// =======================================================================
// function : StreamUnitsToSeconds
// purpose  :
// =======================================================================
double Media_FormatContext::StreamUnitsToSeconds (const AVStream& theStream,
                                                  int64_t theTimeUnits)
{
#ifdef HAVE_FFMPEG
  return UnitsToSeconds (theStream.time_base, theTimeUnits);
#else
  (void )&theStream;
  (void )theTimeUnits;
  return 0.0;
#endif
}

// =======================================================================
// function : SecondsToUnits
// purpose  :
// =======================================================================
int64_t Media_FormatContext::SecondsToUnits (double theTimeSeconds)
{
#ifdef HAVE_FFMPEG
  return int64_t(theTimeSeconds / ST_AV_TIME_BASE_D);
#else
  (void )theTimeSeconds;
  return 0;
#endif
}

// =======================================================================
// function : SecondsToUnits
// purpose  :
// =======================================================================
int64_t Media_FormatContext::SecondsToUnits (const AVRational& theTimeBase,
                                             double theTimeSeconds)
{
#ifdef HAVE_FFMPEG
  return int64_t(theTimeSeconds / av_q2d (theTimeBase));
#else
  (void )&theTimeBase;
  (void )theTimeSeconds;
  return 0;
#endif
}

// =======================================================================
// function : Media_FormatContext
// purpose  :
// =======================================================================
int64_t Media_FormatContext::StreamSecondsToUnits (const AVStream& theStream,
                                                   double theTimeSeconds)
{
#ifdef HAVE_FFMPEG
  return SecondsToUnits (theStream.time_base, theTimeSeconds);
#else
  (void )&theStream;
  (void )theTimeSeconds;
  return 0;
#endif
}

// =======================================================================
// function : Media_FormatContext
// purpose  :
// =======================================================================
Media_FormatContext::Media_FormatContext()
: myFormatCtx   (NULL),
  myPtsStartBase(0.0),
  myDuration    (0.0)
{
  //
}

// =======================================================================
// function : ~Media_FormatContext
// purpose  :
// =======================================================================
Media_FormatContext::~Media_FormatContext()
{
  Close();
}

// =======================================================================
// function : NbSteams
// purpose  :
// =======================================================================
unsigned int Media_FormatContext::NbSteams() const
{
#ifdef HAVE_FFMPEG
  return myFormatCtx->nb_streams;
#else
  return 0;
#endif
}

// =======================================================================
// function : Stream
// purpose  :
// =======================================================================
const AVStream& Media_FormatContext::Stream (unsigned int theIndex) const
{
#ifdef HAVE_FFMPEG
  return *myFormatCtx->streams[theIndex];
#else
  (void )theIndex;
  throw Standard_ProgramError("Media_FormatContext::Stream()");
#endif
}

// =======================================================================
// function : OpenInput
// purpose  :
// =======================================================================
bool Media_FormatContext::OpenInput (const TCollection_AsciiString& theInput,
                                     const Handle(Media_AVIOContext)& theStream)
{
  Close();

  TCollection_AsciiString anExt = theInput;
  anExt.LowerCase();
  if (anExt.EndsWith(".png")
      || anExt.EndsWith(".jpg")
      || anExt.EndsWith(".jpeg")
      || anExt.EndsWith(".mpo")
      || anExt.EndsWith(".bmp")
      || anExt.EndsWith(".tif")
      || anExt.EndsWith(".tiff"))
  {
    // black-list images to workaround non-zero duration
    if (!OpenImageInput(theInput, theStream))
      return false;

  #ifdef HAVE_FFMPEG
    // retrieve stream information
    if (avformat_find_stream_info(myFormatCtx, nullptr) < 0)
    {
      Message::SendFail() << "FFmpeg: Couldn't find stream information in '" << theInput << "'";
      Close();
      return false;
    }
  #endif
    return true;
  }

  myIOContext = theStream;
#ifdef HAVE_FFMPEG
    myFormatCtx = avformat_alloc_context();
    if (myIOContext.get() != nullptr)
      myFormatCtx->pb = myIOContext->GetAvioContext();

  const int avErrCode = avformat_open_input (&myFormatCtx, theInput.ToCString(), NULL, NULL);
  if (avErrCode != 0)
  {
    Message::SendFail (TCollection_AsciiString ("FFmpeg: Couldn't open video file '") + theInput
                     + "'\nError: " + FormatAVErrorDescription (avErrCode));
    Close();
    return false;
  }

  // retrieve stream information
  if (avformat_find_stream_info (myFormatCtx, NULL) < 0)
  {
    Message::SendFail (TCollection_AsciiString ("FFmpeg: Couldn't find stream information in '") + theInput + "'");
    Close();
    return false;
  }

#ifdef _DEBUG
  av_dump_format (myFormatCtx, 0, theInput.ToCString(), false);
#endif

  myDuration = 0.0;
  myPtsStartBase = 0.0;

  myDuration = FormatUnitsToSeconds (myFormatCtx->duration);
  if (myFormatCtx->nb_streams != 0)
  {
    myPtsStartBase = 2.e+100;
    for (unsigned int aStreamId = 0; aStreamId < myFormatCtx->nb_streams; ++aStreamId)
    {
      const AVStream& aStream = *myFormatCtx->streams[aStreamId];
      myPtsStartBase = Min (myPtsStartBase, StreamUnitsToSeconds (aStream, aStream.start_time));
      myDuration     = Max (myDuration,     StreamUnitsToSeconds (aStream, aStream.duration));
    }
  }

  return true;
#else
  Message::SendFail ("Error: FFmpeg library is unavailable");
  (void )theInput;
  return false;
#endif
}

// =======================================================================
// function : OpenImageInput
// purpose  :
// =======================================================================
bool Media_FormatContext::OpenImageInput(const TCollection_AsciiString& theInput,
                                         const Handle(Media_AVIOContext)& theStream)
{
  Close();

  myIOContext = theStream;
#ifdef HAVE_FFMPEG
  static const AVInputFormat* anImg2Format     = av_find_input_format("image2");
  static const AVInputFormat* anImg2PipeFormat = av_find_input_format("image2pipe");
  const AVInputFormat*        anImgFormat      = myIOContext.get() != nullptr ? anImg2Format : anImg2PipeFormat;

  myFormatCtx = avformat_alloc_context();
  if (myIOContext.get() != nullptr)
    myFormatCtx->pb = myIOContext->GetAvioContext();

  const int avErrCode = avformat_open_input(&myFormatCtx, theInput.ToCString(), anImgFormat, nullptr);
  if (avErrCode != 0)
  {
    Message::SendFail(TCollection_AsciiString ("FFmpeg: Couldn't open video file '") + theInput
                    + "'\nError: " + FormatAVErrorDescription (avErrCode));
    Close();
    return false;
  }

  myDuration = 0.0;
  myPtsStartBase = 0.0;
  return true;
#else
  Message::SendFail("Error: FFmpeg library is unavailable");
  (void)theInput;
  return false;
#endif
}

// =======================================================================
// function : Close
// purpose  :
// =======================================================================
void Media_FormatContext::Close()
{
  if (myFormatCtx != NULL)
  {
  #ifdef HAVE_FFMPEG
    avformat_close_input (&myFormatCtx);
    //avformat_free_context (myFormatCtx);
  #endif
  }
  myIOContext.Nullify();
}

// =======================================================================
// function : FormatTime
// purpose  :
// =======================================================================
TCollection_AsciiString Media_FormatContext::FormatTime (double theSeconds)
{
  double aSecIn = theSeconds;
  unsigned int aHours   = (unsigned int )(aSecIn * THE_SECOND_IN_HOUR);
  aSecIn -= double(aHours) * THE_SECONDS_IN_HOUR;
  unsigned int aMinutes = (unsigned int )(aSecIn * THE_SECOND_IN_MINUTE);
  aSecIn -= double(aMinutes) * THE_SECONDS_IN_MINUTE;
  unsigned int aSeconds = (unsigned int )aSecIn;
  aSecIn -= double(aSeconds);
  double aMilliSeconds = 1000.0 * aSecIn;

  char aBuffer[64];
  if (aHours > 0)
  {
    Snprintf (aBuffer, "%02u:%02u:%02u", aHours, aMinutes, aSeconds);
    return aBuffer;
  }
  else if (aMinutes > 0)
  {
    Snprintf (aBuffer, "%02u:%02u", aMinutes, aSeconds);
    return aBuffer;
  }
  else if (aSeconds > 0)
  {
    Snprintf (aBuffer, "%2u s", aSeconds);
    return aBuffer;
  }

  return TCollection_AsciiString (aMilliSeconds) + " ms";
}

// =======================================================================
// function : FormatTimeProgress
// purpose  :
// =======================================================================
TCollection_AsciiString Media_FormatContext::FormatTimeProgress (double theProgress,
                                                                 double theDuration)
{
  double aSecIn1 = theProgress;
  unsigned int aHours1   = (unsigned int )(aSecIn1 * THE_SECOND_IN_HOUR);
  aSecIn1 -= double(aHours1) * THE_SECONDS_IN_HOUR;
  unsigned int aMinutes1 = (unsigned int )(aSecIn1 * THE_SECOND_IN_MINUTE);
  aSecIn1 -= double(aMinutes1) * THE_SECONDS_IN_MINUTE;
  unsigned int aSeconds1 = (unsigned int )aSecIn1;
  aSecIn1 -= double(aSeconds1);

  double aSecIn2 = theDuration;
  unsigned int aHours2   = (unsigned int )(aSecIn2 * THE_SECOND_IN_HOUR);
  aSecIn2 -= double(aHours2) * THE_SECONDS_IN_HOUR;
  unsigned int aMinutes2 = (unsigned int )(aSecIn2 * THE_SECOND_IN_MINUTE);
  aSecIn2 -= double(aMinutes2) * THE_SECONDS_IN_MINUTE;
  unsigned int aSeconds2 = (unsigned int )aSecIn2;
  aSecIn2 -= double(aSeconds2);

  char aBuffer[256];
  if (aHours1 > 0
   || aHours2 > 0)
  {
    Snprintf (aBuffer, "%02u:%02u:%02u / %02u:%02u:%02u", aHours1, aMinutes1, aSeconds1, aHours2, aMinutes2, aSeconds2);
    return aBuffer;
  }
  Snprintf (aBuffer, "%02u:%02u / %02u:%02u", aMinutes1, aSeconds1, aMinutes2, aSeconds2);
  return aBuffer;
}

// =======================================================================
// function : StreamInfo
// purpose  :
// =======================================================================
TCollection_AsciiString Media_FormatContext::StreamInfo (unsigned int theIndex,
                                                         AVCodecContext* theCodecCtx) const
{
#ifdef HAVE_FFMPEG
  const AVStream& aStream = *myFormatCtx->streams[theIndex];

  AVCodecContext* aCodecCtx = theCodecCtx;
  if (aCodecCtx == NULL)
  {
#if(LIBAVFORMAT_VERSION_INT < AV_VERSION_INT(59, 0, 100))
  Standard_DISABLE_DEPRECATION_WARNINGS
    aCodecCtx = aStream.codec;
  Standard_ENABLE_DEPRECATION_WARNINGS
#endif
  }

  char aFrmtBuff[4096] = {};
  avcodec_string (aFrmtBuff, sizeof(aFrmtBuff), aCodecCtx, 0);
  TCollection_AsciiString aStreamInfo (aFrmtBuff);

  if (aStream.sample_aspect_ratio.num && av_cmp_q(aStream.sample_aspect_ratio, aStream.codecpar->sample_aspect_ratio))
  {
    AVRational aDispAspectRatio;
    av_reduce (&aDispAspectRatio.num, &aDispAspectRatio.den,
               aStream.codecpar->width  * int64_t(aStream.sample_aspect_ratio.num),
               aStream.codecpar->height * int64_t(aStream.sample_aspect_ratio.den),
               1024 * 1024);
    aStreamInfo = aStreamInfo + ", SAR " + aStream.sample_aspect_ratio.num + ":" + aStream.sample_aspect_ratio.den
                + " DAR " + aDispAspectRatio.num + ":" + aDispAspectRatio.den;
  }

  if (aStream.codecpar->codec_type == AVMEDIA_TYPE_VIDEO)
  {
    if (aStream.avg_frame_rate.den != 0 && aStream.avg_frame_rate.num != 0)
    {
      aStreamInfo += TCollection_AsciiString(", ") + formatFps (av_q2d (aStream.avg_frame_rate)) + " fps";
    }
    if (aStream.r_frame_rate.den != 0 && aStream.r_frame_rate.num != 0)
    {
      aStreamInfo += TCollection_AsciiString(", ") + formatFps (av_q2d (aStream.r_frame_rate)) + " tbr";
    }
    if (aStream.time_base.den != 0 && aStream.time_base.num != 0)
    {
      aStreamInfo += TCollection_AsciiString(", ") + formatFps(1 / av_q2d (aStream.time_base)) + " tbn";
    }
    if (aCodecCtx->time_base.den != 0 && aCodecCtx->time_base.num != 0)
    {
      aStreamInfo += TCollection_AsciiString(", ") + formatFps(1 / av_q2d (aCodecCtx->time_base)) + " tbc";
    }
  }
  if (myDuration > 0.0)
  {
    aStreamInfo += TCollection_AsciiString(", duration: ") + FormatTime (myDuration);
  }
  return aStreamInfo;
#else
  (void )theIndex;
  (void )theCodecCtx;
  return TCollection_AsciiString();
#endif
}

// =======================================================================
// function : ReadPacket
// purpose  :
// =======================================================================
bool Media_FormatContext::ReadPacket (const Handle(Media_Packet)& thePacket)
{
  if (thePacket.IsNull())
  {
    return false;
  }

#ifdef HAVE_FFMPEG
  return av_read_frame (myFormatCtx, thePacket->ChangePacket()) >= 0;
#else
  return false;
#endif
}

// =======================================================================
// function : SeekStream
// purpose  :
// =======================================================================
bool Media_FormatContext::SeekStream (unsigned int theStreamId,
                                      double theSeekPts,
                                      bool theToSeekBack)
{
#ifdef HAVE_FFMPEG
  const int aFlags = theToSeekBack ? AVSEEK_FLAG_BACKWARD : 0;
  AVStream& aStream = *myFormatCtx->streams[theStreamId];
  if ((aStream.disposition & AV_DISPOSITION_ATTACHED_PIC) != 0)
  {
    return false;
  }

  int64_t aSeekTarget = StreamSecondsToUnits (aStream, theSeekPts + StreamUnitsToSeconds (aStream, aStream.start_time));
  bool isSeekDone = av_seek_frame (myFormatCtx, theStreamId, aSeekTarget, aFlags) >= 0;

#if(LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(59, 3, 100))
  /// TODO AVStream::cur_dts has been removed without libavformat version bump and without entry in APIchanges
  /// with comment "no reason to have them exposed in a public header"
#else
  // try 10 more times in backward direction to work-around huge duration between key frames
  // will not work for some streams with undefined cur_dts (AV_NOPTS_VALUE)!!!
  for (int aTries = 10; isSeekDone && theToSeekBack && aTries > 0 && (aStream.cur_dts > aSeekTarget); --aTries)
  {
    aSeekTarget -= StreamSecondsToUnits (aStream, 1.0);
    isSeekDone = av_seek_frame (myFormatCtx, theStreamId, aSeekTarget, aFlags) >= 0;
  }
#endif

  if (isSeekDone)
  {
    return true;
  }

  TCollection_AsciiString aStreamType = aStream.codecpar->codec_type == AVMEDIA_TYPE_VIDEO
                                      ? "Video"
                                      : (aStream.codecpar->codec_type == AVMEDIA_TYPE_AUDIO
                                       ? "Audio"
                                       : "");
  Message::SendWarning (TCollection_AsciiString ("Error while seeking ") + aStreamType + " stream to "
                      + theSeekPts + " sec (" + (theSeekPts + StreamUnitsToSeconds (aStream, aStream.start_time)) + " sec)");
  return false;
#else
  (void )theStreamId;
  (void )theSeekPts;
  (void )theToSeekBack;
  return false;
#endif
}

// =======================================================================
// function : Seek
// purpose  :
// =======================================================================
bool Media_FormatContext::Seek (double theSeekPts,
                                bool   theToSeekBack)
{
#ifdef HAVE_FFMPEG
  const int aFlags      = theToSeekBack ? AVSEEK_FLAG_BACKWARD : 0;
  int64_t   aSeekTarget = SecondsToUnits (theSeekPts);
  if (av_seek_frame (myFormatCtx, -1, aSeekTarget, aFlags) >= 0)
  {
    return true;
  }

  const char* aFileName = 
  #if(LIBAVFORMAT_VERSION_INT >= AV_VERSION_INT(58, 7, 100))
    myFormatCtx->url;
  #else
    myFormatCtx->filename;
  #endif

  Message::SendWarning (TCollection_AsciiString("Disaster! Seeking to ") + theSeekPts + " [" + aFileName + "] has failed.");
  return false;
#else
  (void )theSeekPts;
  (void )theToSeekBack;
  return false;
#endif
}
