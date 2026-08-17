// Created on: 2013-12-20
// Created by: Denis BOGOLEPOV
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

#ifndef BVH_Box_HeaderFile
#define BVH_Box_HeaderFile

#include <BVH_Constants.hxx>
#include <BVH_Types.hxx>
#include <Standard_Macro.hxx>
#include <Standard_Dump.hxx>
#include <Standard_ShortReal.hxx>

#include <limits>

//! Base class for BVH_Box (CRTP idiom is used).
//! @tparam T             Numeric data type
//! @tparam N             Vector dimension
//! @tparam TheDerivedBox Template of derived class that defined axis aligned bounding box.
template <class T, int N, template <class /*T*/, int /*N*/> class TheDerivedBox>
class BVH_BaseBox {};

// forward declaration
template <class T, int N> class BVH_Box;

//! Partial template specialization for BVH_Box when N = 3.
template <class T>
class BVH_BaseBox<T, 3, BVH_Box>
{
public:

  //! Transforms this box with given transformation.
  void Transform (const NCollection_Mat4<T>& theTransform)
  {
    if (theTransform.IsIdentity())
    {
      return;
    }

    BVH_Box<T, 3> *aThis = static_cast<BVH_Box<T, 3>*>(this);
    if (!aThis->IsValid())
    {
      return;
    }

    BVH_Box<T, 3> aBox = Transformed (theTransform);

    aThis->CornerMin() = aBox.CornerMin();
    aThis->CornerMax() = aBox.CornerMax();
  }

  //! Returns a box which is the result of applying the
  //! given transformation to this box.
  BVH_Box<T, 3> Transformed (const NCollection_Mat4<T>& theTransform) const
  {
    const BVH_Box<T, 3> *aThis = static_cast<const BVH_Box<T, 3>*>(this);
    if (theTransform.IsIdentity())
    {
      return *aThis;
    }

    if (!aThis->IsValid())
    {
      return *aThis;
    }

    BVH_Box<T, 3> aResultBox;
    for (size_t aX = 0; aX <= 1; ++aX)
    {
      for (size_t aY = 0; aY <= 1; ++aY)
      {
        for (size_t aZ = 0; aZ <= 1; ++aZ)
        {
          typename BVH::VectorType<T, 4>::Type aPnt =
            theTransform * typename BVH::VectorType<T, 4>::Type (aX ? aThis->CornerMax().x() : aThis->CornerMin().x(),
                                                                 aY ? aThis->CornerMax().y() : aThis->CornerMin().y(),
                                                                 aZ ? aThis->CornerMax().z() : aThis->CornerMin().z(),
                                                                 static_cast<T> (1.0));

          aResultBox.Add (aPnt.xyz());
        }
      }
    }
    return aResultBox;
  }
};

//! Defines axis aligned bounding box (AABB) based on BVH vectors.
//! \tparam T Numeric data type
//! \tparam N Vector dimension
template<class T, int N>
class BVH_Box : public BVH_BaseBox<T, N, BVH_Box>
{
public:

  typedef typename BVH::VectorType<T, N>::Type BVH_VecNt;

public:

  //! Creates uninitialized bounding box.
  BVH_Box() : myIsInited (Standard_False) {}

  //! Creates bounding box of given point.
  BVH_Box (const BVH_VecNt& thePoint)
  : myMinPoint (thePoint),
    myMaxPoint (thePoint),
    myIsInited (Standard_True) {}

  //! Creates bounding box from corner points.
  BVH_Box (const BVH_VecNt& theMinPoint,
           const BVH_VecNt& theMaxPoint)
  : myMinPoint (theMinPoint),
    myMaxPoint (theMaxPoint),
    myIsInited (Standard_True) {}

public:

  //! Clears bounding box.
  void Clear() { myIsInited = Standard_False; }

  //! Is bounding box valid?
  Standard_Boolean IsValid() const { return myIsInited; }

  //! Appends new point to the bounding box.
  void Add (const BVH_VecNt& thePoint)
  {
    if (!myIsInited)
    {
      myMinPoint = thePoint;
      myMaxPoint = thePoint;
      myIsInited = Standard_True;
    }
    else
    {
      myMinPoint = myMinPoint.cwiseMin (thePoint);
      myMaxPoint = myMaxPoint.cwiseMax (thePoint);
    }
  }

  //! Combines bounding box with another one.
  void Combine (const BVH_Box& theBox);

  //! Returns minimum point of bounding box.
  const BVH_VecNt& CornerMin() const { return myMinPoint; }

  //! Returns maximum point of bounding box.
  const BVH_VecNt& CornerMax() const { return myMaxPoint; }

  //! Returns minimum point of bounding box.
  BVH_VecNt& CornerMin() { return myMinPoint; }

  //! Returns maximum point of bounding box.
  BVH_VecNt& CornerMax() { return myMaxPoint; }

  //! Returns surface area of bounding box.
  //! If the box is degenerated into line, returns the perimeter instead.
  T Area() const;

  //! Returns diagonal of bounding box.
  BVH_VecNt Size() const { return myMaxPoint - myMinPoint; }

  //! Returns center of bounding box.
  BVH_VecNt Center() const { return (myMinPoint + myMaxPoint) * static_cast<T> (0.5); }

  //! Returns center of bounding box along the given axis.
  T Center (const Standard_Integer theAxis) const;

  //! Dumps the content of me into the stream
  void DumpJson (Standard_OStream& theOStream, Standard_Integer theDepth = -1) const;

  //! Inits the content of me from the stream
  Standard_Boolean InitFromJson (const Standard_SStream& theSStream, Standard_Integer& theStreamPos);

public:

  //! Checks if the Box is out of the other box.
  Standard_Boolean IsOut (const BVH_Box<T, N>& theOther) const
  {
    if (!theOther.IsValid())
      return Standard_True;

    return IsOut (theOther.myMinPoint, theOther.myMaxPoint);
  }

  //! Checks if the Box is out of the other box defined by two points.
  Standard_Boolean IsOut (const BVH_VecNt& theMinPoint,
                          const BVH_VecNt& theMaxPoint) const
  {
    if (!IsValid())
      return Standard_True;

    int n = Min (N, 3);
    for (int i = 0; i < n; ++i)
    {
      if (myMinPoint[i] > theMaxPoint[i] ||
          myMaxPoint[i] < theMinPoint[i])
        return Standard_True;
    }
    return Standard_False;
  }

  //! Checks if the Box fully contains the other box.
  Standard_Boolean Contains (const BVH_Box<T, N>& theOther,
                             Standard_Boolean& hasOverlap) const
  {
    hasOverlap = Standard_False;
    if (!theOther.IsValid())
      return Standard_False;

    return Contains (theOther.myMinPoint, theOther.myMaxPoint, hasOverlap);
  }

  //! Checks if the Box is fully contains the other box.
  Standard_Boolean Contains (const BVH_VecNt& theMinPoint,
                             const BVH_VecNt& theMaxPoint,
                             Standard_Boolean& hasOverlap) const
  {
    hasOverlap = Standard_False;
    if (!IsValid())
      return Standard_False;

    Standard_Boolean isInside = Standard_True;

    int n = Min (N, 3);
    for (int i = 0; i < n; ++i)
    {
      hasOverlap = (myMinPoint[i] <= theMaxPoint[i] &&
                    myMaxPoint[i] >= theMinPoint[i]);
      if (!hasOverlap)
        return Standard_False;

      isInside = isInside && (myMinPoint[i] <= theMinPoint[i] &&
                              myMaxPoint[i] >= theMaxPoint[i]);
    }
    return isInside;
  }

  //! Checks if the Point is out of the box.
  Standard_Boolean IsOut (const BVH_VecNt& thePoint) const
  {
    if (!IsValid())
      return Standard_True;

    int n = Min (N, 3);
    for (int i = 0; i < n; ++i)
    {
      if (thePoint[i] < myMinPoint[i] ||
          thePoint[i] > myMaxPoint[i])
        return Standard_True;
    }
    return Standard_False;
  }

protected:

  BVH_VecNt        myMinPoint; //!< Minimum point of bounding box
  BVH_VecNt        myMaxPoint; //!< Maximum point of bounding box
  Standard_Boolean myIsInited; //!< Is bounding box initialized?

};

namespace BVH
{
  //! Tool class for calculating box center along the given axis.
  //! \tparam T Numeric data type
  //! \tparam N Vector dimension
  template<class T, int N>
  struct CenterAxis
  {
    // Not implemented
  };

  template<class T>
  struct CenterAxis<T, 2>
  {
    static T Center (const BVH_Box<T, 2>& theBox, const Standard_Integer theAxis)
    {
      if (theAxis == 0)
      {
        return (theBox.CornerMin().x() + theBox.CornerMax().x()) * static_cast<T> (0.5);
      }
      else if (theAxis == 1)
      {
        return (theBox.CornerMin().y() + theBox.CornerMax().y()) * static_cast<T> (0.5);
      }
      return static_cast<T> (0.0);
    }
  };

  template<class T>
  struct CenterAxis<T, 3>
  {
    static T Center (const BVH_Box<T, 3>& theBox, const Standard_Integer theAxis)
    {
      if (theAxis == 0)
      {
        return (theBox.CornerMin().x() + theBox.CornerMax().x()) * static_cast<T> (0.5);
      }
      else if (theAxis == 1)
      {
        return (theBox.CornerMin().y() + theBox.CornerMax().y()) * static_cast<T> (0.5);
      }
      else if (theAxis == 2)
      {
        return (theBox.CornerMin().z() + theBox.CornerMax().z()) * static_cast<T> (0.5);
      }
      return static_cast<T> (0.0);
    }
  };

  template<class T>
  struct CenterAxis<T, 4>
  {
    static T Center (const BVH_Box<T, 4>& theBox, const Standard_Integer theAxis)
    {
      if (theAxis == 0)
      {
        return (theBox.CornerMin().x() + theBox.CornerMax().x()) * static_cast<T> (0.5);
      }
      else if (theAxis == 1)
      {
        return (theBox.CornerMin().y() + theBox.CornerMax().y()) * static_cast<T> (0.5);
      }
      else if (theAxis == 2)
      {
        return (theBox.CornerMin().z() + theBox.CornerMax().z()) * static_cast<T> (0.5);
      }
      return static_cast<T> (0.0);
    }
  };

  //! Tool class for calculating surface area of the box.
  //! \tparam T Numeric data type
  //! \tparam N Vector dimension
  template<class T, int N>
  struct SurfaceCalculator
  {
    // Not implemented
  };

  template<class T>
  struct SurfaceCalculator<T, 2>
  {
    static T Area (const typename BVH_Box<T, 2>::BVH_VecNt& theSize)
    {
      const T anArea = theSize.x() * theSize.y();

      if (anArea < std::numeric_limits<T>::epsilon())
      {
        return theSize.x() + theSize.y();
      }

      return anArea;
    }
  };

  template<class T>
  struct SurfaceCalculator<T, 3>
  {
    static T Area (const typename BVH_Box<T, 3>::BVH_VecNt& theSize)
    {
      const T anArea = ( theSize.x() * theSize.y() +
                         theSize.x() * theSize.z() +
                         theSize.z() * theSize.y() ) * static_cast<T> (2.0);

      if (anArea < std::numeric_limits<T>::epsilon())
      {
        return theSize.x() +
               theSize.y() +
               theSize.z();
      }

      return anArea;
    }
  };

  template<class T>
  struct SurfaceCalculator<T, 4>
  {
    static T Area (const typename BVH_Box<T, 4>::BVH_VecNt& theSize)
    {
      const T anArea = ( theSize.x() * theSize.y() +
                         theSize.x() * theSize.z() +
                         theSize.z() * theSize.y() ) * static_cast<T> (2.0);

      if (anArea < std::numeric_limits<T>::epsilon())
      {
        return theSize.x() +
               theSize.y() +
               theSize.z();
      }

      return anArea;
    }
  };

  //! Tool class for calculate component-wise vector minimum
  //! and maximum (optimized version).
  //! \tparam T Numeric data type
  //! \tparam N Vector dimension
  template<class T, int N>
  struct BoxMinMax
  {
    typedef typename BVH::VectorType<T, N>::Type BVH_VecNt;

    static void CwiseMin (BVH_VecNt& theVec1, const BVH_VecNt& theVec2)
    {
      theVec1.x() = Min (theVec1.x(), theVec2.x());
      theVec1.y() = Min (theVec1.y(), theVec2.y());
      theVec1.z() = Min (theVec1.z(), theVec2.z());
    }

    static void CwiseMax (BVH_VecNt& theVec1, const BVH_VecNt& theVec2)
    {
      theVec1.x() = Max (theVec1.x(), theVec2.x());
      theVec1.y() = Max (theVec1.y(), theVec2.y());
      theVec1.z() = Max (theVec1.z(), theVec2.z());
    }
  };

  template<class T>
  struct BoxMinMax<T, 2>
  {
    typedef typename BVH::VectorType<T, 2>::Type BVH_VecNt;

    static void CwiseMin (BVH_VecNt& theVec1, const BVH_VecNt& theVec2)
    {
      theVec1.x() = Min (theVec1.x(), theVec2.x());
      theVec1.y() = Min (theVec1.y(), theVec2.y());
    }

    static void CwiseMax (BVH_VecNt& theVec1, const BVH_VecNt& theVec2)
    {
      theVec1.x() = Max (theVec1.x(), theVec2.x());
      theVec1.y() = Max (theVec1.y(), theVec2.y());
    }
  };
}

// =======================================================================
// function : Combine
// purpose  :
// =======================================================================
template<class T, int N>
void BVH_Box<T, N>::Combine (const BVH_Box& theBox)
{
  if (theBox.myIsInited)
  {
    if (!myIsInited)
    {
      myMinPoint = theBox.myMinPoint;
      myMaxPoint = theBox.myMaxPoint;
      myIsInited = Standard_True;
    }
    else
    {
      BVH::BoxMinMax<T, N>::CwiseMin (myMinPoint, theBox.myMinPoint);
      BVH::BoxMinMax<T, N>::CwiseMax (myMaxPoint, theBox.myMaxPoint);
    }
  }
}

// =======================================================================
// function : Area
// purpose  :
// =======================================================================
template<class T, int N>
T BVH_Box<T, N>::Area() const
{
  return !myIsInited ? static_cast<T> (0.0) : BVH::SurfaceCalculator<T, N>::Area (myMaxPoint - myMinPoint);
}

// =======================================================================
// function : Center
// purpose  :
// =======================================================================
template<class T, int N>
T BVH_Box<T, N>::Center (const Standard_Integer theAxis) const
{
  return BVH::CenterAxis<T, N>::Center (*this, theAxis);
}

// =======================================================================
// function : DumpJson
// =======================================================================
template<class T, int N>
void BVH_Box<T, N>::DumpJson(Standard_OStream& theOStream, Standard_Integer ) const
{
  // overloads for template specialization
  struct MinMaxDumper
  {
    static void dump(Standard_OStream& theOStream,
                     const typename BVH::VectorType<T, 1>::Type& theMin,
                     const typename BVH::VectorType<T, 1>::Type& theMax)
    {
      OCCT_DUMP_FIELD_VALUES_NUMERICAL(theOStream, "MinPoint", 1, theMin)
      OCCT_DUMP_FIELD_VALUES_NUMERICAL(theOStream, "MaxPoint", 1, theMax)
    }

    static void dump(Standard_OStream& theOStream,
                     const typename BVH::VectorType<T, 2>::Type& theMin,
                     const typename BVH::VectorType<T, 2>::Type& theMax)
    {
      OCCT_DUMP_FIELD_VALUES_NUMERICAL(theOStream, "MinPoint", 2, theMin[0], theMin[1])
      OCCT_DUMP_FIELD_VALUES_NUMERICAL(theOStream, "MaxPoint", 2, theMax[0], theMax[1])
    }

    static void dump(Standard_OStream& theOStream,
                     const typename BVH::VectorType<T, 3>::Type& theMin,
                     const typename BVH::VectorType<T, 3>::Type& theMax)
    {
      OCCT_DUMP_FIELD_VALUES_NUMERICAL(theOStream, "MinPoint", 3, theMin[0], theMin[1], theMin[2])
      OCCT_DUMP_FIELD_VALUES_NUMERICAL(theOStream, "MaxPoint", 3, theMax[0], theMax[1], theMin[2])
    }

    static void dump(Standard_OStream& theOStream,
                     const typename BVH::VectorType<T, 4>::Type& theMin,
                     const typename BVH::VectorType<T, 4>::Type& theMax)
    {
      OCCT_DUMP_FIELD_VALUES_NUMERICAL(theOStream, "MinPoint", 4, theMin[0], theMin[1], theMin[2], theMin[3])
      OCCT_DUMP_FIELD_VALUES_NUMERICAL(theOStream, "MaxPoint", 4, theMax[0], theMax[1], theMin[2], theMin[3])
    }
  };

  OCCT_DUMP_FIELD_VALUE_NUMERICAL(theOStream, myIsInited)
  MinMaxDumper::dump(theOStream, myMinPoint, myMaxPoint);
}

// =======================================================================
// function : InitFromJson
// =======================================================================
template<class T, int N>
Standard_Boolean BVH_Box<T, N>::InitFromJson(const Standard_SStream& theSStream, Standard_Integer& theStreamPos)
{
  // overloads for template specialization
  struct MinMaxDumper
  {
    static bool init(const TCollection_AsciiString& theStr,
                     Standard_Integer& thePos,
                     typename BVH::VectorType<T, 1>::Type& theMin,
                     typename BVH::VectorType<T, 1>::Type& theMax)
    {
      typename BVH::VectorType<T, 1>::Type aT = {};
      OCCT_INIT_VECTOR_CLASS(theStr, "MinPoint", thePos, 1, aT);
      theMin = aT;
      OCCT_INIT_VECTOR_CLASS(theStr, "MaxPoint", thePos, 1, aT);
      theMax = aT;
      return true;
    }

    static bool init(const TCollection_AsciiString& theStr,
                     Standard_Integer& thePos,
                     typename BVH::VectorType<T, 2>::Type& theMin,
                     typename BVH::VectorType<T, 2>::Type& theMax)
    {
      typename BVH::VectorType<T, 2>::Type aT;
      OCCT_INIT_VECTOR_CLASS(theStr, "MinPoint", thePos, 2, aT[0], aT[1]);
      theMin = aT;
      OCCT_INIT_VECTOR_CLASS(theStr, "MaxPoint", thePos, 2, aT[0], aT[1]);
      theMax = aT;
      return true;
    }

    static bool init(const TCollection_AsciiString& theStr,
                     Standard_Integer& thePos,
                     typename BVH::VectorType<T, 3>::Type& theMin,
                     typename BVH::VectorType<T, 3>::Type& theMax)
    {
      typename BVH::VectorType<T, 3>::Type aT;
      OCCT_INIT_VECTOR_CLASS(theStr, "MinPoint", thePos, 3, aT[0], aT[1], aT[2]);
      theMin = aT;
      OCCT_INIT_VECTOR_CLASS(theStr, "MaxPoint", thePos, 3, aT[0], aT[1], aT[2]);
      theMax = aT;
      return true;
    }

    static bool init(const TCollection_AsciiString& theStr,
                     Standard_Integer& thePos,
                     typename BVH::VectorType<T, 4>::Type& theMin,
                     typename BVH::VectorType<T, 4>::Type& theMax)
    {
      typename BVH::VectorType<T, 4>::Type aT;
      OCCT_INIT_VECTOR_CLASS(theStr, "MinPoint", thePos, 4, aT[0], aT[1], aT[2], aT[3]);
      theMin = aT;
      OCCT_INIT_VECTOR_CLASS(theStr, "MaxPoint", thePos, 4, aT[0], aT[1], aT[2], aT[3]);
      theMax = aT;
      return true;
    }
  };

  Standard_Integer aPos = theStreamPos;

  Standard_Integer anIsInited = 0;
  TCollection_AsciiString aStreamStr = Standard_Dump::Text(theSStream);

  OCCT_INIT_FIELD_VALUE_INTEGER(aStreamStr, aPos, anIsInited);
  myIsInited = anIsInited != 0;

  if (!MinMaxDumper::init(aStreamStr, aPos, myMinPoint, myMaxPoint))
    return false;

  theStreamPos = aPos;
  return true;
}

#endif // _BVH_Box_Header
