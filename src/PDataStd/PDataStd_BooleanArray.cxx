// Created on: 2007-05-29
// Created by: Vlad Romashko
// Copyright (c) 2007-2012 OPEN CASCADE SAS
//
// The content of this file is subject to the Open CASCADE Technology Public
// License Version 6.5 (the "License"). You may not use the content of this file
// except in compliance with the License. Please obtain a copy of the License
// at http://www.opencascade.org and read it completely before using this file.
//
// The Initial Developer of the Original Code is Open CASCADE S.A.S., having its
// main offices at: 1, place des Freres Montgolfier, 78280 Guyancourt, France.
//
// The Original Code and all software distributed under the License is
// distributed on an "AS IS" basis, without warranty of any kind, and the
// Initial Developer hereby disclaims all such warranties, including without
// limitation, any warranties of merchantability, fitness for a particular
// purpose or non-infringement. Please see the License for the specific terms
// and conditions governing the rights and limitations under the License.


#include <PDataStd_BooleanArray.ixx>

//=======================================================================
//function : PDataStd_BooleanArray
//purpose  : 
//=======================================================================
PDataStd_BooleanArray::PDataStd_BooleanArray() :
    myLower(0),
    myUpper(0)
{ 

}

//=======================================================================
//function : SetLower
//purpose  : 
//=======================================================================
void PDataStd_BooleanArray::SetLower(const Standard_Integer lower)
{
  myLower = lower;
}

//=======================================================================
//function : SetUpper
//purpose  : 
//=======================================================================
void PDataStd_BooleanArray::SetUpper(const Standard_Integer upper)
{
  myUpper = upper;
}

//=======================================================================
//function : Lower
//purpose  : 
//=======================================================================
Standard_Integer PDataStd_BooleanArray::Lower (void) const 
{ 
  return myLower;
}

//=======================================================================
//function : Upper
//purpose  : 
//=======================================================================
Standard_Integer PDataStd_BooleanArray::Upper (void) const 
{ 
  return myUpper;
}

//=======================================================================
//function : Set
//purpose  : 
//=======================================================================
void PDataStd_BooleanArray::Set(const Handle(PColStd_HArray1OfInteger)& values)
{
  myValues = values;
}

//=======================================================================
//function : Get
//purpose  : 
//=======================================================================
const Handle(PColStd_HArray1OfInteger)& PDataStd_BooleanArray::Get() const
{
  return myValues;
}
