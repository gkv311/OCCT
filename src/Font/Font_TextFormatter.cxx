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

#include <Font_TextFormatter.hxx>

#include <Font_FTFont.hxx>

IMPLEMENT_STANDARD_RTTIEXT (Font_TextFormatter, Standard_Transient)

// =======================================================================
// function : Font_TextFormatter
// purpose  :
// =======================================================================
Font_TextFormatter::Font_TextFormatter()
{
  //
}

// =======================================================================
// function : Reset
// purpose :
// =======================================================================
void Font_TextFormatter::Reset()
{
  myIsFormatted = false;
  myString.Clear();
  myLines.Clear();
  myGlyphs.Clear();

  myMaxSymbolWidth = 0.0f;
  myBndSize.SetValues(0.0f, 0.0f);
}

// =======================================================================
// function : TextLine::Update
// purpose :
// =======================================================================
void Font_TextFormatter::TextLine::Update(const Font_FTFont&                      theFont,
                                          const Graphic3d_HorizontalTextAlignment theAlignX,
                                          const FontScaling&                      theFontScale)
{
  Ascender = Max(Ascender, theFont.Ascender() * theFontScale.SizeScaling);
  Descender = Min(Descender, theFont.Descender() * theFontScale.SizeScaling);
  LineSpacing = Max(LineSpacing, theFont.LineSpacing() * theFontScale.SizeScaling * theFontScale.LineScaling);
  AlignX = theAlignX;
}

// =======================================================================
// function : TextLine::TrimRight
// purpose :
// =======================================================================
void Font_TextFormatter::TextLine::TrimRight(const NCollection_Vector<TextGlyph>& theGlyphs, const float theMaxWidth)
{
  if (this->Pen.x() <= theMaxWidth)
    return;

  for (int aTail = this->GlyphUpper; aTail >= this->GlyphLower; --aTail)
  {
    const TextGlyph& aTailGlyph = theGlyphs[aTail];
    const bool       isSpace = Font_TextFormatter::IsSeparatorSymbol(aTailGlyph.Char);
    if (!isSpace)
    {
      this->Pen.x() = aTailGlyph.Pos.x() + aTailGlyph.Width;
      return;
    }
    else if (aTailGlyph.Pos.x() <= theMaxWidth)
    {
      this->Pen.x() = Max(aTailGlyph.Pos.x(), theMaxWidth);
      return;
    }

    this->Pen.x() = aTailGlyph.Pos.x();
  }
}

// =======================================================================
// function : Append
// purpose :
// =======================================================================
void Font_TextFormatter::Append(const NCollection_String& theString,
                                Font_FTFont& theFont,
                                const Graphic3d_HorizontalTextAlignment theAlignX,
                                const FontScaling& theFontScale)
{
  if (myIsFormatted)
    throw Standard_ProgramError("Font_TextFormatter::Append() called after Format() without Reset()");

  if (theString.IsEmpty())
    return;

  if (myLines.IsEmpty())
  {
    myLines.Append(TextLine());
    myLines.Last().GlyphLower = myGlyphs.Upper() + 1;
  }

  TextLine* aLine = &myLines.Last();
  aLine->Update(theFont, theAlignX, theFontScale);

  const int anOldLen = myString.Length();
  myString += theString;

  Font_TextFormatter::Iterator aFormatterIt(*this);
  for (; aFormatterIt.More(); aFormatterIt.Next())
  {
    if (aFormatterIt.SymbolPosition() >= anOldLen)
      break;
  }

  // first pass - render all symbols using associated font on single ZERO baseline
  const float aSpaceScaling = theFontScale.SizeScaling * theFontScale.SpaceScaling;
  for (; aFormatterIt.More(); aFormatterIt.Next())
  {
    const Standard_Utf32Char aCharThis = aFormatterIt.Symbol();
    const Standard_Utf32Char aCharNext = aFormatterIt.SymbolNext();
    if (IsCommandSymbol(aCharThis))
      continue; // skip carriage control codes

    bool  isNewline = (aCharThis == '\x0A'); // LF (line feed, new line)
    bool  isGraphSymbol = false;
    bool  toWrapWord = false;
    float anAdvanceX = 0.0f;

    const float aGlyphWidth = theFont.AdvanceX(aCharThis, '\n') * aSpaceScaling;
    if (aCharThis == ' ')
    {
      aLine->LastWordStart = myGlyphs.Upper() + 2;
      anAdvanceX = theFont.AdvanceX(' ', aCharNext) * aSpaceScaling;
    }
    else if (aCharThis == '\t')
    {
      aLine->LastWordStart = myGlyphs.Upper() + 2;
      const int aNbSpaces = (myTabSize - (aLine->NbIndentSymbols - 1) % myTabSize);
      anAdvanceX = theFont.AdvanceX(' ', aCharNext) * float(aNbSpaces) * aSpaceScaling;
      aLine->NbIndentSymbols += aNbSpaces;
    }
    else if (!isNewline)
    {
      isGraphSymbol = true;
      anAdvanceX = theFont.AdvanceX(aCharThis, aCharNext) * aSpaceScaling;

      if (HasWrapping() && aLine->NbGraphSymbols > 0)
      {
        if (!myIsWordWrapping || aLine->LastWordStart == aLine->GlyphLower)
        {
          if ((aLine->Pen.x() + aGlyphWidth) >= myWrappingWidth)
            isNewline = true; // symbol wrapping
        }
        else if (myIsWordWrapping && aLine->LastWordStart > aLine->GlyphLower)
        {
          if ((aLine->Pen.x() + aGlyphWidth) >= myWrappingWidth)
          {
            isNewline = true; // word wrapping
            toWrapWord = aLine->LastWordStart <= myGlyphs.Upper();
          }
        }
      }
    }

    if (toWrapWord)
    {
      // move word to the next line
      const int   aNbMoved = myGlyphs.Upper() - aLine->LastWordStart + 1;
      const float aMovePen = myGlyphs[aLine->LastWordStart].Pos.x();
      const float aNewlinePen = aLine->Pen.x() - aMovePen;
      for (int aWordGlyph = aLine->LastWordStart; aWordGlyph <= myGlyphs.Upper(); ++aWordGlyph)
        myGlyphs[aWordGlyph].Pos.x() -= aMovePen;

      aLine->NbGraphSymbols -= aNbMoved;
      aLine->GlyphUpper = aLine->LastWordStart - 1;
      aLine->Pen.x() = aMovePen;

      myLines.Append(TextLine());
      aLine = &myLines.Last();
      aLine->Update(theFont, theAlignX, theFontScale);

      aLine->GlyphLower     = myGlyphs.Upper() - aNbMoved;
      aLine->GlyphUpper     = myGlyphs.Upper();
      aLine->LastWordStart  = aLine->GlyphLower;
      aLine->NbGraphSymbols = aNbMoved;
      aLine->Pen.x() = aNewlinePen;
    }
    else if (isNewline)
    {
      myLines.Append(TextLine());
      aLine = &myLines.Last();
      aLine->Update(theFont, theAlignX, theFontScale);

      aLine->GlyphLower    = myGlyphs.Upper() + 1;
      aLine->LastWordStart = aLine->GlyphLower;
    }

    ++aLine->NbIndentSymbols;
    if (isGraphSymbol)
      ++aLine->NbGraphSymbols;

    // list of all symbols, including whitespace and newlines, but except command symbols
    myGlyphs.Append(TextGlyph(aCharThis, aLine->Pen, aGlyphWidth));
    aLine->Pen.x() += anAdvanceX;
    aLine->GlyphUpper = myGlyphs.Upper();

    myMaxSymbolWidth = Max (myMaxSymbolWidth, anAdvanceX);
  }
}

// =======================================================================
// function : BoundingBox
// purpose :
// =======================================================================
Font_Rect Font_TextFormatter::BoundingBox() const
{
  Font_Rect aBox = {};
  if (myLines.IsEmpty())
    return aBox;

  aBox.Left = 0.0f;
  switch (myAlignX)
  {
    default:
    case Graphic3d_HTA_LEFT: aBox.Left = 0.0f; break;
    case Graphic3d_HTA_RIGHT: aBox.Left = -myBndSize.x(); break;
    case Graphic3d_HTA_CENTER: aBox.Left = -0.5f * myBndSize.x(); break;
  }
  aBox.Right = aBox.Left + myBndSize.x();

  switch (myAlignY)
  {
    default:
    case Graphic3d_VerticalTextAlignment_TopAscender:
      aBox.Top = 0.0f;
      break;
    case Graphic3d_VerticalTextAlignment_TopBaseline:
      aBox.Top = myLines.First().Ascender;
      break;
    case Graphic3d_VerticalTextAlignment_Center:
      aBox.Top = 0.5f * myBndSize.y();
      break;
    case Graphic3d_VerticalTextAlignment_BottomBaseline:
      aBox.Top = myLines.First().Pen.y() + myBndSize.y() + myLines.Last().Descender;
      break;
    case Graphic3d_VerticalTextAlignment_BottomDescender:
      aBox.Top = myLines.First().Pen.y() + myBndSize.y();
      break;
  }
  aBox.Bottom = aBox.Top - myBndSize.y();
  return aBox;
}

// =======================================================================
// function : Format
// purpose :
// =======================================================================
void Font_TextFormatter::Format()
{
  if (myGlyphs.IsEmpty() || myIsFormatted || myLines.IsEmpty())
    return;

  myIsFormatted = true;

  // calculate width for horizontal alignment
  myBndSize.x() = myWrappingWidth;
  if (HasWrapping())
  {
    // it is not possible to wrap less than symbol width
    myBndSize.x() = Max(myBndSize.x(), myMaxSymbolWidth);

    // trim right spaces
    for (TextLine& aLineIter : myLines)
      aLineIter.TrimRight(myGlyphs, myWrappingWidth);
  }
  else
  {
    for (const TextLine& aLineIter : myLines)
      myBndSize.x() = Max(myBndSize.x(), aLineIter.Pen.x());
  }

  // arrange baselines vertically based on line spacing
  myBndSize.y() = 0.0f;
  {
    NCollection_List<TextLine>::Iterator aLinePrev(myLines);
    NCollection_List<TextLine>::Iterator aLineIter = aLinePrev;
    aLineIter.Next();
    for (; aLineIter.More(); aLinePrev.Next(), aLineIter.Next())
    {
      const float aSpacing = aLinePrev->LineSpacing * 0.5f + aLineIter->LineSpacing * 0.5f;
      myBndSize.y() += aSpacing;
      aLineIter->Pen.y() = aLinePrev->Pen.y() - aSpacing;
    }
    myBndSize.y() += myLines.First().LineSpacing * 0.5f + myLines.Last().LineSpacing * 0.5f;
  }

  // apply horizontal alignment
  for (const TextLine& aLineIter : myLines)
  {
    float aMoveX = 0.0f;
    switch (aLineIter.AlignX)
    {
      default:
      case Graphic3d_HTA_LEFT:
        aMoveX = 0.0f;
        break;
      case Graphic3d_HTA_RIGHT:
        aMoveX = (myBndSize.x() - aLineIter.Pen.x()) - myBndSize.x();
        break;
      case Graphic3d_HTA_CENTER:
        aMoveX = 0.5f * (myBndSize.x() - aLineIter.Pen.x()) - 0.5f * myBndSize.x();
        break;
    }

    if (aMoveX != 0.0f)
    {
      for (int aGlyphIter = aLineIter.GlyphLower; aGlyphIter <= aLineIter.GlyphUpper; ++aGlyphIter)
        myGlyphs[aGlyphIter].Pos.x() += aMoveX;
    }
  }

  // apply vertical alignment (note that OY looks up)
  float aMoveY = 0.0f;
  switch (myAlignY)
  {
    default:
    case Graphic3d_VerticalTextAlignment_TopBaseline:
    {
      aMoveY = 0.0f; // already at first baseline
      break;
    }
    case Graphic3d_VerticalTextAlignment_TopAscender:
    {
      aMoveY = -myLines.First().Ascender;
      break;
    }
    case Graphic3d_VerticalTextAlignment_BottomDescender:
    {
      aMoveY = -myLines.Last().Pen.y() - myLines.Last().Descender;
      break;
    }
    case Graphic3d_VerticalTextAlignment_BottomBaseline:
    {
      aMoveY = -myLines.Last().Pen.y();
      break;
    }
    case Graphic3d_VerticalTextAlignment_Center:
    {
      // old formula - aligns closer to the center of lower-cases characters
      aMoveY = myBndSize.y() * 0.5f - myLines.First().Ascender;
      // alternative formula - aligns closer to the center of upper-cases characters
      //aMoveY = myBndSize.y() * 0.5f - myLines.Last().LineSpacing - myLines.Last().Descender;
      break;
    }
  }

  for (const TextLine& aLineIter : myLines)
  {
    for (int aGlyphIter = aLineIter.GlyphLower; aGlyphIter <= aLineIter.GlyphUpper; ++aGlyphIter)
      myGlyphs[aGlyphIter].Pos.y() += aLineIter.Pen.y() + aMoveY;
  }
}
