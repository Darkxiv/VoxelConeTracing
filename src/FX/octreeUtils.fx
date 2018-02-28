#include "utils.fx"

// octree FX specific header file

cbuffer cbSceneBB
{
    float3 minBB;
    float3 maxBB;
};

uint octreeHeight;
uint octreeResolution; // logical octree size: 1 << octreeHeight
uint octreeBufferSize; // texture size

RWStructuredBuffer<uint> nodesPackCounter : register(u0);
RWTexture2D<uint> octreeRW : register(u1); // can't be struct - should use R32 buffer because it takes to lock nodes flag // u1 - effect11 issue
Texture2D<uint> octreeR;

// octree node struct
//    -x -y -z <----------- 0 // subnodes for current node
//    +x -y -z                // at the last level leaf is a pointer (index) to voxel array
//    -x +y -z
//    +x +y -z
//    -x -y +z
//    +x -y +z
//    -x +y +z
//    +x +y +z
//    -x <----------------- 8 // neighbors - pointer (index) to octree
//    +x
//    -y
//    +y
//    -y
//    +z
//    parent <------------- 14
//    flags <-------------- 15 // see NODE_XXX values
#define MAX_OCTREE_HEIGHT 8
#define CHILDS_COUNT 8
#define SIZE_OF_NODE_STRUCT 16

#define NODE_UNDEFINED 0xffffffff
#define NODE_SUBDIVIDE 0xff00ff00
#define NODE_ALLOCATED 0x0
#define NODE_LIT       0x00000100 // first 8 bits for lit voxel in the node from the last level (possibly unnecessary)

struct Voxel
{
    uint position;
    uint color;
    uint normal;
    uint pad; // 128 bits aligment
};

RWStructuredBuffer<Voxel> voxelArrayRW; // or we can try to use AppendStructuredBuffer instead
StructuredBuffer<Voxel> voxelArrayR;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint CoordsToIndex( uint2 coords )
{
    return coords.y * octreeBufferSize + coords.x;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint2 IndexToCoords( uint index )
{
    return uint2( index % octreeBufferSize, index / octreeBufferSize );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint GetFlag( uint nodeIndex )
{
    return octreeRW[IndexToCoords( nodeIndex + 15 )];
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint GetParent( uint nodeIndex )
{
    return octreeRW[IndexToCoords( nodeIndex + 14 )];
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint GetParent( uint2 nodeCoords )
{
    return octreeRW[uint2( nodeCoords.x + 14, nodeCoords.y )];
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint GetParentR( uint2 nodeCoords )
{
    return octreeR[uint2( nodeCoords.x + 14, nodeCoords.y )];
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint GetChild( uint nodeIndex, uint offset )
{
    return octreeRW[IndexToCoords( nodeIndex + offset )];
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint GetChild( uint nodeIndex, uint3 mask )
{
    uint offset = mask.x + ( mask.y << 1 ) + ( mask.z << 2 );
    return octreeRW[IndexToCoords( nodeIndex + offset )];
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint GetChild( uint2 nodeCoords, uint3 mask )
{
    uint2 offset = uint2( mask.x + ( mask.y << 1 ) + ( mask.z << 2 ), 0);
    return octreeRW[nodeCoords + offset];
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint NeighborMaskToOffset( uint3 mask )
{
    // mask.x == 0 - not a x-neighbor
    // mask.x == 1 - negative x
    // mask.x == 2 - positive x
    
    //        x-offset              y-offset                         z-offset
    return ( 0 + mask.x ) + ( 2 * step( 1, mask.y ) + mask.y ) + ( 4 * step( 1, mask.z ) + mask.z );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint GetNodeNeighbor( uint nodeIndex, uint3 mask )
{
    uint offset = NeighborMaskToOffset( mask );
    return octreeRW[IndexToCoords( nodeIndex + 7 + offset )];
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint GetNodeNeighborR( uint nodeIndex, uint3 mask )
{
    uint offset = NeighborMaskToOffset( mask );
    return octreeR[IndexToCoords( nodeIndex + 7 + offset )];
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint2 GetParentC( uint nodeIndex )
{
    return IndexToCoords( nodeIndex + 14 );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint2 GetFlagC( uint nodeIndex )
{
    return IndexToCoords( nodeIndex + 15 );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint2 GetChildC( uint nodeIndex, uint offset )
{
    return IndexToCoords( nodeIndex + offset );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint2 GetChildC( uint nodeIndex, uint3 mask )
{
    uint offset = mask.x + ( mask.y << 1 ) + ( mask.z << 2 );
    return IndexToCoords( nodeIndex + offset );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint GetChildI( uint nodeIndex, uint3 mask )
{
    uint offset = mask.x + ( mask.y << 1 ) + ( mask.z << 2 );
    return nodeIndex + offset;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint2 GetNodeNeighborC( uint nodeIndex, uint3 mask )
{
    uint offset = NeighborMaskToOffset( mask );
    return IndexToCoords( nodeIndex + 7 + offset );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint2 GetParentC( uint2 nodeCoords )
{
    return uint2( nodeCoords.x + 14, nodeCoords.y );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint2 GetFlagC( uint2 nodeCoords )
{
    return uint2( nodeCoords.x + 15, nodeCoords.y );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint2 GetChildC( uint2 nodeCoords, uint offset )
{
    return uint2( nodeCoords.x + offset, nodeCoords.y );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint2 GetChildC( uint2 nodeCoords, uint3 mask )
{
    uint offset = mask.x + ( mask.y << 1 ) + ( mask.z << 2 );
    return uint2( nodeCoords.x + offset, nodeCoords.y );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint2 GetNodeNeighborC( uint2 nodeCoords, uint3 mask )
{
    uint offset = NeighborMaskToOffset( mask );
    return uint2( nodeCoords.x + 7 + offset, nodeCoords.y );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint IDToIndex( uint nodeID )
{
    // index - is an index inside octree
    // nodeID - is a index inside array (if we treat octree as array of nodes)
    return nodeID * SIZE_OF_NODE_STRUCT;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint IndexToID( uint index )
{
    return index / SIZE_OF_NODE_STRUCT;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint WorlPosToOctreePos( float3 pos )
{
    return PackUint3ToUint( uint3( ( pos - minBB ) / ( maxBB - minBB ) * octreeResolution ) );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float3 OctreePosToWorldPos( uint pos )
{
    return float3( UnpackUintToUint3( pos ) ) / octreeResolution * ( maxBB - minBB ) + minBB;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint AllocateNodes( in uint nodeIndex )
{
    // allocate 8 nodes, write childs/parent adresses
    // return nodes count
    
    uint parentIndex = nodeIndex;
    uint nodesPackCount = nodesPackCounter.IncrementCounter();
    uint index = nodesPackCount * CHILDS_COUNT + 1; // 1 is for preallocated root node
    
    [unroll]
    for ( uint i = 0; i < CHILDS_COUNT; i++ )
    {
        // convert index to octree adress
        uint childIndex = ( index + i ) * SIZE_OF_NODE_STRUCT;
        octreeRW[GetChildC( nodeIndex, i )] = childIndex;
        octreeRW[GetParentC( childIndex )] = parentIndex;
    }

    return ( nodesPackCount + 1 ) * CHILDS_COUNT + 1;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SelectNode( in int3 pos, inout uint3 mask, inout int3 halfSize, inout int3 curA, inout uint nodeValue )
{
    // generate mask to select node from the next level
    halfSize = halfSize >> 1;
    int3 relativePosition = pos - ( curA + halfSize );
    mask = uint3( step( 0, sign( relativePosition ) ) );
    curA = curA + halfSize * mask;

    // find appropriate child coords
    nodeValue = GetChildI( nodeValue, mask );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool TraverseOctree( in uint vPos, in uint currentLevel, out uint nodeValue )
{
    nodeValue = 0;

    if ( currentLevel > octreeHeight )
        return false;

    int3 pos = UnpackUintToUint3(vPos);
    if ( any( step( octreeResolution, pos ) ) )
        return false;
    
    int3 halfSize = octreeResolution;
    int3 nodeCoords = 0; // A = 0, B is an A + halfSize
    uint3 mask = 0;

    [unroll(MAX_OCTREE_HEIGHT)]
    for ( uint treeLevel = 0; treeLevel < currentLevel; treeLevel++ )
    {
        // calc node relative postion in the octree
        SelectNode( pos, mask, halfSize, nodeCoords, nodeValue );

        // for the last octree level 
        // return a place for pointer to voxel array
        if ( treeLevel != ( octreeHeight - 1 ) )
        {
            nodeValue = octreeRW[IndexToCoords( nodeValue )];

            if ( nodeValue == NODE_UNDEFINED )
                return false;
        }
    }

    return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// read-only octree TraverseOctree version
bool TraverseOctreeR( in uint vPos, in uint currentLevel, out uint nodeValue, out int3 nodeCoords )
{
    nodeValue = 0;

    if ( currentLevel > octreeHeight )
        return false;

    int3 pos = UnpackUintToUint3(vPos);
    if ( any( step( octreeResolution, pos ) ) )
        return false;
    
    int3 halfSize = octreeResolution;
    nodeCoords = 0;
    uint3 mask = 0;
    
    [unroll(MAX_OCTREE_HEIGHT)]
    for ( uint treeLevel = 0; treeLevel < currentLevel; treeLevel++ )
    {
        SelectNode( pos, mask, halfSize, nodeCoords, nodeValue );

        if ( treeLevel != ( octreeHeight - 1 ) )
        {
            nodeValue = octreeR[IndexToCoords( nodeValue )].r;

            if ( nodeValue == NODE_UNDEFINED )
                return false;
        }
    }

    return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// traverse octree and write photon flags
bool PhotonTraverseOctree( in uint vPos, out uint nodeValue, out uint parentIndex, out uint3 mask )
{
    nodeValue = 0;
    parentIndex = 0;
    uint2 flagC = 0;
    
    int3 pos = UnpackUintToUint3(vPos);
    if ( any( step( octreeResolution, pos ) ) )
        return false;
    
    int3 halfSize = octreeResolution;
    int3 nodeCoords = 0;
    
    [unroll(MAX_OCTREE_HEIGHT)]
    for ( uint treeLevel = 0; treeLevel < octreeHeight; treeLevel++ )
    {
        parentIndex = nodeValue;
        
        // offset pointer
        SelectNode( pos, mask, halfSize, nodeCoords, nodeValue );
        
        // get pointer to the next node (or voxelID value)
        nodeValue = octreeRW[IndexToCoords( nodeValue )].r;
        
        if ( nodeValue == NODE_UNDEFINED )
        {
            return false;
        }
        else if ( treeLevel != octreeHeight - 1 ) 
        {
            // write values to octree flag that we was here (write flag for the last level after the loop)
            flagC = GetFlagC( parentIndex );
            octreeRW[flagC] = NODE_LIT;
        }
    }

    flagC = GetFlagC( parentIndex );

    // each bit mean voxel/octan in the node
    uint voxelMask = 1 << ( mask.x + ( mask.y << 1 ) + ( mask.z << 2 ) );
    uint oldFlag = 0;
    
    // mark lit voxel
    // NOTE: with InterlockedOr we prevent redundant writing to brick bufer, but InterlockedOr can cause performance degrade
    // should be tested, is this way really faster... I guess no
    InterlockedOr( octreeRW[flagC], voxelMask | NODE_LIT, oldFlag );
    
    // already was here
    if ( oldFlag & voxelMask )
        return false;

    return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ConnectNeighbors( uint nodeIndex )
{
    // store child nodes neighbors indicies
    
    [unroll]
    for ( uint i = 0; i < CHILDS_COUNT; i++ )
    {
        // get current child
        uint childIndex = GetChild( nodeIndex, i );

        // x, y, z: 0 is negative, 1 is positive
        const uint relativePos[3] = { ( i & 0x1 ) == 1, ( i & 0x2 ) == 0x2, ( i & 0x4 ) == 0x4 };
        
        // 0 - no axis, 1 - negative axis, 2 - positive axis
        const uint3 neighborTable[6] = {
            uint3( 1, 0, 0 ),
            uint3( 2, 0, 0 ),
            uint3( 0, 1, 0 ),
            uint3( 0, 2, 0 ),
            uint3( 0, 0, 1 ),
            uint3( 0, 0, 2 )
        };

        // 0 - negative axis, 1 - positive axis
        const uint3 childsTable[6] = {
            uint3( 0, relativePos[1], relativePos[2] ),
            uint3( 1, relativePos[1], relativePos[2] ),
            uint3( relativePos[0], 0, relativePos[2] ),
            uint3( relativePos[0], 1, relativePos[2] ),
            uint3( relativePos[0], relativePos[1], 0 ),
            uint3( relativePos[0], relativePos[1], 1 )
        };

        // find and save -x, +x, -y, +y, -z, +z neighbors coords
        uint2 neighborCoords = 0;

        [unroll]
        for ( int j = 0; j < 3; j++ )
        {
            //     next code follow this pseudocode:
            //
            //    if (-axis)    // relative axis (x,y,z) of child node inside current (parent) node
            //    {
            //        child.neighbor[-axis] = neighbor[-axis].child[+axis]  // I
            //        child.neighbor[+axis] = child[+axis]                  // II
            //    }
            //    else
            //    {
            //        child.neighbor[+axis] = neighbor[+axis].child[-axis]  // I
            //        child.neighbor[-axis] = child[-axis]                  // II
            //    }
            //
        
            uint3 neighborIndex1 = neighborTable[j * 2];
            uint3 neighborIndex2 = neighborTable[j * 2 + 1];

            uint3 relativeChildIndex = childsTable[j * 2 + 1];

            if ( relativePos[j] )
            {
                neighborIndex1 = neighborTable[j * 2 + 1];
                neighborIndex2 = neighborTable[j * 2];

                relativeChildIndex = childsTable[j * 2];
            }
        
            // I
            uint parentNeighborIndex = GetNodeNeighbor( nodeIndex, neighborIndex1 );
            if ( parentNeighborIndex != 0xffffffff && GetFlag( parentNeighborIndex ) == NODE_ALLOCATED )
            {
                neighborCoords = GetNodeNeighborC( childIndex, neighborIndex1 );
                octreeRW[neighborCoords] = GetChild( parentNeighborIndex, relativeChildIndex );
            }

            // II
            neighborCoords = GetNodeNeighborC( childIndex, neighborIndex2 );
            octreeRW[neighborCoords] = GetChild( nodeIndex, relativeChildIndex );
        }
    }
}
