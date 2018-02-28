#include "octreeUtils.fx"
#include "indirectBufferUtils.fx"

// This fx voxelizes scene and creates/updates octree based on received voxels

float4x4 gWorldView;
float4x4 gOrthoProj;

struct emptyRT{};

Texture2D albedoTexture;
Texture2D normalTexture;

uint fragBufferSize;

uint currentOctreeLevel;
bool useNormalMap;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint2 GetAddr( in uint index, in uint texSize, in uint formatSize = 1, in uint offset = 0 )
{
    uint texIndex = index * formatSize + offset;
    return uint2( texIndex % texSize, texIndex / texSize );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FullVertexOut CreateVoxelArrayVS( Vertex_3F3F3F2F vin )
{
    FullVertexOut vout;

    vout.Position = vin.Pos;
    vout.PosH = mul( float4(vin.Pos, 1.0f), gWorldView );
    vout.UV = vin.UV;
    
    // todo mul by inverse world
    vout.Normal = vin.Normal;
    vout.Binormal = vin.Binormal;

    return vout;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
[maxvertexcount(3)]
void CreateVoxelArrayGS( triangle FullVertexOut gin[3], inout TriangleStream<FullVertexOut> triStream )
{
    // rotate triangle according to camera to get more voxel info
    
    float4x4 rotMatX = { 1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, -1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f };

    float4x4 rotMatY = { 0.0f, 0.0f, 1.0f, 0.0f,
                        0.0f, 1.0f, 0.0f, 0.0f,
                        -1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f };

    float4x4 rotMatZ = { 0.0f, -1.0f, 0.0f, 0.0f,
                        1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f };
    

    float4x4 rotMat = { 1.0f, 0.0f, 0.0f, 0.0f,
                        0.0f, 1.0f, 0.0f, 0.0f,
                        0.0f, 0.0f, 1.0f, 0.0f,
                        0.0f, 0.0f, 0.0f, 1.0f };

    FullVertexOut gout[3];
    float4 posH[3];
    
    [unroll]
    for ( uint i = 0; i < 3; i++ )
    {
        // store every vertex data except SV_POSITION
        gout[i] = gin[i];
        posH[i] = gin[i].PosH;
    }

    // scale triangle to have at least one pixel
    float minScale = 1.0f;
    float minLen = ( maxBB.x - minBB.x ) / octreeResolution * 0.666f;
    if ( length( posH[0] - posH[1] ) < minLen )
        minScale = max( minScale, minLen / length( posH[0] - posH[1] ) );
        
    if ( length( posH[1] - posH[2] ) < minLen )
        minScale = max( minScale, minLen / length( posH[1] - posH[2] ) );
        
    if ( length( posH[2] - posH[0] ) < minLen )
        minScale = max( minScale, minLen / length( posH[2] - posH[0] ) );

    for ( i = 0; i < 3; i++ )
        posH[i] *= minScale;


    // calculate normal as orientation of triangle
    float3 orientation = abs( cross( posH[0].xyz - posH[1].xyz, posH[0].xyz - posH[2].xyz ) );

    // select the most valuable axis
    if ( orientation.x > orientation.y && orientation.x > orientation.z )
    {
        rotMat = rotMatY;
    }
    else if ( orientation.y > orientation.z )
    {
        rotMat = rotMatX;
    }
    // else orientation.z is max - pass

    [unroll]
    for ( i = 0; i < 3; i++ )
    {
        // rotate triangle accord to valuable axis
        posH[i] = mul( mul( posH[i], rotMat ), gOrthoProj );
    }

    // move new PosH to -1,-1,0
    float4 minCoords = min(min(posH[0], posH[1]), posH[2]) + float4(1.0f, 1.0f, 0.0f, 0.0f);
    minCoords.w = 0.0f;

    [unroll]
    for ( i = 0; i < 3; i++ )
    {
        gout[i].PosH = posH[i] - minCoords;
        triStream.Append(gout[i]);
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
emptyRT CreateVoxelArrayPS( FullVertexOut pin ) : SV_Target
{
    emptyRT output;

    float4 albedo = albedoTexture.Sample(linearSampler, pin.UV);
    
    float3 normal = normalize(pin.Normal);
    if ( useNormalMap )
    {
        float4 localNormal = normalTexture.Sample(linearSampler, pin.UV);
        normal = CalculateNormal( localNormal.xyz, normal, normalize( pin.Binormal ) );
    }
    // pack signed normal to unsigned space
    normal = ( normal + 1.0f ) * 0.5f;
    
    // write values to voxel array
    Voxel voxel;
    voxel.position = WorlPosToOctreePos( pin.Position.xyz );
    voxel.color = PackFloat4ToUint(albedo);
    voxel.normal = PackFloat3ToUint(normal);
    voxel.pad = 0;

    uint bufferSize = voxelArrayRW.IncrementCounter();
    voxelArrayRW[bufferSize] = voxel;

    // write voxels count to indirect buffer
    IndirectIncVoxelCount();

    return output;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FlagNodesVS(uint voxelID: SV_VertexID)
{
    uint nodeIndex = 0;

    // flags nodes that should contains any voxel information    
    if ( TraverseOctree( voxelArrayR[voxelID].position, currentOctreeLevel, nodeIndex ) )
    {
        uint2 flagCoords = GetFlagC(nodeIndex);
        if ( octreeRW[flagCoords] != NODE_ALLOCATED )
        {
            octreeRW[flagCoords] = NODE_SUBDIVIDE;
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void SubdivideNodesVS( uint nodeOffset: SV_VertexID )
{
    // get node id with local node id (nodeOffset) and current level nodes offset in the octree
    uint currentLevelOffset = LevelOffset( currentOctreeLevel );
    uint nodeID = currentLevelOffset + nodeOffset;
    uint nodeIndex = IDToIndex( nodeID );

    uint2 flagCoords = GetFlagC( nodeIndex );
    
    // if nodes is flagged
    if ( octreeRW[flagCoords] == NODE_SUBDIVIDE )
    {
        // allocate 8 nodes for a next level
        uint nodesCount = AllocateNodes( nodeIndex );
        octreeRW[flagCoords] = NODE_ALLOCATED;

        // compute offset for a next level
        uint nodesPerCurrentLevel = NodesCountFromLevel( currentOctreeLevel );
        uint nextLevelOffset = currentLevelOffset + nodesPerCurrentLevel; // constant value per pass
        uint nodesCountPerNextLevel = nodesCount - nextLevelOffset;

        // fill indirect buffer for next octree level
        uint nextOctreeLevel = currentOctreeLevel + 1;
        StoreLevelOffset( nextOctreeLevel, nextLevelOffset );
        StoreMaxNodesCount( nextOctreeLevel, nodesCountPerNextLevel );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ConnectNeighborsVS( uint nodeOffset: SV_VertexID )
{
    // get node index with local node id (nodeOffset) and current level nodes offset in the octree
    uint levelOffset = LevelOffset( currentOctreeLevel );
    uint nodeID = levelOffset + nodeOffset;
    uint nodeIndex = IDToIndex( nodeID );
    
    if ( GetFlag( nodeIndex ) == NODE_ALLOCATED )
    {
        ConnectNeighbors( nodeIndex );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ConnectNodesToVoxelsVS( uint voxelID: SV_VertexID )
{
    uint leafPlace = 0;
    
    if ( TraverseOctree( voxelArrayR[voxelID].position, octreeHeight, leafPlace ) )
    {
        octreeRW[IndexToCoords( leafPlace )] = voxelID;
        octreeRW[GetFlagC( IndexToCoords( leafPlace - leafPlace % SIZE_OF_NODE_STRUCT ) )] = NODE_ALLOCATED; // this is a hack but I don't remember why

        // TODO check how to merge values if needed
        // write to node flag value thats it's not empty

        // maybe write to deffered voxel array and merge it after
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

technique11 GenerateOctree
{
    // during this pass static geometry is rasterized and written to the voxel array
    pass CreateVoxelArray
    {
        SetVertexShader( CompileShader( vs_5_0, CreateVoxelArrayVS() ) );
        SetGeometryShader( CompileShader( gs_5_0, CreateVoxelArrayGS() ) );
        SetPixelShader( CompileShader( ps_5_0, CreateVoxelArrayPS() ) );
    }
    
    // during these passes we construct octree level by level
    // for (currentOctreeLevel = 0; currentOctreeLevel < octreeHeight - 1; currentOctreeLevel++)
    // {
        // use indirect drawcall with indirectDrawBuffer

        pass FlagNodes
        {
            SetVertexShader( CompileShader( vs_5_0, FlagNodesVS() ) );
            SetGeometryShader( NULL );
            SetPixelShader( NULL );
        }

        pass SubdivideNodes
        {
            SetVertexShader( CompileShader( vs_5_0, SubdivideNodesVS() ) );
            SetGeometryShader( NULL );
            SetPixelShader( NULL );
        }

        pass ConnectNeighbors
        {
            SetVertexShader( CompileShader( vs_5_0, ConnectNeighborsVS() ) );
            SetGeometryShader( NULL );
            SetPixelShader( NULL );
        }

    // }

    pass ConnectNodesToVoxels
    {
        SetVertexShader( CompileShader( vs_5_0, ConnectNodesToVoxelsVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( NULL );
    }

    // additional pass to merge voxels ?
    //     think about dynamic part of octree?
}