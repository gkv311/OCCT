// Created on: 2013-01-29
// Created by: Kirill GAVRILOV
// Copyright (c) 2013-2014 OPEN CASCADE SAS
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

#ifndef Font_TextFormatter_Header
#define Font_TextFormatter_Header

#include <Font_Rect.hxx>
#include <Graphic3d_HorizontalTextAlignment.hxx>
#include <Graphic3d_VerticalTextAlignment.hxx>
#include <NCollection_Vector.hxx>
#include <NCollection_String.hxx>

class Font_FTFont;

DEFINE_STANDARD_HANDLE(Font_TextFormatter, Standard_Transient)

//! This class is intended to prepare formatted text by using:
//! - font to string combination,
//! - alignment,
//! - wrapping.
//!
//! After formatting, each symbol of formatted text is placed in some position.
//! Further work with the formatter is using an iterator.
//! The iterator gives an access to each symbol inside the initial row.
//! Also it's possible to get only significant/writable symbols of the text.
//! Formatter gives an access to geometrical position of a symbol by the symbol index in the text.
//!
//! Example of correspondence of some text symbol to an index in "row_1\n\nrow_2\n":
//! - "row_1\n"  - 0-5 indices;
//! - "\n"       - 6 index;
//! - "\n"       - 7 index;
//! - "row_2\n"  - 8-13 indices.
//!
//! Example of the formatter using:
//! @code
//!   Handle(Font_TextFormatter) aFormatter = new Font_TextFormatter();
//!   ... // setting of additional properties such as wrapping or alignment
//!   aFormatter->Append("text_1", aFont1);
//!   aFormatter->Append("text_2", aFont2);
//!   aFormatter->Format();
//! @endcode
class Font_TextFormatter : public Standard_Transient
{
public:

  //! Returns true if the symbol is CR, BEL, FF, NP, BS or VT
  static bool IsCommandSymbol(Standard_Utf32Char theSymbol)
  {
    return theSymbol == '\x0D' // CR  (carriage return)
        || theSymbol == '\a'   // BEL (alarm)
        || theSymbol == '\f'   // FF  (form feed) NP (new page)
        || theSymbol == '\b'   // BS  (backspace)
        || theSymbol == '\v';  // VT  (vertical tab)
  }

  //! Returns true if the symbol separates words when wrapping is enabled
  static bool IsSeparatorSymbol(Standard_Utf32Char theSymbol)
  {
    return theSymbol == '\x0A' // new line
        || theSymbol == ' '    // space
        || theSymbol == '\t';  // tab
  }

public:
  //! Iteration filter flags. Command symbols are skipped with any filter.
  enum IterationFilter
  {
    IterationFilter_None             = 0x0000, //!< no filter
    IterationFilter_ExcludeInvisible = 0x0002, //!< exclude ' ', '\t', '\n'
  };

  //! Iterator through formatted symbols.
  //! It's possible to filter returned symbols to have only significant ones.
  class Iterator
  {
  public:
    //! Constructor with initialization.
    Iterator (const Font_TextFormatter& theFormatter,
              IterationFilter theFilter = IterationFilter_None)
    : myFilter (theFilter), myIter (theFormatter.myString.Iterator()), mySymbolChar (0), mySymbolCharNext (0)
    {
      mySymbolPosition = readNextSymbol (-1, mySymbolChar);
      mySymbolNext = readNextSymbol (mySymbolPosition, mySymbolCharNext);
    }

    //! Returns TRUE if iterator points to a valid item.
    bool More() const { return mySymbolPosition >= 0; }

    //! Returns TRUE if next item exists
    bool HasNext() const { return mySymbolNext >= 0; }

    //! Returns current symbol.
    Standard_Utf32Char Symbol() const { return mySymbolChar; }

    //! Returns the next symbol if exists.
    Standard_Utf32Char SymbolNext() const { return mySymbolCharNext; }

    //! Returns current symbol position.
    int SymbolPosition() const { return mySymbolPosition; }

    //! Returns the next symbol position.
    int SymbolPositionNext() const { return mySymbolNext; }

    //! Moves to the next item.
    void Next()
    {
      mySymbolPosition = mySymbolNext;
      mySymbolChar = mySymbolCharNext;
      mySymbolNext = readNextSymbol (mySymbolPosition, mySymbolCharNext);
    }

  protected:
    //! Finds index of the next symbol
    inline int readNextSymbol(const int theSymbolStartingFrom, Standard_Utf32Char& theSymbolChar);

  protected:
    IterationFilter      myFilter;         //!< possibility to filter not-necessary symbols
    NCollection_Utf8Iter myIter;           //!< the next symbol iterator value over the text formatter string
    int                  mySymbolPosition; //!< the current position
    Standard_Utf32Char   mySymbolChar;     //!< the current symbol
    int                  mySymbolNext;     //!< position of the next symbol in iterator, if zero, the iterator is finished
    Standard_Utf32Char   mySymbolCharNext; //!< the current symbol
  };

  //! Scaling parameters.
  struct FontScaling
  {
    float SizeScaling  = 1.0f; //!< size scaling
    float SpaceScaling = 1.0f; //!< scale space between glyphs along X-axis
    float LineScaling  = 1.0f; //!< scale line space between glyphs along Y-axis
  };

  //! Text glyph definition.
  struct TextGlyph
  {
    //! The bottom left corner of a glyph on baseline.
    NCollection_Vec2<float> Pos;
    //! Character code.
    Standard_Utf32Char Char = 0;
    //! Width of a character
    float Width = 0.0f;

    TextGlyph() = default;
    TextGlyph(Standard_Utf32Char theChar, const NCollection_Vec2<float>& thePos, float theWidth)
    : Pos(thePos),
      Char(theChar),
      Width(theWidth)
    {
    }
  };

  //! Text line definition.
  struct TextLine
  {
    NCollection_Vec2<float> Pen; //!< position of the next symbol to put in the line

    float LineSpacing = 0.0f; //!< line spacing (the distance to the next line)
    float Ascender = 0.0f;
    float Descender = 0.0f;

    Graphic3d_HorizontalTextAlignment AlignX = Graphic3d_HTA_LEFT; //!< horizontal alignment style

    int GlyphLower = 0; //!< index of the first glyph in the line
    int GlyphUpper = -1; //!< index of the last glyph  in the line
    int NbIndentSymbols = 0; //!< counter to process tabulation symbols
    int NbGraphSymbols = 0; //!< counter to graphical symbols in the line (to process wrapping)
    int LastWordStart = 0; //!< index of the beginning of the last word in the line (for word wrapping)

    //! Update font parameters.
    Standard_EXPORT void Update(const Font_FTFont& theFont,
                                const Graphic3d_HorizontalTextAlignment theAlignX,
                                const FontScaling& theFontScale);

    //! Tighten up the right boundary - drop whitespaces.
    Standard_EXPORT void TrimRight(const NCollection_Vector<TextGlyph>& theGlyphs, const float theMaxWidth);
  };

public:

  //! Default constructor.
  Standard_EXPORT Font_TextFormatter();

  //! Returns horizontal alignment style
  Graphic3d_HorizontalTextAlignment HorizontalTextAlignment() const { return myAlignX; }

  //! Returns vertical alignment style
  Graphic3d_VerticalTextAlignment VerticalTextAlignment() const { return myAlignY; }

  //! Sets alignment style.
  void SetupAlignment(Graphic3d_HorizontalTextAlignment theAlignX,
                      Graphic3d_VerticalTextAlignment   theAlignY)
  {
    myAlignX = theAlignX;
    myAlignY = theAlignY;
  }

  //! Returns tab size.
  int TabSize() const { return myTabSize; }

  //! Sets tab size.
  void SetTabSize(int theSize) { myTabSize = theSize; }

  //! Returns text maximum width, zero means that the text is not bounded by width.
  float Wrapping() const { return myWrappingWidth; }

  //! Sets text wrapping width, zero means that the text is not bounded by width.
  void SetWrapping(float theWidth) { myWrappingWidth = theWidth; }

  //! Returns TRUE wrapping width is set.
  bool HasWrapping() const { return myWrappingWidth > 0.0f; }

  //! Returns TRUE when trying not to break words when wrapping text.
  bool WordWrapping() const { return myIsWordWrapping; }

  //! Sets flag to avoid breaking words when wrapping text.
  void SetWordWrapping(bool theIsWordWrapping) { myIsWordWrapping = theIsWordWrapping; }

public:

  //! Resets input buffer and formatting results.
  Standard_EXPORT virtual void Reset();

  //! Adds text to the buffer.
  //!
  //! At this step, the tool collects horizontal offsets of each glyph on a baseline (advances),
  //! arranges input text into lines, handles wrapping.
  //!
  //! @param[in] theString text to add
  //! @param[in] theFont   font for text properties
  //! @throw Standard_ProgramError if text has been already formatted by Format()
  void Append (const NCollection_String& theString,
               Font_FTFont&              theFont)
  {
    Append (theString, theFont, myAlignX, FontScaling());
  }

  //! Adds text to the buffer.
  //!
  //! At this step, the tool collects horizontal offsets of each glyphs on a baseline (advances),
  //! arranges input text into lines, handles wrapping.
  //!
  //! @param[in] theString text to add
  //! @param[in] theFont   font for text properties
  //! @param[in] theAlignX horizontal alignment style
  //! @param[in] theFontScale font scale factors
  //! @throw Standard_ProgramError if text has been already formatted by Format(
  Standard_EXPORT virtual void Append (const NCollection_String& theString,
                                       Font_FTFont& theFont,
                                       const Graphic3d_HorizontalTextAlignment theAlignX,
                                       const FontScaling& theFontScale);

  //! Performs formatting of the buffered text - apply horizontal and vertical alignment.
  Standard_EXPORT virtual void Format();

  //! Return formatting state.
  bool IsDone() const { return myIsFormatted; }

public:

  //! Returns specific glyph rectangle.
  const NCollection_Vec2<float>& BottomLeft (int theIndex) const
  { return myGlyphs[theIndex].Pos; }

  //! Returns buffered text.
  const NCollection_String& String() const { return myString; }

  //! Returns width of formatted text.
  float ResultWidth() const { return myBndSize.x(); }

  //! Returns height of formatted text.
  float ResultHeight() const { return myBndSize.y(); }

  //! Return bounding box
  Standard_EXPORT virtual Font_Rect BoundingBox() const;

  //! Returns maximum width of the text symbol
  float MaximumSymbolWidth() const { return myMaxSymbolWidth; }

  //! Return list of lines.
  const NCollection_List<TextLine>& ResultLines() const { return myLines; }

  //! Return formatted glyphs.
  const NCollection_Vector<TextGlyph>& ResultGlyphs() const { return myGlyphs; }

public:

  Standard_DEPRECATED("BoundingBox() should be used instead")
  void BndBox(Font_Rect& theBndBox) const { theBndBox = BoundingBox(); }

  DEFINE_STANDARD_RTTIEXT (Font_TextFormatter, Standard_Transient)

protected: //! @name configuration

  //! Horizontal alignment style.
  Graphic3d_HorizontalTextAlignment myAlignX = Graphic3d_HTA_LEFT;
  //! Vertical alignment style.
  Graphic3d_VerticalTextAlignment myAlignY = Graphic3d_VTA_TOP;
  //! Horizontal tabulation width (number of space symbols).
  int myTabSize = 8;
  //! Wrapping width.
  float myWrappingWidth = 0.0f;
  //! Avoid breaking words when wrapping (true by default).
  bool myIsWordWrapping = true;

protected: //! @name input data

  //! List of lines.
  NCollection_List<TextLine> myLines;
  //! The list of formatted glyphs.
  NCollection_Vector<TextGlyph> myGlyphs;
  //! Buffered text.
  NCollection_String myString;

  //! Text width and size.
  NCollection_Vec2<float> myBndSize;
  //! Maximum symbol width within formatted text.
  float myMaxSymbolWidth = 0.0f;
  //! Formatting state.
  bool  myIsFormatted = false;

};

// =======================================================================
// function : Iterator::readNextSymbol
// purpose :
// =======================================================================
inline int Font_TextFormatter::Iterator::readNextSymbol (const int theSymbolStartingFrom,
                                                         Standard_Utf32Char& theSymbolChar)
{
  int aNextSymbol = theSymbolStartingFrom;
  for (; *myIter != 0; ++myIter)
  {
    const Standard_Utf32Char aCharCurr = *myIter;
    if (Font_TextFormatter::IsCommandSymbol (aCharCurr))
      continue; // skip unsupported carriage control codes

    ++aNextSymbol;
    if ((myFilter & IterationFilter_ExcludeInvisible) != 0)
    {
      if (IsSeparatorSymbol(aCharCurr))
        continue;
    }
    ++myIter;
    theSymbolChar = aCharCurr;
    return aNextSymbol; // found the first next, not command and not filtered symbol
  }
  return -1; // the next symbol is not found
}

#endif // Font_TextFormatter_Header
