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

#include <gp.hxx>

#include <gp_Ax2.hxx>
#include <gp_Ax2d.hxx>
#include <gp_Dir.hxx>
#include <gp_Dir2d.hxx>
#include <gp_Pnt.hxx>
#include <gp_Pnt2d.hxx>

//=======================================================================
//function : Origin
//purpose  : 
//=======================================================================
const gp_Pnt&  gp::Origin()
{
  static constexpr gp_Pnt gp_Origin = gp_Pnt::Origin();
  return gp_Origin;
}

//=======================================================================
//function : DX
//purpose  : 
//=======================================================================

const gp_Dir&  gp::DX()
{
  static constexpr gp_Dir gp_DX = gp_Dir::DX();
  return gp_DX;
}

//=======================================================================
//function : DY
//purpose  : 
//=======================================================================

const gp_Dir&  gp::DY()
{
  static constexpr gp_Dir gp_DY = gp_Dir::DY();
  return gp_DY;
}

//=======================================================================
//function : DZ
//purpose  : 
//=======================================================================

const gp_Dir&  gp::DZ()
{
  static constexpr gp_Dir gp_DZ = gp_Dir::DZ();
  return gp_DZ;
}

//=======================================================================
//function : OX
//purpose  : 
//=======================================================================

const gp_Ax1&  gp::OX()
{
  static constexpr gp_Ax1 gp_OX = gp_Ax1::OX();
  return gp_OX;
}

//=======================================================================
//function : OY
//purpose  : 
//=======================================================================

const gp_Ax1&  gp::OY()
{
  static constexpr gp_Ax1 gp_OY = gp_Ax1::OY();
  return gp_OY;
}

//=======================================================================
//function : OZ
//purpose  : 
//=======================================================================

const gp_Ax1&  gp::OZ()
{
  static constexpr gp_Ax1 gp_OZ = gp_Ax1::OZ();
  return gp_OZ;
}

//=======================================================================
//function : XOY
//purpose  : 
//=======================================================================

const gp_Ax2&  gp::XOY()
{
  static constexpr gp_Ax2 gp_XOY = gp_Ax2::XOY();
  return gp_XOY;
}

//=======================================================================
//function : ZOX
//purpose  : 
//=======================================================================

const gp_Ax2&  gp::ZOX()
{
  static constexpr gp_Ax2 gp_ZOX = gp_Ax2::ZOX();
  return gp_ZOX;
}

//=======================================================================
//function : YOZ
//purpose  : 
//=======================================================================

const gp_Ax2&  gp::YOZ()
{
  static constexpr gp_Ax2 gp_YOZ = gp_Ax2::YOZ();
  return gp_YOZ;
}

//=======================================================================
//function : Origin2d
//purpose  : 
//=======================================================================

const gp_Pnt2d&  gp::Origin2d()
{
  static constexpr gp_Pnt2d gp_Origin2d = gp_Pnt2d::Origin();
  return gp_Origin2d;
}

//=======================================================================
//function : DX2d
//purpose  : 
//=======================================================================

const gp_Dir2d&  gp::DX2d()
{
  static constexpr gp_Dir2d gp_DX2d = gp_Dir2d::DX();
  return gp_DX2d;
}

//=======================================================================
//function : DY2d
//purpose  : 
//=======================================================================

const gp_Dir2d&  gp::DY2d()
{
  static constexpr gp_Dir2d gp_DY2d = gp_Dir2d::DY();
  return gp_DY2d;
}

//=======================================================================
//function : OX2d
//purpose  : 
//=======================================================================

const gp_Ax2d&  gp::OX2d()
{
  static constexpr gp_Ax2d gp_OX2d = gp_Ax2d::OX();
  return gp_OX2d;
}

//=======================================================================
//function : OY2d
//purpose  : 
//=======================================================================

const gp_Ax2d&  gp::OY2d()
{
  static constexpr gp_Ax2d gp_OY2d = gp_Ax2d::OY();
  return gp_OY2d;
}

