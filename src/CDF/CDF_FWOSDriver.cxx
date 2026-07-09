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


#include <CDF_FWOSDriver.hxx>
#include <CDM_MetaData.hxx>
#include <OSD_Directory.hxx>
#include <OSD_Environment.hxx>
#include <OSD_File.hxx>
#include <OSD_FileNode.hxx>
#include <OSD_Path.hxx>
#include <OSD_Protection.hxx>
#include <OSD_SingleProtection.hxx>
#include <Standard_Type.hxx>
#include <TCollection_ExtendedString.hxx>

IMPLEMENT_STANDARD_RTTIEXT(CDF_FWOSDriver,CDF_MetaDataDriver)

#ifdef _MSC_VER
#include <tchar.h>
#endif  // _MSC_VER

//==============================================================================
//function : PutSlash
//purpose  :
//==============================================================================
static const char PutSlash[] =
#ifdef _WIN32
  "\\";
#else
  "/";
#endif  // _WIN32

//==============================================================================
//function : CDF_FWOSDriver
//purpose  :
//==============================================================================
CDF_FWOSDriver::CDF_FWOSDriver(CDM_MetaDataLookUpTable& theLookUpTable)
: myLookUpTable (&theLookUpTable)
{
}

//==============================================================================
//function : Find
//purpose  :
//==============================================================================
Standard_Boolean CDF_FWOSDriver::Find(const TCollection_ExtendedString& theFolder,
                                      const TCollection_ExtendedString& theName,
                                      const TCollection_ExtendedString& /*theVersion*/)
{
  const OSD_Path aPath1 = OSD_Path(TCollection_AsciiString(theFolder));
  OSD_Directory aDirectory(aPath1);
  if (!aDirectory.Exists())
    return false;

  const OSD_Path aPath2 = OSD_Path(TCollection_AsciiString(Concatenate(theFolder, theName)));
  OSD_File aFile(aPath2);
  return aFile.Exists();
}

//==============================================================================
//function : HasReadPermission
//purpose  :
//==============================================================================
Standard_Boolean CDF_FWOSDriver::HasReadPermission(const TCollection_ExtendedString& theFolder,
                                                   const TCollection_ExtendedString& theName,
                                                   const TCollection_ExtendedString& /*theVersion*/)
{
  const OSD_Path aPath = OSD_Path(TCollection_AsciiString(Concatenate(theFolder, theName)));
  const OSD_SingleProtection aProtection = OSD_File(aPath).Protection().User();
  switch (aProtection)
  {
    case OSD_None:
    case OSD_R:
    case OSD_RW:
    case OSD_RX:
    case OSD_WX:
    case OSD_RWX:
    case OSD_RD:
    case OSD_RWD:
    case OSD_RXD:
    case OSD_RWXD:
      return Standard_True;
    default:
      return Standard_False;
  }
}

//==============================================================================
//function : MetaData
//purpose  :
//==============================================================================
Handle(CDM_MetaData) CDF_FWOSDriver::MetaData(const TCollection_ExtendedString& theFolder,
                                              const TCollection_ExtendedString& theName,
                                              const TCollection_ExtendedString& /*theVersion*/)
{
  const TCollection_ExtendedString p = Concatenate(theFolder, theName);
  return CDM_MetaData::LookUp(*myLookUpTable, theFolder, theName, p, p, OSD_File::IsReadOnly(p));
}

//==============================================================================
//function : CreateMetaData
//purpose  :
//==============================================================================
Handle(CDM_MetaData) CDF_FWOSDriver::CreateMetaData(const Handle(CDM_Document)& theDocument,
                                                    const TCollection_ExtendedString& theFileName)
{
  return CDM_MetaData::LookUp(*myLookUpTable, theDocument->RequestedFolder(), theDocument->RequestedName(),
                              BuildFileName(theDocument),
                              theFileName, OSD_File::IsReadOnly(theFileName));
}

//==============================================================================
//function : BuildFileName
//purpose  :
//==============================================================================
TCollection_ExtendedString CDF_FWOSDriver::BuildFileName(const Handle(CDM_Document)& theDocument)
{
  return Concatenate(theDocument->RequestedFolder(), theDocument->RequestedName());
}

//==============================================================================
//function : FindFolder
//purpose  :
//==============================================================================
Standard_Boolean CDF_FWOSDriver::FindFolder(const TCollection_ExtendedString& theFolder)
{
  const OSD_Path aPath = OSD_Path(TCollection_AsciiString(theFolder));
  OSD_Directory aDirectory(aPath);
  return aDirectory.Exists();
}

//==============================================================================
//function : Concatenate
//purpose  :
//==============================================================================
TCollection_ExtendedString CDF_FWOSDriver::Concatenate(const TCollection_ExtendedString& theFolder,
                                                       const TCollection_ExtendedString& theName)
{
  return theFolder + PutSlash + theName;
}

//==============================================================================
//function : DefaultFolder
//purpose  :
//==============================================================================
TCollection_ExtendedString CDF_FWOSDriver::DefaultFolder()
{
  // return home folder when defined and temporary folder otherwise
#ifdef _WIN32
  const TCollection_AsciiString aHome = OSD_Environment("HOMEDRIVE").Value();
  if (!aHome.IsEmpty())
    return aHome + OSD_Environment("HOMEPATH").Value();

  const TCollection_AsciiString aTemp = OSD_Environment("TEMP").Value();
  return !aTemp.IsEmpty() ? aTemp : ".";
#else
  const TCollection_AsciiString aHome = OSD_Environment("HOME").Value();
  return !aHome.IsEmpty() ? aHome : "/tmp";
#endif
}

//==============================================================================
//function : SetName
//purpose  :
//==============================================================================
TCollection_ExtendedString CDF_FWOSDriver::SetName(const Handle(CDM_Document)& aDocument,
                                                      const TCollection_ExtendedString& aName)
{
  
  TCollection_ExtendedString xn(aName), n(aName);

#ifdef _WIN32
  //windows is not case sensitive
  //make the extension lower case
  for(int i = 1; i <= xn.Length(); i++)
  {
	Standard_ExtCharacter echar = xn.Value(i);
	echar = towlower(echar);
	xn.SetValue(i, echar);
  }
#endif
  
  TCollection_ExtendedString e (aDocument->FileExtension());
  TCollection_ExtendedString xe(e);
  if (e.Length() > 0) {
#ifdef _WIN32
    //windows is not case sensitive
    //make the extension lower case
    for(int i = 1; i <= xe.Length(); i++)
    {
	  Standard_ExtCharacter echar = xe.Value(i);
	  echar = towlower(echar);
	  xe.SetValue(i, echar);
    }
#endif
    xe.Insert(1, '.');
    e.Insert(1, '.');
    Standard_Integer ln = xn.Length();
    Standard_Integer le = xe.Length();
    Standard_Boolean ExtensionIsAlreadyThere = Standard_False;
    if(ln>=le) {
      Standard_Integer ind=xn.SearchFromEnd(xe);
      ExtensionIsAlreadyThere = ind+le-1==ln;
    }
    if(!ExtensionIsAlreadyThere) n+=e;
  }
  return n;
}
