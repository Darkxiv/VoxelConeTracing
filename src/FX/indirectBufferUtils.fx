
// FX specific header file

RWBuffer<uint> indirectDrawBuffer;
#define INDIRECT_STRUCTURE_SIZE 4
// use this indirectDrawBuffer as buffer for DrawInstancedIndirect calls
// struct INDIRECT_STRUCTURE
// {
//      uint vCount;
//      uint iCount;        // always 1
//      uint StartVertex;
//      uint StartInstance;
// };
// StartVertex and StartInstance can be unused since we don't use vertex buffer or more than one instance for these calls
// also we use this buffer as storage for voxels count, nodes per octree level count and nodes offset per octree level
//
// we use VS threads for octree nodes computations based on voxels information
#define INDIRECT_VOXELS_OFFSET 0
// for voxels we treat this struct as
//      struct INDIRECT_STRUCTURE
//      {
//          uint voxelsCount;
//          uint reserved;
//          uint unused;
//          uint unused;
//      };
#define INDIRECT_OCTREE_OFFSET 4
// for octree computations we treat this struct as
//      struct INDIRECT_STRUCTURE
//      {
//          uint nodesCountPerLevel;
//          uint reserved;
//          uint nodesOffsetPerLevel;
//          uint unused;
//      };
//
// so buffer contains:
// offset: 0------------------------4---------------------------------------------------------8--------------------------------------------------------16-----
// data:   [ voxelsCount; 1; 0; 0; ][ nodesCount for level 0; 1; nodesOffset for level 0; 0; ][ nodesCount for level 1; 1; nodesOffset for level 1; 0; ] . . .


///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint IndirectIndex( uint octreeLevel )
{
    return INDIRECT_OCTREE_OFFSET + octreeLevel * INDIRECT_STRUCTURE_SIZE;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint NodesCountFromLevel( uint octreeLevel )
{
    return indirectDrawBuffer[ IndirectIndex( octreeLevel ) ];
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint LevelOffset( uint octreeLevel )
{
    return indirectDrawBuffer[ IndirectIndex( octreeLevel ) + 2 ];
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void StoreMaxNodesCount( uint octreeLevel, uint nodesCount )
{
    InterlockedMax( indirectDrawBuffer[ IndirectIndex( octreeLevel ) ], nodesCount );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void StoreLevelOffset( uint octreeLevel, uint offset )
{
    indirectDrawBuffer[ IndirectIndex( octreeLevel ) + 2 ] = offset;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void IndirectIncVoxelCount()
{
    InterlockedAdd( indirectDrawBuffer[INDIRECT_VOXELS_OFFSET], 1 );
}
