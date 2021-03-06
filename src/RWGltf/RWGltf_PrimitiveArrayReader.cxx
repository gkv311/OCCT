// Author: Kirill Gavrilov
// Copyright (c) 2019 OPEN CASCADE SAS
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

#include <RWGltf_PrimitiveArrayReader.hxx>

#include <RWGltf_GltfLatePrimitiveArray.hxx>

#include <BRep_Builder.hxx>
#include <Message.hxx>
#include <Message_Messenger.hxx>
#include <OSD_CachedFileSystem.hxx>
#include <Standard_ArrayStreamBuffer.hxx>
#include <TopoDS.hxx>
#include <TopoDS_Iterator.hxx>

IMPLEMENT_STANDARD_RTTIEXT(RWGltf_PrimitiveArrayReader, Standard_Transient)

// =======================================================================
// function : reportError
// purpose  :
// =======================================================================
void RWGltf_PrimitiveArrayReader::reportError (const TCollection_AsciiString& theText)
{
  Message::SendFail (myErrorPrefix + theText);
}

// =======================================================================
// function : load
// purpose  :
// =======================================================================
bool RWGltf_PrimitiveArrayReader::load (const Handle(RWGltf_GltfLatePrimitiveArray)& theMesh,
                                        const Handle(OSD_FileSystem)& theFileSystem)
{
  reset();
  if (theMesh.IsNull()
   || theMesh->PrimitiveMode() == RWGltf_GltfPrimitiveMode_UNKNOWN)
  {
    return false;
  }

  for (NCollection_Sequence<RWGltf_GltfPrimArrayData>::Iterator aDataIter (theMesh->Data()); aDataIter.More(); aDataIter.Next())
  {
    const RWGltf_GltfPrimArrayData& aData = aDataIter.Value();
    if (!aData.StreamData.IsNull())
    {
      Standard_ArrayStreamBuffer aStreamBuffer ((const char* )aData.StreamData->Data(), aData.StreamData->Size());
      std::istream aStream (&aStreamBuffer);
      aStream.seekg ((std::streamoff )aData.StreamOffset, std::ios_base::beg);
      if (!readBuffer (aStream, theMesh->Id(), aData.Accessor, aData.Type, theMesh->PrimitiveMode()))
      {
        return false;
      }
      continue;
    }
    else if (aData.StreamUri.IsEmpty())
    {
      reportError (TCollection_AsciiString ("Buffer '") + theMesh->Id() + "' does not define uri.");
      return false;
    }

    opencascade::std::shared_ptr<std::istream> aSharedStream = theFileSystem->OpenIStream (aData.StreamUri, std::ios::in | std::ios::binary, aData.StreamOffset);
    if (aSharedStream.get() == NULL)
    {
      reportError (TCollection_AsciiString ("Buffer '") + theMesh->Id() + "refers to invalid file '" + aData.StreamUri + "'.");
      return false;
    }
    if (!readBuffer (*aSharedStream.get(), theMesh->Id(), aData.Accessor, aData.Type, theMesh->PrimitiveMode()))
    {
      return false;
    }
  }
  return true;
}
