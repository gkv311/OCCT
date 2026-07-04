# This Python script defines Qt Creator debugging helpers for OCCT types and collections.
#
# Make this location known to Qt Creator using the following setting:
# - Debugger > Locals & Expressions > Extra Debugging Helpers.

from dumper import Children, SubItem, UnnamedSubItem, DumperBase
from utils import DisplayFormat, TypeCode

# Print handle (a smart-pointer).
def qdump__opencascade__handle(d, value):
    entity = value['entity']
    if entity.pointer() == 0:
        d.putValue('(null)')
    else:
        d.putItem(entity.dereference())
        d.putBetterType(value.type)

# Print UTF-8 string.
def qdump__TCollection_AsciiString(d, value):
    mystring = value['mystring'].pointer()
    mylength = value['mylength'].integer()
    d.putCharArrayHelper(mystring, mylength, d.createType('char'), 'utf8')

# Print UTF-16 string.
def qdump__TCollection_ExtendedString(d, value):
    mystring = value['mystring'].pointer()
    mylength = value['mylength'].integer()
    d.putCharArrayHelper(mystring, mylength, d.createType('char16_t'), 'utf16')

# Print UTF-8 string.
def qdump__TCollection_HAsciiString(d, value):
    mystring = value['myString']['mystring'].pointer()
    mylength = value['myString']['mylength'].integer()
    d.putCharArrayHelper(mystring, mylength, d.createType('char'), 'utf8')

# Print UTF-16 string.
def qdump__TCollection_HExtendedString(d, value):
    mystring = value['myString']['mystring'].pointer()
    mylength = value['myString']['mylength'].integer()
    d.putCharArrayHelper(mystring, mylength, d.createType('char16_t'), 'utf16')

# Print 2D vector.
def qdump__gp_XY(d, value):
    d.putValue('(%s, %s)' % (value.split('dd')))
    d.putPlainChildren(value)

 # Print 2D vector.
def qdump__gp_Vec2d(d, value):
    d.putValue('(%s, %s)' % (value.split('dd')))
    d.putPlainChildren(value)

# Print 2D vector.
def qdump__gp_Dir2d(d, value):
    d.putValue('(%s, %s)' % (value.split('dd')))
    d.putPlainChildren(value)

# Print 2x2 matrix as a list of rows (as returned by gp_Mat2d::Row())
# Note that indexation starts from 1 in this collection, which is preserved in the output.
def qdump__gp_Mat2d(d, value):
    d.putValue('mat2(%s, %s)(%s, %s)' % (value.split('dddd')))
    d.putExpandable()
    if d.isExpanded():
        nbChildren = 4
        with Children(d, nbChildren):
            d.putMembersItem(value, True)
            (row1, row2) = value.split('{gp_XY}{gp_XY}')
            d.putSubItem('row1', row1)
            d.putSubItem('row2', row2)

# Print 3D vector.
def qdump__gp_XYZ(d, value):
    d.putValue('(%s, %s, %s)' % (value.split('ddd')))
    d.putPlainChildren(value)

# Print 3D vector.
def qdump__gp_Vec(d, value):
    d.putValue('(%s, %s, %s)' % (value.split('ddd')))
    d.putPlainChildren(value)

# Print 3D vector.
def qdump__gp_Dir(d, value):
    d.putValue('(%s, %s, %s)' % (value.split('ddd')))
    d.putPlainChildren(value)

# Print 3D vector.
def qdump__gp_Pnt(d, value):
    d.putValue('(%s, %s, %s)' % (value.split('ddd')))
    d.putPlainChildren(value)

# Print quaternion.
def qdump__gp_Quaternion(d, value):
    d.putValue('(%s, %s, %s, %s)' % (value.split('dddd')))
    d.putPlainChildren(value)

# Print 3x3 matrix as a list of rows (as returned by gp_Mat::Row())
# Note that indexation starts from 1 in this collection, which is preserved in the output.
def qdump__gp_Mat(d, value):
    d.putValue('mat3')
    d.putExpandable()
    if d.isExpanded():
        nbChildren = 4
        with Children(d, nbChildren):
            d.putMembersItem(value, True)
            # indexation starts from 1 in this class
            (row1, row2, row3) = value.split('{gp_XYZ}{gp_XYZ}{gp_XYZ}')
            d.putSubItem('row1', row1)
            d.putSubItem('row2', row2)
            d.putSubItem('row3', row3)

# Print triangle indices.
def qdump__Poly_Triangle(d, value):
    d.putValue('(%s, %s, %s)' % (value.split('iii')))
    d.putPlainChildren(value)

# Print 2D vector.
def qdump__NCollection_Vec2(d, value):
    itemType = value.type[0]
    if itemType.name == 'double':
        d.putValue('(%s, %s)' % (value.split('dd')))
    elif itemType.name == 'float':
        d.putValue('(%s, %s)' % (value.split('ff')))
    elif itemType.name == 'int':
        d.putValue('(%s, %s)' % (value.split('ii')))
    else:
        d.putPlainChildren(value)

# Print 3D vector.
def qdump__NCollection_Vec3(d, value):
    itemType = value.type[0]
    if itemType.name == 'double':
        d.putValue('(%s, %s, %s)' % (value.split('ddd')))
    elif itemType.name == 'float':
        d.putValue('(%s, %s, %s)' % (value.split('fff')))
    elif itemType.name == 'int':
        d.putValue('(%s, %s, %s)' % (value.split('iii')))
    else:
        d.putPlainChildren(value)

# Print 3x3 matrix as a list of columns (as returned by NCollection_Mat3::GetColumn())
def qdump__NCollection_Mat3(d, value):
    itemType = value.type[0]
    d.putValue('mat3<>')
    d.putExpandable()
    if d.isExpanded():
        nbChildren = 4
        with Children(d, nbChildren):
            d.putMembersItem(value, True)
            vec3typeStr = 'NCollection_Vec3<%s>' % itemType.name
            (col0, col1, col2) = value.split('{%s}{%s}{%s}' % (vec3typeStr, vec3typeStr, vec3typeStr))
            d.putSubItem('col0', col0)
            d.putSubItem('col1', col1)
            d.putSubItem('col2', col2)

# Print 4D vector.
def qdump__NCollection_Vec4(d, value):
    itemType = value.type[0]
    if itemType.name == 'double':
        d.putValue('(%s, %s, %s, %s)' % (value.split('dddd')))
    elif itemType.name == 'float':
        d.putValue('(%s, %s, %s, %s)' % (value.split('ffff')))
    elif itemType.name == 'int':
        d.putValue('(%s, %s, %s, %s)' % (value.split('iiii')))
    else:
        d.putPlainChildren(value)

# Print 4x4 matrix as a list of columns (as returned by NCollection_Mat4::GetColumn())
def qdump__NCollection_Mat4(d, value):
    itemType = value.type[0]
   d.putValue('mat4<>')
    d.putExpandable()
    if d.isExpanded():
        nbChildren = 5
        with Children(d, nbChildren):
            d.putMembersItem(value, True)
            vec4typeStr = 'NCollection_Vec4<%s>' % itemType.name
            (col0, col1, col2, col3) = value.split('{%s}{%s}{%s}{%s}' % (vec4typeStr, vec4typeStr, vec4typeStr, vec4typeStr))
            d.putSubItem('col0', col0)
            d.putSubItem('col1', col1)
            d.putSubItem('col2', col2)
            d.putSubItem('col3', col3)

# List elements of array with indexation, stored by collection instance via Lower and Upper methods.
def qdump__NCollection_Array1(d, value):
    itemType     = value.type[0]
    itemSize     = itemType.size()
    myData       = value['myData'].pointer()
    myLowerBound = value['myLowerBound'].integer()
    myUpperBound = value['myUpperBound'].integer()
    myDeletable  = value['myDeletable'].integer()
    if myDeletable == 0:
        d.putValue('occ_arr1<%s> [%d, %d] (view)' % (itemType.name, myLowerBound, myUpperBound))
    else:
        d.putValue('occ_arr1<%s> [%d, %d]' % (itemType.name, myLowerBound, myUpperBound))
    d.putExpandable()
    if d.isExpanded():
        nbChildren = (myUpperBound - myLowerBound + 1) + 1
        with Children(d, nbChildren, maxNumChild=1000):
            d.putMembersItem(value, True)
            # warning! myData stores a pointer with myLowerBound offset
            for i in range(myLowerBound, myUpperBound + 1):
                aVal = d.createValue(myData + itemSize * i, itemType.name)
                d.putSubItem('[%d]' % i, aVal)

# List elements of linked list.
def qdump__NCollection_List(d, value):
    itemType     = value.type[0]
    nodeTypeName = 'NCollection_TListNode<%s>' % itemType.name
    myLength     = value['myLength'].integer()
    d.putValue('list<%s> [%d]' % (itemType.name, myLength))
    d.putExpandable()
    if d.isExpanded():
        nbChildren = myLength + 1
        aNodeIter  = value['myFirst']
        with Children(d, nbChildren, maxNumChild=1000):
            d.putMembersItem(value, True)
            for i in range(0, myLength):
                aTypedNode = d.createValue(aNodeIter.pointer(), nodeTypeName)
                d.putSubItem('[%d]' % i, aTypedNode['myValue'])
                aNodeIter = aNodeIter['myNext']

# List elements of a sequence collection (linked list with indexes).
# Note that indexation starts from 1 in this collection, which is preserved in the output.
def qdump__NCollection_Sequence(d, value):
    itemType     = value.type[0]
    nodeTypeName = 'NCollection_Sequence<%s>::Node' % itemType.name
    mySize       = value['mySize'].integer()
    d.putValue('occ_seq<%s> [%d]' % (itemType.name, mySize))
    d.putExpandable()
    if d.isExpanded():
        nbChildren = mySize + 1
        with Children(d, nbChildren, maxNumChild=1000):
            d.putMembersItem(value, True)
            aNodeIter = value['myFirstItem']
            for i in range(1, mySize + 1):
                aTypedNode = d.createValue(aNodeIter.pointer(), nodeTypeName)
                d.putSubItem('[%d]' % i, aTypedNode['myValue'])
                aNodeIter = aNodeIter['myNext']

# List elements of indexed map by their index in array (myData2).
# Note that indexation starts from 1 in this collection, which is preserved in the output.
def qdump__NCollection_IndexedMap(d, value):
    keyType = value.type[0]
    nodePtrTypeName = 'NCollection_TListNode<%s>*' % keyType.name
    nodePtrSize = d.createType(nodePtrTypeName).size()
    myData1 = value['myData1']
    myData2 = value['myData2']
    mySize  = value['mySize'].integer()
    myNbBuckets = value['myNbBuckets'].integer()
    d.putValue('occ_imap<%s> [%d]' % (keyType.name, mySize))
    d.putExpandable()
    if d.isExpanded():
        nbChildren = mySize + 4
        with Children(d, nbChildren, maxNumChild=1000):
            d.putMembersItem(value, True)
            d.putArrayItem('myData1', myData1.pointer(), myNbBuckets, nodePtrTypeName)
            d.putArrayItem('myData2', myData2.pointer(), myNbBuckets, nodePtrTypeName)
            for i in range(0, mySize):
                aNodeIter = d.createValue(myData2.pointer() + nodePtrSize * i, nodePtrTypeName)
                # indexation starts from 1 in this collection
                d.putSubItem('[%d]' % (i + 1), aNodeIter['myValue'])

# List elements of hashed map from buckets stored in (myData1).
# Note that elements are listed in arbitrary order in this map.
def qdump__NCollection_Map(d, value):
    keyType = value.type[0]
    nodePtrTypeName = 'NCollection_TListNode<%s>*' % keyType.name
    nodePtrSize = d.createType(nodePtrTypeName).size()
    myData1     = value['myData1'].pointer()
    mySize      = value['mySize'].integer()
    myNbBuckets = value['myNbBuckets'].integer()
    d.putValue('occ_map<%s> [%d]' % (keyType.name, mySize))
    d.putExpandable()
    if d.isExpanded():
        nbChildren = mySize + 2
        with Children(d, nbChildren, maxNumChild=1000):
            d.putMembersItem(value, True)
            d.putArrayItem('myData1', myData1, myNbBuckets, nodePtrTypeName)
            aValCounter = 0
            aBuckIter = 0
            aNodeIter = None
            while aValCounter < mySize:
                if aNodeIter != None and aNodeIter.pointer() != 0:
                    d.putSubItem('[%d]' % aValCounter, aNodeIter['myValue'])
                    aValCounter += 1
                    aNextPtr = aNodeIter['myNext']
                    if aNextPtr.pointer() != 0:
                        aNodeIter = d.createValue(aNextPtr.address(), nodePtrTypeName)
                    else:
                        aNodeIter = None
                else:
                    if aBuckIter >= myNbBuckets:
                        break
                    aNodeIter = d.createValue(myData1 + nodePtrSize * aBuckIter, nodePtrTypeName)
                    aBuckIter += 1

# List elements of indexed data map by their index in array (myData2).
# The elements are represented as a key+value pair.
# Note that indexation starts from 1 in this collection, which is preserved in the output.
def qdump__NCollection_IndexedDataMap(d, value):
    keyType   = value.type[0]
    valueType = value.type[1]
    # TODO unable to define such a type
    #nodePtrTypeName = 'NCollection_TListNode<std::pair<%s,%s>>*' % (keyType.name, valueType.name)
    nodePtrTypeName = 'NCollection_TListNode<%s>*' % keyType.name
    nodePtrSize = d.createType(nodePtrTypeName).size()
    myData1     = value['myData1'].pointer()
    myData2     = value['myData2'].pointer()
    mySize      = value['mySize'].integer()
    myNbBuckets = value['myNbBuckets'].integer()
    d.putValue('occ_imap<%s,%s> [%d]' % (keyType.name, valueType.name, mySize))
    d.putExpandable()
    if d.isExpanded():
        nbChildren = mySize + 3
        with Children(d, nbChildren, maxNumChild=1000):
            d.putMembersItem(value, True)
            d.putArrayItem('myData1', myData1, myNbBuckets, nodePtrTypeName)
            d.putArrayItem('myData2', myData2, myNbBuckets, nodePtrTypeName)
            for i in range(0, mySize):
                aNodeIter = d.createValue(myData2 + nodePtrSize * i, nodePtrTypeName)
                aVal = d.createValue(aNodeIter['myValue'].address(), 'std::pair<%s,%s>' % (keyType.name, valueType.name))
                # indexation starts from 1 in this collection
                d.putSubItem('[%d]' % (i + 1), aVal)

# List elements of hashed data map from buckets stored in (myData1).
# The elements are represented as a key+value pair.
# Note that elements are listed in arbitrary order in this map.
def qdump__NCollection_DataMap(d, value):
    keyType   = value.type[0]
    valueType = value.type[1]
    # TODO unable to define such a type
    #nodePtrTypeName = 'NCollection_TListNode<std::pair<%s,%s>>*' % (keyType.name, valueType.name)
    nodePtrTypeName = 'NCollection_TListNode<%s>*' % keyType.name
    nodePtrSize = d.createType(nodePtrTypeName).size()
    myData1     = value['myData1'].pointer()
    mySize      = value['mySize'].integer()
    myNbBuckets = value['myNbBuckets'].integer()
    d.putValue('occ_map<%s,%s> [%d]' % (keyType.name, valueType.name, mySize))
    d.putExpandable()
    if d.isExpanded():
        nbChildren = mySize + 2
        with Children(d, nbChildren, maxNumChild=1000):
            d.putMembersItem(value, True)
            d.putArrayItem('myData1', myData1, myNbBuckets, nodePtrTypeName)
            aValCounter = 0
            aBuckIter = 0
            aNodeIter = None
            while aValCounter < mySize:
                if aNodeIter != None and aNodeIter.pointer() != 0:
                    aVal = d.createValue(aNodeIter['myValue'].address(), 'std::pair<%s,%s>' % (keyType.name, valueType.name))
                    d.putSubItem('[%d]' % aValCounter, aVal)
                    aValCounter += 1
                    aNextPtr = aNodeIter['myNext']
                    if aNextPtr.pointer() != 0:
                        aNodeIter = d.createValue(aNextPtr.address(), nodePtrTypeName)
                    else:
                        aNodeIter = None
                else:
                    if aBuckIter >= myNbBuckets:
                        break
                    aNodeIter = d.createValue(myData1 + nodePtrSize * aBuckIter, nodePtrTypeName)
                    aBuckIter += 1

# List elements of double-indexed vector.
def qdump__NCollection_Vector(d, value):
    valueType  = value.type[0]
    memBlockTypeName = 'NCollection_BaseVector::MemBlock'
    memBlockSize = d.createType(memBlockTypeName).size()
    myItemSize  = value['myItemSize'].integer()
    myIncrement = value['myIncrement'].integer()
    myLength    = value['myLength'].integer()
    myNBlocks   = value['myNBlocks'].integer()
    d.putValue('occ_vector<%s> [%d]' % (valueType.name, myLength))
    d.putExpandable()
    if d.isExpanded():
        nbChildren = myLength + 1
        with Children(d, nbChildren, maxNumChild=1000):
            d.putMembersItem(value, True)
            aBlockIter = value['myData']
            anIterInBlock = 0
            aValCounter = 0
            while aValCounter < myLength:
                if anIterInBlock >= myIncrement:
                    anIterInBlock = 0
                    aBlockPtr = aBlockIter.pointer() + memBlockSize
                    aBlockIter = d.createValue(aBlockPtr, memBlockTypeName)
                aBlockData = aBlockIter['DataPtr'].pointer()
                aVal = d.createValue(aBlockData + anIterInBlock * valueType.size(), valueType)
                d.putSubItem('[%d]' % aValCounter, aVal)
                aValCounter  += 1
                anIterInBlock += 1

# List elements of (packed) integer map from buckets stored in (myData1).
# Note that elements are listed in arbitrary order in this map.
def qdump__TColStd_PackedMapOfInteger(d, value):
    myData1     = value['myData1'].pointer()
    myExtent    = value['myExtent'].integer()
    myNbBuckets = value['myNbBuckets'].integer()
    # this is static constant
    MASK_HIGH   = value['MASK_HIGH'].integer()
    d.putValue('occ_pmap<int> [%d]' % myExtent)
    d.putExpandable()
    if d.isExpanded():
        nbChildren = myExtent + 2
        with Children(d, nbChildren, maxNumChild=1000):
            d.putMembersItem(value, True)
            d.putArrayItem('myData1', myData1, myNbBuckets, 'TColStd_PackedMapOfInteger::TColStd_intMapNode*')
            aValCounter = 0
            aBuckIter = 0
            aNodeIter = None
            # ~0U of 32-bit integer
            anIntMask = 0xFFFFFFFF
            while aValCounter < myExtent:
                if aNodeIter != None and aNodeIter.pointer() != 0:
                    # see TColStd_intMapNode_findNext
                    val = (aNodeIter['myData'].integer() & anIntMask);
                    if val == 0:
                        anIntMask = 0xFFFFFFFF
                        aNodeIter = aNodeIter['myNext']
                        continue

                    aMask = 0xFFFFFFFF
                    nZeros = 0
                    if (val & 0x0000ffff) == 0:
                        aMask = 0xffff0000
                        nZeros = 16
                        val >>= 16
                    if (val & 0x000000ff) == 0:
                        aMask <<= 8
                        nZeros += 8
                        val  >>= 8
                    if (val & 0x0000000f) == 0:
                        aMask <<= 4
                        nZeros += 4
                        val  >>= 4
                    if (val & 0x00000003) == 0:
                        aMask <<= 2
                        nZeros += 2
                        val  >>= 2
                    if (val & 0x00000001) == 0:
                        aMask <<= 1
                        nZeros += 1
                    anIntMask = aMask << 1
                    aKey = nZeros + (aNodeIter['myMask'].integer() & MASK_HIGH)
                    # TODO find a way to put key as a value
                    d.putSubItem('[%d]=%d' % (aValCounter, aKey), aNodeIter)
                    aValCounter += 1
                else:
                    if aBuckIter >= myNbBuckets:
                        break
                    aNodeIter = value['myData1'][aBuckIter]
                    aBuckIter += 1
