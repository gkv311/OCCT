// Created on: 1995-10-25
// Created by: Christian CAILLET
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

#ifndef _StepData_EnumTool_HeaderFile
#define _StepData_EnumTool_HeaderFile

#include <TCollection_AsciiString.hxx>

#include <cstring>

//! This class gives a way of conversion between the value of an
//! enumeration and its representation in STEP.
//! An enumeration corresponds to an integer with reserved values, which begin to 0.
//! In STEP, it is represented by a name in capital letter and limited by two dots, e.g. '.UNKNOWN.'
//!
//! StepData_EnumTool works with integers, it is just required to cast
//! between an integer and an enumeration of required type.
//!
//! Its definition is intended to allow static creation in once,
//! without having to recreate once for each use.
//!
//! It is possible to define subclasses on it, which directly give
//! the good list of definition texts, and accepts a enumeration
//! of the good type instead of an integer.
class StepData_EnumTool 
{
public:

  //! Auxiliary structure defining string with its length.
  struct StringView
  {
    const char*  String = nullptr;
    const size_t Size = 0;

    template<size_t N>
    constexpr StringView(const char (&theStr)[N]) : String(theStr), Size(N) {}
  };

public:

  //! Empty constructor.
  constexpr StepData_EnumTool() {}

  //! Creates an EnumTool with definitions given static array of strings.
  //! Each term corresponds to one value of the enumeration, with two dots around for STEP.
  //!
  //! Such a static constructor allows to build a static description as:
  //! @code
  //!   static constexpr StepData_EnumTool::StringView EnumValues[] = {".e0.",".e1."..., ".eN."};
  //!   static constexpr StepData_EnumTool myenumtool(EnumValues);
  //! @endcode
  //!
  //! A null definition can be input by given "$":
  //! the corresponding position is attached to "null/undefined" value
  //! (as one particular item of the enumeration list).
  template<int N>
  constexpr StepData_EnumTool(const StringView (&theList)[N])
  : myValues(theList), myExtent(N)
  {
    //
  }

  //! Returns the maximum integer for a suitable value.
  //! Remark: while values begin at zero, MaxValue is the count of recorded values minus one
  Standard_Integer MaxValue() const { return myExtent - 1; }

  //! Returns the value attached to "null/undefined value".
  //! If none is specified or if Optional has been set to False, returns -1
  //! Null Value has been specified by definition "$".
  Standard_Integer NullValue() const { return myIsOpt ? Value("$") : 0; }

  //! Returns the text which corresponds to a given numeric value; it is limited by dots.
  //! If @p theNumber is out of range, returns an empty string.
  const char* Text (const Standard_Integer theNumber) const
  {
    if (theNumber >= 0 && theNumber < myExtent)
      return myValues[theNumber].String;

    return "";
  }

  //! Returns the numeric value found for a text.
  //! The text must be in capitals and limited by dots.
  //! A non-suitable text gives a negative value to be returned.
  Standard_Integer Value (const Standard_CString theText) const
  {
    const size_t aLen = std::strlen(theText) + 1;
    for (int anIndex = 0; anIndex < myExtent; ++anIndex)
    {
      if (myValues[anIndex].Size == aLen
       && std::strncmp(myValues[anIndex].String, theText, aLen) == 0)
        return anIndex;
    }
    return -1;
  }

  //! Returns the enumeration value found for a text.
  //! @param[out] theValue retrieved enumeration value
  //! @param[in]  theText enumeration text
  //! @return FALSE if input text not found
  template<typename T>
  bool Value (T& theValue, const Standard_CString theText) const
  {
    const Standard_Integer aVal = Value(theText);
    if (aVal == -1)
      return false;

    theValue = static_cast<T>(aVal);
    return true;
  }

private:

  const StringView* myValues = nullptr;
  Standard_Integer myExtent = 0;
  Standard_Boolean myIsOpt = true;

};

#endif // _StepData_EnumTool_HeaderFile
