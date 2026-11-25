// Created on: 2002-02-01
// Created by: QA Admin
// Copyright (c) 2002-2014 OPEN CASCADE SAS
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

#ifndef _QADraw_HeaderFile
#define _QADraw_HeaderFile

#include <Draw_Interpretor.hxx>
#include <Message.hxx>

//! Draw Harness plugin defining non-general commands specific to test cases.
class QADraw 
{
public:

  DEFINE_STANDARD_ALLOC

  Standard_EXPORT static void CommonCommands (Draw_Interpretor& theCommands);

  Standard_EXPORT static void AdditionalCommands (Draw_Interpretor& theCommands);

  Standard_EXPORT static void TutorialCommands (Draw_Interpretor& theCommands);

  //! Loads all QA Draw commands. Used for plugin.
  Standard_EXPORT static void Factory (Draw_Interpretor& theCommands);

public:

  //! Auxiliary class for QA_ macros creating a message buffer
  //! for printing an OK or Error message based on performed check.
  class MessageBuffer : public Message_Messenger::StreamBuffer
  {
  public:
    //! Comparison operators.
    enum class Check { Lesser, LesserEqual, Equal, GreaterEqual, Greater };
    //! Comparison with tolerance.
    enum class CheckNear { Near };

    //! Constructor for Boolean exression.
    MessageBuffer(bool theIsOK, const char* theDesc) : MessageBuffer(theIsOK)
    {
      *this << "Checking (" << theDesc << ") ";
    }

    //! Constructor performing comparison between two given values.
    template<typename T1, typename T2>
    MessageBuffer(Check theCheck, const T1& theA, const char* theAStr, const T2& theB, const char* theBStr)
      : MessageBuffer(compareIsOK(theCheck, theA, theB))
    {
      *this << "Checking (" << theAStr << compareSymbol(theCheck) << theBStr << ")"
                                           << " --> (" << theA << compareSymbol(theCheck) << theB << ") ";
    }

    //! Constructor performing comparison between two given values with tolerance.
    template<typename T>
    MessageBuffer(CheckNear , const T& theA, const char* theAStr, const T& theB, const char* theBStr, const T& theTol, const char* theTolStr)
      : MessageBuffer(compareIsNear(theA, theB, theTol))
    {
      *this << "Checking Abs(" << theAStr << " - " << theBStr << ") <= " << theTolStr
                                           << " --> (" << theA << " - " << theB << ") <= " << theTol << " ";
    }

    //! Destructor adds OK or Error to the end and flushed message to the output.
    ~MessageBuffer()
    {
      *this << (myIsOK ? ": OK" : ": Error");
    }

  private:

    //! Private constructor.
    explicit MessageBuffer(bool theIsOK)
    : Message_Messenger::StreamBuffer(Message::Send(theIsOK ? Message_Info : Message_Fail)),
      myIsOK(theIsOK)
    {
      //
    }

    //! Return TRUE if two values are considered equal with the given tolerance.
    template<typename T>
    static bool compareNear(const T& theA, const T& theB, const T& theTol)
    {
      return std::abs(theA - theB) <= theTol;
    }

    //! Apply comparison operator on two given values and return result.
    template<typename T1, typename T2>
    static bool compareIsOK(Check theCheck, const T1& theA, const T2& theB)
    {
      switch (theCheck)
      {
        case Check::Lesser:       return theA < theB;
        case Check::LesserEqual:  return theA <= theB;
        case Check::Equal:        return theA == theB;
        case Check::GreaterEqual: return theA >= theB;
        case Check::Greater:      return theA > theB;
      }
      return false;
    }

    //! Return comparison operator symbol.
    static const char* compareSymbol(Check theCheck)
    {
      switch (theCheck)
      {
        case Check::Lesser:       return " < ";
        case Check::LesserEqual:  return " <= ";
        case Check::Equal:        return " == ";
        case Check::GreaterEqual: return " >= ";
        case Check::Greater:      return " > ";
      }
      return "?";
    }

  private:
    bool myIsOK = false;

  };

};

//! @def QA_ASSERT_TRUE
//! Assertion alias for TRUE epxression.
#define QA_ASSERT_TRUE(theOK) Standard_ASSERT_RAISE(theOK, "")

//! @def QA_ASSERT_EQ
//! Assertion alias testing two values for equality.
#define QA_ASSERT_EQ(theA, theB) Standard_ASSERT_RAISE((theA) == (theB) , "")

//! @def QA_EXPECT_TRUE
//! For unit-tests. Checks if the given expression is TRUE and prints OK or Error status.
//! Usage example:
//! @code
//!   TopoDS_Shape theShape = ...;
//!   QA_EXPECT_TRUE(!theShape.IsNull()) << "The shape should not be NULL"
//! @endcode
#define QA_EXPECT_TRUE(theOK) \
  QADraw::MessageBuffer((theOK), #theOK)

//! @def QA_EXPECT_LT
//! For unit-tests. Checks if the first value is lesser than the second one and prints OK or Error status.
#define QA_EXPECT_LT(theA, theB) \
  QADraw::MessageBuffer(QADraw::MessageBuffer::Check::Lesser, (theA), #theA, (theB), #theB)

//! @def QA_EXPECT_LE
//! For unit-tests. Checks if the first value is lesser or equal to the second one and prints OK or Error status.
#define QA_EXPECT_LE(theA, theB) \
  QADraw::MessageBuffer(QADraw::MessageBuffer::Check::LesserEqual, (theA), #theA, (theB), #theB)

//! @def QA_EXPECT_EQ
//! For unit-tests. Checks if the first value is equal to the second one and prints OK or Error status.
#define QA_EXPECT_EQ(theA, theB) \
  QADraw::MessageBuffer(QADraw::MessageBuffer::Check::Equal, (theA), #theA, (theB), #theB)

//! @def QA_EXPECT_GE
//! For unit-tests. Checks if the first value is greater or equal to the second one and prints OK or Error status.
#define QA_EXPECT_GE(theA, theB) \
  QADraw::MessageBuffer(QADraw::MessageBuffer::Check::GreaterEqual, (theA), #theA, (theB), #theB)

//! @def QA_EXPECT_GT
//! For unit-tests. Checks if the first value is greater than the second one and prints OK or Error status.
#define QA_EXPECT_GT(theA, theB) \
  QADraw::MessageBuffer(QADraw::MessageBuffer::Check::Greater, (theA), #theA, (theB), #theB)

//! @def QA_EXPECT_NEAR
//! For unit-tests. Checks if two values are equal with the given tolerance and prints OK or Error status.
#define QA_EXPECT_NEAR(theA, theB, theTol) \
  QADraw::MessageBuffer(QADraw::MessageBuffer::CheckNear::Near, (theA), #theA, (theB), #theB, (theTol), #theTol)

#endif // _QADraw_HeaderFile
