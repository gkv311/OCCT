// Created on: 1997-06-19
// Created by: Christophe LEYNADIER
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

#ifndef _Standard_GUID_HeaderFile
#define _Standard_GUID_HeaderFile

#include <Standard_Handle.hxx>

#include <Standard_Integer.hxx>
#include <Standard_CString.hxx>
#include <Standard_UUID.hxx>
#include <Standard_PCharacter.hxx>
#include <Standard_PExtCharacter.hxx>
#include <Standard_OStream.hxx>
#include <Standard_RangeError.hxx>

#define Standard_GUID_SIZE 36
#define Standard_GUID_SIZE_ALLOC Standard_GUID_SIZE+1

//! Class defining Globally Unique Identifier (GUID).
class Standard_GUID 
{
public:
  //! Check the format of a GUID string.
  //! It checks the size, the position of the '-' and the correct size of fields.
  template<typename T>
  static constexpr bool CheckGUIDFormat (const T* theGuid);

public:

  DEFINE_STANDARD_ALLOC

  //! Empty constructor.
  constexpr Standard_GUID() {}

  //! Build a GUID from an 36 characters string with the following format:
  //! "00000000-0000-0000-0000-000000000000".
  //! @throws Standard_RangeError in case of invalid format
  constexpr Standard_GUID(const char* theGuid);

  //! Build a GUID from a wide string with the following format:
  //! "00000000-0000-0000-0000-000000000000".
  //! @throws Standard_RangeError in case of invalid format
  constexpr Standard_GUID(const Standard_ExtString theGuid);

  //! Build a GUID from the given values.
  constexpr Standard_GUID(const Standard_Integer the32b,
                          const Standard_ExtCharacter the16b1, const Standard_ExtCharacter the16b2, const Standard_ExtCharacter the16b3,
                          const Standard_Byte the8b1, const Standard_Byte the8b2, const Standard_Byte the8b3,
                          const Standard_Byte the8b4, const Standard_Byte the8b5, const Standard_Byte the8b6);

  //! Copy constructor.
  Standard_GUID(const Standard_GUID& theGuid) = default;

  //! Build a GUID from UUID structure.
  Standard_GUID(const Standard_UUID& theUUID) { Assign (theUUID); }

  //! Return UUID structure from GUID.
  Standard_UUID ToUUID() const;

  //! translate the GUID into ascii string
  //! the aStrGuid is allocated by user.
  //! the guid have the following format:
  //!
  //! "00000000-0000-0000-0000-000000000000"
  Standard_EXPORT void ToCString (char* theBuffer, const size_t theSize) const;

  //! translate the GUID into unicode string
  //! the aStrGuid is allocated by user.
  //! the guid have the following format:
  //!
  //! "00000000-0000-0000-0000-000000000000"
  Standard_EXPORT void ToExtString (Standard_ExtCharacter* theBuffer, const size_t theSize) const;
  
  Standard_DEPRECATED("The size of a buffer should be provided")
  void ToCString (const Standard_PCharacter theBuffer) const
  {
    ToCString (theBuffer, Standard_GUID_SIZE_ALLOC);
  }

  Standard_DEPRECATED("The size of a buffer should be provided")
  void ToExtString (const Standard_PExtCharacter theBuffer) const
  {
    ToExtString(theBuffer, Standard_GUID_SIZE_ALLOC);
  }

  constexpr bool IsSame (const Standard_GUID& theGuid) const;
  constexpr bool operator== (const Standard_GUID& theGuid) const { return IsSame(theGuid); }

  constexpr bool IsNotSame (const Standard_GUID& theGuid) const { return !IsSame(theGuid); }
  constexpr bool operator!= (const Standard_GUID& theGuid) const { return !IsSame(theGuid); }

  void Assign (const Standard_GUID& theGuid) { *this = theGuid; }
  Standard_GUID& operator= (const Standard_GUID& theGuid) = default;

  void Assign (const Standard_UUID& theUuid);
  Standard_GUID& operator= (const Standard_UUID& theUuid)
  {
    Assign(theUuid);
    return *this;
  }

  //! Display the GUID with the following format:
  //!
  //! "00000000-0000-0000-0000-000000000000"
  Standard_EXPORT void ShallowDump (Standard_OStream& aStream) const;

  //! Hash function for GUID.
  Standard_EXPORT Standard_Integer Hash (const Standard_Integer Upper) const;

  //! Computes a hash code for the given GUID of the Standard_Integer type, in the range [1, theUpperBound]
  //! @param theGUID the GUID which hash code is to be computed
  //! @param theUpperBound the upper bound of the range a computing hash code must be within
  //! @return a computed hash code, in the range [1, theUpperBound]
  static Standard_Integer HashCode (const Standard_GUID& theGUID, Standard_Integer theUpperBound)
  {
    return theGUID.Hash(theUpperBound);
  }

  //! Returns True  when the two GUID are the same.
  static bool IsEqual (const Standard_GUID& theGuid1, const Standard_GUID& theGuid2)
  {
    return theGuid1 == theGuid2;
  }

private:

  //! Check if input char is hexademical letter.
  //! Similar to std::isxdigit() but constexpr.
  static constexpr bool isXDigit(const int theC)
  {
    return (theC >= '0' && theC <= '9')
        || (theC >= 'A' && theC <= 'F')
        || (theC >= 'a' && theC <= 'f');
  }

  //! Parse a pair of hexademical letters defining a single byte value.
  template<typename T>
  static constexpr bool isX8(const T* theGuid, int thePos)
  {
    return isXDigit(theGuid[thePos]) && isXDigit(theGuid[thePos + 1]);
  }

  //! Parse 4 hexademical letters defining a value of 2 bytes.
  template<typename T>
  static constexpr bool isX16(const T* theGuid, int thePos)
  {
    return isX8(theGuid, thePos + 0) && isX8(theGuid, thePos + 2);
  }

  //! Parse 8 hexademical letters defining a value of 4 bytes.
  template<typename T>
  static constexpr bool isX32(const T* theGuid, int thePos)
  {
    return isX16(theGuid, thePos) && isX16(theGuid, thePos + 4);
  }

  //! Parse single hexademical letter.
  static constexpr unsigned int parseXDigit(const int theC)
  {
    return (theC >= 'a') ? (theC - 'a' + 10)
         : ((theC >= 'A') ? (theC - 'A' + 10)
          : (unsigned int)(theC) - '0');
  }

  //! Parse a pair of hexademical letters defining a single byte value.
  //! Could be replaced by std::from_chars() from C++17.
  template<typename T>
  static constexpr uint8_t parseX8(const T* theGuid, int thePos)
  {
    return uint8_t(parseXDigit(theGuid[thePos]) * 16u + parseXDigit(theGuid[thePos + 1]));
  }

  //! Parse 4 hexademical letters defining a value of 2 bytes.
  //! Could be replaced by std::from_chars() from C++17.
  template<typename T>
  static constexpr uint16_t parseX16(const T* theGuid, int thePos)
  {
    return uint16_t(parseX8(theGuid, thePos + 0)) * 16u * 16u
         + uint16_t(parseX8(theGuid, thePos + 2));
  }

  //! Parse 8 hexademical letters defining a value of 4 bytes.
  //! Could be replaced by std::from_chars() from C++17.
  template<typename T>
  static constexpr uint32_t parseX32(const T* theGuid, int thePos)
  {
    return uint32_t(parseX16(theGuid, thePos)) * 16u * 16u * 16u * 16u
         + uint32_t(parseX16(theGuid, thePos + 4));
  }

  //! Non constexpr helper to throw exception or to generate compiler error in constexpr expression.
  static int invalidFormat()
  {
    throw Standard_RangeError("Invalid format of GUID");
  }

private:

  Standard_Integer my32b = 0;
  Standard_ExtCharacter my16b1 = 0;
  Standard_ExtCharacter my16b2 = 0;
  Standard_ExtCharacter my16b3 = 0;
  Standard_Byte my8b1 = 0;
  Standard_Byte my8b2 = 0;
  Standard_Byte my8b3 = 0;
  Standard_Byte my8b4 = 0;
  Standard_Byte my8b5 = 0;
  Standard_Byte my8b6 = 0;

};

//============================================================================
// function : CheckGUIDFormat
//============================================================================
template<typename T>
constexpr bool Standard_GUID::CheckGUIDFormat (const T* theGuid)
{
  return theGuid != nullptr
      && isX32(theGuid, 0) && theGuid[8] == '-'
      && isX16(theGuid, 9) && theGuid[13] == '-'
      && isX16(theGuid, 14) && theGuid[18] == '-'
      && isX16(theGuid, 19) && theGuid[23] == '-'
      && isX8(theGuid, 24) && isX8(theGuid, 26) && isX8(theGuid, 28)
      && isX8(theGuid, 30) && isX8(theGuid, 32) && isX8(theGuid, 34)
      && theGuid[36] == '\0';
}

//============================================================================
// function : Standard_GUID
//============================================================================
constexpr Standard_GUID::Standard_GUID(const char* theGuid)
: my32b(CheckGUIDFormat(theGuid) ? parseX32(theGuid, 0) : invalidFormat()),
  my16b1(parseX16(theGuid, 9)),
  my16b2(parseX16(theGuid, 14)),
  my16b3(parseX16(theGuid, 19)),
  my8b1(parseX8(theGuid, 24)),
  my8b2(parseX8(theGuid, 26)),
  my8b3(parseX8(theGuid, 28)),
  my8b4(parseX8(theGuid, 30)),
  my8b5(parseX8(theGuid, 32)),
  my8b6(parseX8(theGuid, 34))
{
  //
}

//============================================================================
// function : Standard_GUID
//============================================================================
constexpr Standard_GUID::Standard_GUID(const Standard_ExtString theGuid)
: my32b(CheckGUIDFormat(theGuid) ? parseX32(theGuid, 0) : invalidFormat()),
  my16b1(parseX16(theGuid, 9)),
  my16b2(parseX16(theGuid, 14)),
  my16b3(parseX16(theGuid, 19)),
  my8b1(parseX8(theGuid, 24)),
  my8b2(parseX8(theGuid, 26)),
  my8b3(parseX8(theGuid, 28)),
  my8b4(parseX8(theGuid, 30)),
  my8b5(parseX8(theGuid, 32)),
  my8b6(parseX8(theGuid, 34))
{
  //
}

//============================================================================
// function : Standard_GUID
//============================================================================
constexpr Standard_GUID::Standard_GUID(const Standard_Integer the32b,
                                       const Standard_ExtCharacter the16b1,
                                       const Standard_ExtCharacter the16b2,
                                       const Standard_ExtCharacter the16b3,
                                       const Standard_Byte the8b1, const Standard_Byte the8b2, const Standard_Byte the8b3,
                                       const Standard_Byte the8b4, const Standard_Byte the8b5, const Standard_Byte the8b6)
: my32b(the32b),
  my16b1(the16b1), my16b2(the16b2), my16b3(the16b3),
  my8b1(the8b1), my8b2(the8b2), my8b3(the8b3),
  my8b4(the8b4), my8b5(the8b5), my8b6(the8b6)
{
  //
}

//============================================================================
// function : IsSame
//============================================================================
constexpr bool Standard_GUID::IsSame(const Standard_GUID& uid) const
{
  return my32b  == uid.my32b
      && my16b1 == uid.my16b1 && my16b2 == uid.my16b2 && my16b3 == uid.my16b3
      && my8b1  == uid.my8b1 && my8b2  == uid.my8b2 && my8b3  == uid.my8b3
      && my8b4  == uid.my8b4 && my8b5  == uid.my8b5 && my8b6  == uid.my8b6;
}

//============================================================================
// function : Assign
//============================================================================
inline void Standard_GUID::Assign (const Standard_UUID& theUUID)
{
  my32b  = theUUID.Data1;
  my16b1 = theUUID.Data2;
  my16b2 = theUUID.Data3;
  my16b3 = (theUUID.Data4[0] << 8) | (theUUID.Data4[1]);
  my8b1  = theUUID.Data4[2];
  my8b2  = theUUID.Data4[3];
  my8b3  = theUUID.Data4[4];
  my8b4  = theUUID.Data4[5];
  my8b5  = theUUID.Data4[6];
  my8b6  = theUUID.Data4[7];
}

//============================================================================
// function : ToUUID
//============================================================================
inline Standard_UUID Standard_GUID::ToUUID() const
{
  Standard_UUID result;
  result.Data1 = my32b;
  result.Data2 = my16b1;
  result.Data3 = my16b2;
  result.Data4[0] = (unsigned char)(my16b3 >> 8);
  result.Data4[1] = (char) my16b3;
  result.Data4[2] = my8b1;
  result.Data4[3] = my8b2;
  result.Data4[4] = my8b3;
  result.Data4[5] = my8b4;
  result.Data4[6] = my8b5;
  result.Data4[7] = my8b6;
  return result;
}

#endif // _Standard_GUID_HeaderFile
