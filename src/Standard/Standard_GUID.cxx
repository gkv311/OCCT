// Copyright (c) 1998-1999 Matra Datavision
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

#include <Standard_GUID.hxx>

#include <Standard_IStream.hxx>

//=======================================================================
//function : ToCString
//purpose  : 
//=======================================================================
void Standard_GUID::ToCString(char* aStrGuid, const size_t theSize) const
{
  if (theSize < Standard_GUID_SIZE_ALLOC)
    throw Standard_RangeError ("Standard_GUID::ToCString() buffer of insufficient size");

  Snprintf(aStrGuid, theSize,"%.8x-%.4x-%.4x-%.4x-%.2x%.2x%.2x%.2x%.2x%.2x",
	  my32b,
	  (unsigned short) my16b1,
	  (unsigned short) my16b2,
	  (unsigned short) my16b3,
	  (unsigned char)my8b1,
	  (unsigned char)my8b2,
	  (unsigned char)my8b3,
	  (unsigned char)my8b4,
	  (unsigned char)my8b5,
	  (unsigned char)my8b6);
}

//=======================================================================
//function : ToExtString
//purpose  : 
//=======================================================================
void Standard_GUID::ToExtString(Standard_ExtCharacter* theBuf, const size_t theSize) const
{
  if (theSize < Standard_GUID_SIZE_ALLOC)
    throw Standard_RangeError("Standard_GUID::ToExtString() buffer of insufficient size");

  char sguid[Standard_GUID_SIZE_ALLOC];
  ToCString(sguid, Standard_GUID_SIZE_ALLOC);

  for (Standard_Integer i = 0; i < Standard_GUID_SIZE; i++)
  {
    theBuf[i] = (Standard_ExtCharacter)sguid[i];
  }

  theBuf[Standard_GUID_SIZE] = (Standard_ExtCharacter)0;
}

void Standard_GUID::ShallowDump(Standard_OStream& aStream) const
{
  char sguid[Standard_GUID_SIZE_ALLOC];
  ToCString(sguid, Standard_GUID_SIZE_ALLOC);
  aStream << sguid;
}

//============================================================================
// function : HashCode
// purpose  :
//============================================================================
Standard_Integer Standard_GUID::Hash(const Standard_Integer Upper) const
{
  if (Upper < 1){
    throw Standard_RangeError("Standard_GUID::Hash: Try to apply HashCode method with negative or null argument.");
  }

  char sguid[Standard_GUID_SIZE_ALLOC];
  ToCString(sguid, Standard_GUID_SIZE_ALLOC);

  return ::HashCode(sguid,Upper);
}
