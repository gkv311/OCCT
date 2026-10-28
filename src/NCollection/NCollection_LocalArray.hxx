// Created on: 2009-09-23
// Copyright (c) 2009-2014 OPEN CASCADE SAS
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

#ifndef _NCollection_LocalArray_HeaderFile
#define _NCollection_LocalArray_HeaderFile

#include <Standard.hxx>
#include <Standard_TypeDef.hxx>

#include <algorithm>
#include <cstring>
#include <type_traits>

//! Auxiliary class optimizing creation of array buffer 
//! (using stack allocation for small arrays).
template<class theItem, Standard_Integer MAX_ARRAY_SIZE = 1024> class NCollection_LocalArray
{
  static_assert(std::is_trivially_copyable<theItem>::value, "Type should be trivially copyable");
public:

  explicit NCollection_LocalArray (const size_t theSize)
  : myPtr (myBuffer)
  {
    Allocate(theSize);
  }

  NCollection_LocalArray ()
  : myPtr (myBuffer), mySize(0) {}

  ~NCollection_LocalArray()
  {
    Deallocate();
  }

  void Allocate (const size_t theSize)
  {
    Deallocate();
    if (theSize > MAX_ARRAY_SIZE)
      myPtr = (theItem*)Standard::Allocate (theSize * sizeof(theItem));
    else
      myPtr = myBuffer;

    mySize = theSize;
  }

  //! Resize the array to @p theSize while preserving existing content.
  void Resize (const size_t theSize);

  size_t Size() const
  {
    return mySize;
  }

  operator theItem*() const
  {
    return myPtr;
  }

private:

  NCollection_LocalArray (const NCollection_LocalArray& );
  NCollection_LocalArray& operator= (const NCollection_LocalArray& );

protected:

  void Deallocate()
  {
    if (myPtr != myBuffer)
      Standard::Free (myPtr);
  }

protected:

  theItem  myBuffer[MAX_ARRAY_SIZE];
  theItem* myPtr;
  size_t mySize;

};

// =======================================================================
// function : Resize
// =======================================================================
template<class theItem, Standard_Integer MAX_ARRAY_SIZE>
inline void NCollection_LocalArray<theItem, MAX_ARRAY_SIZE>::Resize(const size_t theSize)
{
  const bool wasLocal   = myPtr == myBuffer;
  const bool toUseLocal = theSize <= size_t(MAX_ARRAY_SIZE);
  if (toUseLocal == wasLocal)
  {
    if (toUseLocal || theSize <= mySize)
    {
      mySize = theSize; // shrink logical size without reallocation
      return;
    }
    else
    {
      myPtr = static_cast<theItem*>(Standard::Reallocate(myPtr, theSize * sizeof(theItem)));
      mySize = theSize;
      return;
    }
  }

  // copy data from local buffer to dynamic buffer
  const size_t aNbCopy = std::min(mySize, theSize);
  if (aNbCopy == 0)
  {
    Allocate(theSize);
    return;
  }

  const theItem* anOldPtr = myPtr;
  myPtr = nullptr;
  Allocate(theSize);

  std::memcpy(myPtr, anOldPtr, aNbCopy * sizeof(theItem));

  if (!wasLocal)
    Standard::Free(anOldPtr);
}

#endif // _NCollection_LocalArray_HeaderFile
