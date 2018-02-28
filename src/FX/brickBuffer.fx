#include "octreeUtils.fx"
#include "indirectBufferUtils.fx"
#include "brickBufferUtils.fx"

// This FX generates brick buffers and injects irradiance

RWTexture3D<uint> opacityBrickBufferRW;
RWTexture3D<uint> irradianceBrickBufferRW;
RWTexture3D<uint> brickBufferRW;
uint currentOctreeLevel;

Texture3D<float4> brickBufferR;

float4x4 gWorldViewProj;
float4x4 gShadowInverseView;
float4x4 gShadowInverseProj;

Texture2D shadowMap;
uint2 shadowMapResolution;

cbuffer LightProps
{
    float4x4 gLightProj;
    float4 lColorRadius;
    float4 lPos;
    float4 lDirection;
};

struct FullScreenQuadOut
{
    float4 PosH : SV_POSITION;
    float2 UV: TEXCOORD;
};

struct DebugRT
{
    float4 Color;
};

struct emptyRT {};

uint debugOctreeLevel;

struct DebugVOut
{
    uint nodeOffset : BLENDINDICES;
};

struct DebugGOut
{
    float4 posH : SV_POSITION;
    float4 wPosOrCol : POSITION; // use as color if we draw voxels
};

#define TEXELS_COUNT 27 // 3x3x3
#define EPS 0.0001f

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AverageTexelsArray( inout float4 texels[TEXELS_COUNT] )
{
//
//                Z                                          interpolate values between corners
//               /\
//  Y           /
//  \        5.0----|*|----6.0                         5.0----5.5----6.0                         5.0----5.5----6.0                         5.0----5.5----6.0
//  |\       /      /      /|                          /      /      /|                          /      /      /|                          /      /      /|
//  |      /      /      /  |                        /      /      /  |                        /      /      /  |                        /      /      /  |
//  |   |*|----|*|----|*|  |*|                    |*|----|*|----|*|  |*|                    |*|----|*|----|*|  7.0                    3.0----3.5----4.0  7.0
//  |   /      /      /|   /|   Interpolate       /      /      /|   /|   Interpolate       /      /      /|   /|   Interpolate       /      /      /|   /|
//  | /      /      /  | /  |  edges along X    /      /      /  | /  |  edges along Y    /      /      /  | /  |  edges along Z    /      /      /  | /  |
// 1.0----|*|----2.0  |*|  8.0 ------------> 1.0----1.5----2.0  |*|  8.0 ------------> 1.0----1.5----2.0  |*|  8.0 ------------> 1.0----1.5----2.0  5.0  8.0
//  |      |      |   /|   /     4 values     |      |      |   /|   /     6 values     |      |      |   /|   /     9 values     |      |      |   /|   /
//  |      |      | /  | /                    |      |      | /  | /                    |      |      | /  | /                    |      |      | /  | /
// |*|----|*|----|*|  |*|                    |*|----|*|----|*|  |*|                    2.0----2.5----3.0  |*|                    2.0----2.5----3.0  6.0
//  |      |      |   /                       |      |      |   /                       |      |      |   /                       |      |      |   /
//  |      |      | /                         |      |      | /                         |      |      | /                         |      |      | /
// 3.0----|*|----4.0------> X                3.0----3.5----4.0                         3.0----3.5----4.0                         3.0----3.5----4.0
//

    // along X
    const uint sizeAlongX = 4;
    const uint3 coordsAlongX[sizeAlongX] = {
        uint3( 1, 0, 0 ), 
        uint3( 1, 2, 0 ),
        uint3( 1, 0, 2 ),
        uint3( 1, 2, 2 )
    };
    
    [unroll]
    for ( uint i = 0; i < sizeAlongX; i++ )
    {
        uint index = CoordsToBrickIndex( coordsAlongX[i] );
        uint indexA = CoordsToBrickIndex( uint3( 0, coordsAlongX[i].yz ) );
        uint indexB = CoordsToBrickIndex( uint3( 2, coordsAlongX[i].yz ) );
        texels[index] = ( texels[indexA] + texels[indexB] ) * 0.5f;
    }

    // along Y
    const uint sizeAlongY = 6;
    const uint3 coordsAlongY[sizeAlongY] = {
        uint3( 0, 1, 0 ),
        uint3( 1, 1, 0 ),
        uint3( 2, 1, 0 ),
        uint3( 0, 1, 2 ),
        uint3( 1, 1, 2 ),
        uint3( 2, 1, 2 )
    };
    
    [unroll]
    for ( i = 0; i < sizeAlongY; i++ )
    {
        uint index = CoordsToBrickIndex( coordsAlongY[i] );
        uint indexA = CoordsToBrickIndex( uint3( coordsAlongY[i].x, 0, coordsAlongY[i].z ) );
        uint indexB = CoordsToBrickIndex( uint3( coordsAlongY[i].x, 2, coordsAlongY[i].z ) );
        texels[index] = ( texels[indexA] + texels[indexB] ) * 0.5f;
    }

    // along Z
    const uint sizeAlongZ = 9;
    const uint3 coordsAlongZ[sizeAlongZ] = {
        uint3( 0, 0, 1 ), uint3( 1, 0, 1 ), uint3( 2, 0, 1 ),
        uint3( 0, 1, 1 ), uint3( 1, 1, 1 ), uint3( 2, 1, 1 ),
        uint3( 0, 2, 1 ), uint3( 1, 2, 1 ), uint3( 2, 2, 1 )
    };
    
    [unroll]
    for ( i = 0; i < sizeAlongZ; i++ )
    {
        uint index = CoordsToBrickIndex( coordsAlongZ[i] );
        uint indexA = CoordsToBrickIndex( uint3( coordsAlongZ[i].xy, 0 ) );
        uint indexB = CoordsToBrickIndex( uint3( coordsAlongZ[i].xy, 2 ) );
        texels[index] = ( texels[indexA] + texels[indexB] ) * 0.5f;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ConstructOpacityVS( uint nodeOffset: SV_VertexID )
{
    // get node index from nodeOffset for given level
    uint levelOffset = LevelOffset( octreeHeight - 1 );
    uint nodeID = levelOffset + nodeOffset;
    uint nodeIndex = IDToIndex( nodeID );

//
//  Map octree node on the brick - later this structure will be used for interpolation values between adjacent nodes
//
//                Z
//               /\
//  Y           /
//  \         |------|------|                          |6|----|*|----|7|
//  |\       /  6   /  7   /|                          /      /      /| 
//  |      /      /      /  |                        /      /      /  | 
//  |    |------|------|  7 |                     |*|----|*|----|*|  |*|
//  |   /  2   /  3   /|   /|                     /      /      /|   /| 
//  | /      /      /  | /  |  map on brick     /      /      /  | /  | 
//  |------|------/  3 |  5 | -------------> |2|----|*|----|3|  |*|  |5|
//  |  2   |  3   |   /|   /     8 values     |      |      |   /|   /  
//  |      |      | /  | /                    |      |      | /  | /    
//  |------|------|  1 |                     |*|----|*|----|*|  |*|     
//  |  0   |  1   |   /                       |      |      |   /       
//  |      |      | /                         |      |      | /         
//  |------|------|------> X                 |0|----|*|----|1|          
//
//    OCTREE NODE                                  BRICK
//     8 values                                  27 values
//

    // init texels array
    float4 texels[TEXELS_COUNT] = { 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx,
                                    0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx,
                                    0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx };
    

    // store 8 values in the corners of brick
    const uint3 cornersCoord[CHILDS_COUNT] = {
        uint3( 0, 0, 0 ), 
        uint3( 2, 0, 0 ),
        uint3( 0, 2, 0 ),
        uint3( 2, 2, 0 ),
        uint3( 0, 0, 2 ),
        uint3( 2, 0, 2 ),
        uint3( 0, 2, 2 ),
        uint3( 2, 2, 2 )
    };

    [unroll]
    for ( uint i = 0; i < CHILDS_COUNT; i++ )
    {
        uint index = CoordsToBrickIndex( cornersCoord[i] );

        if ( octreeR[GetChildC( nodeIndex, i )].r == NODE_UNDEFINED )
            texels[index] = 0.0f;
        else
            texels[index] = 1.0f; // TODO or we should store alpha from voxel? . . .
    }

    AverageTexelsArray( texels );

    // save result into texture
    uint3 brickCoords = NodeIDToTextureCoords( nodeID );

    [unroll]
    for ( i = 0; i < TEXELS_COUNT; i++ )
    {
        uint3 localOffset = IndexToBrickCoords( i );
        opacityBrickBufferRW[brickCoords + localOffset] = PackFloat4ToUint( texels[i] );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AverageLitNodeValuesVS( uint nodeOffset: SV_VertexID )
{
    // mostly the same as ConstructOpacityVS

    // get node index from nodeOffset for given level
    uint levelOffset = LevelOffset( octreeHeight - 1 );
    uint nodeID = levelOffset + nodeOffset;
    uint nodeIndex = IDToIndex( nodeID );

    // we should get octree node flag first and check if it's lit
    uint nodeFlag = octreeR[GetFlagC( nodeIndex )];
    if ( nodeFlag == NODE_UNDEFINED || ( nodeFlag & NODE_LIT ) == 0x0 )
        return;

    // init texels array
    float4 texels[TEXELS_COUNT] = { 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx,
                                    0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx,
                                    0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx };

    // store 8 values in the texels array
    const uint3 cornersCoord[CHILDS_COUNT] = {
        uint3( 0, 0, 0 ), 
        uint3( 2, 0, 0 ),
        uint3( 0, 2, 0 ),
        uint3( 2, 2, 0 ),
        uint3( 0, 0, 2 ),
        uint3( 2, 0, 2 ),
        uint3( 0, 2, 2 ),
        uint3( 2, 2, 2 )
    };

    uint3 brickCoords = NodeIDToTextureCoords( nodeID );

    // copy texels from brickBuffer to average them
    [unroll]
    for ( uint i = 0; i < CHILDS_COUNT; i++ )
    {
        uint index = CoordsToBrickIndex( cornersCoord[i] );
        texels[index] = UnpackUintToFloat4( brickBufferRW[brickCoords + cornersCoord[i]] );
    }

    AverageTexelsArray( texels );

    // save result into texture
    [unroll]
    for ( i = 0; i < TEXELS_COUNT; i++ )
    {
        uint3 localOffset = IndexToBrickCoords( i );
        brickBufferRW[brickCoords + localOffset] = PackFloat4ToUint( texels[i] );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void AverageAlongAxisVS( uint nodeOffset: SV_VertexID, uniform uint axis, uniform bool skipUnlitNodes )
{
    // get brick coords
    uint levelOffset = LevelOffset( currentOctreeLevel );
    uint currentNodeID = levelOffset + nodeOffset;
    uint3 currentCoords = NodeIDToTextureCoords( currentNodeID );

    // get +X, +Y or +Z neighboor mask
    const uint3 neighboorMasks[3] = { uint3( 2, 0, 0 ), uint3( 0, 2, 0 ), uint3( 0, 0, 2 ) };
    uint3 neighboorMask = neighboorMasks[axis];
    
    // get neighboor octree base offset
    uint currentNodeIndex = IDToIndex( currentNodeID );
    uint adjacentNodeIndex = GetNodeNeighborR( currentNodeIndex, neighboorMask );

    // if node is empty - pass
    uint nodeFlag = octreeR[GetFlagC( currentNodeIndex )];
    if ( nodeFlag == NODE_UNDEFINED )
        return;

    if ( skipUnlitNodes )
    {
        // discard unlit nodes
        if ( ( nodeFlag & NODE_LIT ) == 0x0 )
            return;
    }

    // if neighboor doesn't exist brick values stay unchanged
    if ( adjacentNodeIndex == NODE_UNDEFINED )
        return;

    // get neighboor brick coords
    uint adjacentNodeID = IndexToID( adjacentNodeIndex );
    uint3 adjacentCoords = NodeIDToTextureCoords( adjacentNodeID );

    // average adjacent bricks parts
    const uint sizeAlongFace = 9;
    const uint3 coordsAlongFace[3][sizeAlongFace] = {
        {
            // along X
            uint3( 2, 0, 0 ), uint3( 2, 0, 1 ), uint3( 2, 0, 2 ),
            uint3( 2, 1, 0 ), uint3( 2, 1, 1 ), uint3( 2, 1, 2 ),
            uint3( 2, 2, 0 ), uint3( 2, 2, 1 ), uint3( 2, 2, 2 )
        },
        {
            // along Y
            uint3( 0, 2, 0 ), uint3( 0, 2, 1 ), uint3( 0, 2, 2 ),
            uint3( 1, 2, 0 ), uint3( 1, 2, 1 ), uint3( 1, 2, 2 ),
            uint3( 2, 2, 0 ), uint3( 2, 2, 1 ), uint3( 2, 2, 2 )
        },
        {
            // along Z
            uint3( 0, 0, 2 ), uint3( 1, 0, 2 ), uint3( 2, 0, 2 ),
            uint3( 0, 1, 2 ), uint3( 1, 1, 2 ), uint3( 2, 1, 2 ),
            uint3( 0, 2, 2 ), uint3( 1, 2, 2 ), uint3( 2, 2, 2 )
        }
    };

    uint3 adjacentMask = uint3( 1, 1, 1 );
    if      ( axis == 0 ) adjacentMask.x = 0;
    else if ( axis == 1 ) adjacentMask.y = 0;
    else if ( axis == 2 ) adjacentMask.z = 0;
    
    [unroll]
    for ( uint i = 0; i < sizeAlongFace; i++ )
    {
        uint3 currentOffset = coordsAlongFace[axis][i];
        uint3 adjacentOffset = currentOffset * adjacentMask;

        float4 currentValue  = UnpackUintToFloat4( brickBufferRW[currentCoords  + currentOffset] );
        float4 adjacentValue = UnpackUintToFloat4( brickBufferRW[adjacentCoords + adjacentOffset] );
        uint result = PackFloat4ToUint( ( currentValue + adjacentValue ) * 0.5f );
        
        brickBufferRW[currentCoords + currentOffset] = result;
        brickBufferRW[adjacentCoords + adjacentOffset] = result;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
#define SIDES_COUNT 6
static const uint3 sidesCoords[SIDES_COUNT] = {
    uint3( 0, 1, 1 ), uint3( 1, 0, 1 ), uint3( 1, 1, 0 ),
    uint3( 2, 1, 1 ), uint3( 1, 2, 1 ), uint3( 1, 1, 2 )
};

#define EDGES_COUNT 12
static const uint3 edgesCoords[EDGES_COUNT] = {
    uint3( 1, 0, 0 ), uint3( 1, 2, 0 ), uint3( 1, 0, 2 ), uint3( 1, 2, 2 ), 
    uint3( 0, 1, 0 ), uint3( 2, 1, 0 ), uint3( 0, 1, 2 ), uint3( 2, 1, 2 ),
    uint3( 0, 0, 1 ), uint3( 2, 0, 1 ), uint3( 0, 2, 1 ), uint3( 2, 2, 1 )
};

#define CORNERS_COUNT 8
static const uint3 octaneOffset[CORNERS_COUNT] = {
    uint3( 0, 0, 0 ),
    uint3( 1, 0, 0 ),
    uint3( 0, 1, 0 ),
    uint3( 1, 1, 0 ),
    uint3( 0, 0, 1 ),
    uint3( 1, 0, 1 ),
    uint3( 0, 1, 1 ),
    uint3( 1, 1, 1 )
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ApplyOpacityWeightsToTexels( inout float4 texels[TEXELS_COUNT] )
{
    // calculate weighted average

    // center
    texels[CoordsToBrickIndex( uint3( 1, 1, 1 ) )] *= 0.015625f; // 64 values
    
    // sides
    [unroll]
    for ( uint i = 0; i < SIDES_COUNT; i++ )
    {
        texels[CoordsToBrickIndex( sidesCoords[i] )] *= 0.03125f; // 32 values
    }

    // edges
    [unroll]
    for ( i = 0; i < EDGES_COUNT; i++ )
    {
        texels[CoordsToBrickIndex( edgesCoords[i] )] *= 0.0625f; // 16 values
    }

    // corners
    [unroll]
    for ( i = 0; i < CORNERS_COUNT; i++ )
    {
        uint3 cornerCoords = octaneOffset[i] * 2;
        texels[CoordsToBrickIndex( cornerCoords )] *= 0.125f; // 8 values
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ApplyWeightsToTexels( inout float4 texels[TEXELS_COUNT] )
{
    // calculate weighted average

    // center - 64 values
    float weights = texels[CoordsToBrickIndex( uint3( 1, 1, 1 ) )].a;
    texels[CoordsToBrickIndex( uint3( 1, 1, 1 ) )] /= max( weights, EPS );
    
    // sides - 32 values
    [unroll]
    for ( uint i = 0; i < SIDES_COUNT; i++ )
    {
        weights = texels[CoordsToBrickIndex( sidesCoords[i] )].a;
        texels[CoordsToBrickIndex( sidesCoords[i] )] /= max( weights, EPS );
    }

    // edges - 16 values
    [unroll]
    for ( i = 0; i < EDGES_COUNT; i++ )
    {
        weights = texels[CoordsToBrickIndex( edgesCoords[i] )].a;
        texels[CoordsToBrickIndex( edgesCoords[i] )] /= max( weights, EPS );
    }

    // corners - 8 values
    [unroll]
    for ( i = 0; i < CORNERS_COUNT; i++ )
    {
        uint3 cornerCoords = octaneOffset[i] * 2;
        weights = texels[CoordsToBrickIndex( cornerCoords )].a;
        texels[CoordsToBrickIndex( cornerCoords )] /= max( weights, EPS );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ContributeOctaneToTexels( inout float4 texels[TEXELS_COUNT], in float4 brick[TEXELS_COUNT], 
    in uint3 coords, in uint3 tOffset, in bool isIrradiance )
{
    uint3 texelCoords = coords + tOffset;
    uint3 brickCoords = coords;

    // sum up 8 values    
    [unroll]
    for ( uint i = 0; i < CHILDS_COUNT; i++ )
    {
        // should be multiplied by weight but it's taken into account in the ApplyWeightsToTexels / ApplyOpacityWeightsToTexels
        float4 bColor = brick[CoordsToBrickIndex( brickCoords + octaneOffset[i] )];
        if ( isIrradiance )
            bColor.rgb *= bColor.a; // premultiply alpha and divide by overall alpha

        texels[CoordsToBrickIndex( texelCoords )] += bColor;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ContributeOpacityChildsToParent( inout float4 texels[TEXELS_COUNT], uint nodeIndex )
{
    static float4 brick[TEXELS_COUNT];
    
    // each child contribute 8 values to 1 center, 3 sides, 3 edges, 1 corner
    float3 maxv = 0.0f;

    // performance hit: ineffective solution
    [unroll(8)] // error X3511: unable to unroll loop
    for ( uint i = 0; i < CHILDS_COUNT; i++ )
    {
        uint childIndex = octreeR[GetChildC( nodeIndex, i )];
        
        // if child doen't allocated - skip
        if ( octreeR[GetFlagC( childIndex )] == NODE_UNDEFINED )
            continue;

        // convert to brick coords
        uint3 brickCoords = NodeIDToTextureCoords( IndexToID( childIndex ) );

        // copy brick texels to save reads // maybe it's not so fast
        [unroll]
        for ( uint j = 0; j < TEXELS_COUNT; j++ )
        {
            uint3 localOffset = IndexToBrickCoords( j );
            brick[j] = UnpackUintToFloat4( brickBufferRW[brickCoords + localOffset] );
        }
        
        // get max opacity value from each direction
        [unroll]
        for ( uint m = 0; m < 3; m++ )
        {
            [unroll]
            for ( uint n = 0; n < 3; n++ )
            {
                maxv.x = max( max( brick[CoordsToBrickIndex( uint3( 0, m, n ) )].x,
                                   brick[CoordsToBrickIndex( uint3( 1, m, n ) )].x ),
                                   brick[CoordsToBrickIndex( uint3( 2, m, n ) )].x );
                maxv.y = max( max( brick[CoordsToBrickIndex( uint3( m, 0, n ) )].y,
                                   brick[CoordsToBrickIndex( uint3( m, 1, n ) )].y ),
                                   brick[CoordsToBrickIndex( uint3( m, 2, n ) )].y );
                maxv.z = max( max( brick[CoordsToBrickIndex( uint3( m, n, 0 ) )].z,
                                   brick[CoordsToBrickIndex( uint3( m, n, 1 ) )].z ),
                                   brick[CoordsToBrickIndex( uint3( m, n, 2 ) )].z );

                [unroll]
                for ( j = 0; j < 3; j++ )
                {
                    brick[CoordsToBrickIndex( uint3( j, m, n ) )].x = maxv.x;
                    brick[CoordsToBrickIndex( uint3( m, j, n ) )].y = maxv.y;
                    brick[CoordsToBrickIndex( uint3( m, n, j ) )].z = maxv.z;
                }
            }
        }

        // fill texels with octanes from child texels
        uint3 texelOffset = octaneOffset[i];
        [unroll]
        for ( j = 0; j < CHILDS_COUNT; j++ )
        {
            uint3 texelCoords = octaneOffset[j];
            ContributeOctaneToTexels( texels, brick, texelCoords, texelOffset, false );
        }
    }
    
    // takes childs weights into account
    ApplyOpacityWeightsToTexels( texels );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ContributeIrradianceChildsToParent( inout float4 texels[TEXELS_COUNT], uint nodeIndex )
{
    static float4 brick[TEXELS_COUNT];
    
    // each child contribute 8 values to 1 center, 3 sides, 3 edges, 1 corner
    [unroll]
    for ( uint i = 0; i < CHILDS_COUNT; i++ )
    {
        uint childIndex = octreeR[GetChildC( nodeIndex, i )];
        
        // if child doesn't allocated and lit - skip
        uint nodeFlag = octreeR[GetFlagC( childIndex )];
        if ( nodeFlag == NODE_UNDEFINED || ( nodeFlag & NODE_LIT ) == 0x0 )
            continue;
        
        // convert to brick coords
        uint3 brickCoords = NodeIDToTextureCoords( IndexToID( childIndex ) );
        
        // copy brick values to save reads // TODO possibly it doesn't
        [unroll]
        for ( uint j = 0; j < TEXELS_COUNT; j++ )
        {
            uint3 localOffset = IndexToBrickCoords( j );
            
            float4 bColor = UnpackUintToFloat4( brickBufferRW[brickCoords + localOffset] );

            // attach opacity to brick
            float3 opacity = opacityBrickBufferR.Load( int4( brickCoords + localOffset, 0 ) ).xyz;
            bColor.a = dot( opacity, 1.0f.xxx ) * 0.33333f;

            brick[j] = bColor;
        }

        // fill texels with octanes from child texels
        uint3 texelOffset = octaneOffset[i];
        [unroll]
        for ( j = 0; j < CHILDS_COUNT; j++ )
        {
            uint3 texelCoords = octaneOffset[j];
            ContributeOctaneToTexels( texels, brick, texelCoords, texelOffset, true );
        }
    }
    
    // takes childs weights into account
    ApplyWeightsToTexels( texels );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// for each voxels from higher level calculate childs average
void GatherValuesFromLowLevelVS( uint nodeOffset: SV_VertexID, uniform bool isIrradiance )
{
    // get octree and brick coords
    uint levelOffset = LevelOffset( currentOctreeLevel );
    uint currentNodeID = levelOffset + nodeOffset;
    uint currentIndex = IDToIndex( currentNodeID );
    
    // take only allocated nodes into account
    uint nodeFlag = octreeR[GetFlagC( currentIndex )];
    if ( nodeFlag == NODE_UNDEFINED )
        return;

    if ( isIrradiance && ( nodeFlag & NODE_LIT ) == 0x0 )
        return;

    // init texels array
    float4 texels[TEXELS_COUNT] = { 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx,
                                    0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx,
                                    0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx };

    // calculate childs contribution to texels
    // neighboor childs contribution will be taken into account in the average passes
    if ( isIrradiance )
        ContributeIrradianceChildsToParent( texels, currentIndex );
    else
        ContributeOpacityChildsToParent( texels, currentIndex );

    // store texels into a brick buffer
    uint3 brickCoords = NodeIDToTextureCoords( currentNodeID );

    [unroll]
    for ( uint i = 0; i < TEXELS_COUNT; i++ )
    {
        uint3 localOffset = IndexToBrickCoords( i );
        brickBufferRW[brickCoords + localOffset] = PackFloat4ToUint( texels[i] );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DebugVOut DebugBuffersVS( uint nodeOffset: SV_VertexID )
{
    DebugVOut vout;
    vout.nodeOffset = nodeOffset;

    return vout;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// very stupid and ugly solution but ok for debug purpose
float3 GetNodePosition( uint nodeIndex, uint nodeLevel )
{
    uint2 nodeCoords = IndexToCoords( nodeIndex );
    uint3 octreePosition = 0;
    uint childByteOffset = nodeIndex;
    
    [unroll(MAX_OCTREE_HEIGHT)]
    for ( uint level = nodeLevel; level > 0; level-- )
    {
        uint parentNodeByteOffset = GetParentR( nodeCoords );
        nodeCoords = IndexToCoords( parentNodeByteOffset );
        
        uint relativeChildIndex = -1;
        
        [unroll]
        for ( uint childIndex = 0; childIndex < CHILDS_COUNT; childIndex++ )
        {
            if ( octreeR[uint2( nodeCoords.x + childIndex, nodeCoords.y )] == childByteOffset )
            {
                relativeChildIndex = childIndex;
                break;
            }
        }

        childByteOffset = parentNodeByteOffset;

        uint nodeWidth = octreeResolution >> level;
        uint3 mask = uint3( ( relativeChildIndex & 1 ) == 1, ( relativeChildIndex & 2 ) == 2, ( relativeChildIndex & 4 ) == 4 );

        octreePosition += nodeWidth * mask;
    }
    
    float3 worldPosition = float3( octreePosition ) / octreeResolution * ( maxBB - minBB ) + minBB;
    return worldPosition;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
[maxvertexcount(112)] // 8 cubes * 14 verticies
void DebugBuffersGS( point DebugVOut gin[1], inout TriangleStream<DebugGOut> triStream, uniform bool showVoxels )
{
    // find size of cube based on octree level
    // expand node to (8-27) cubes with vertex color from octree / brick buffer
    // translate from world space to NDC space
    // output position / color

    if ( showVoxels )
    {
        uint levelOffset = LevelOffset( octreeHeight - 1 );

        [unroll]
        for ( uint childIndex = 0; childIndex < CHILDS_COUNT; childIndex++ )
        {
            uint nodeID = levelOffset + gin[0].nodeOffset;
            uint nodeIndex = IDToIndex( nodeID );
            uint2 coords = IndexToCoords( nodeIndex + childIndex );
            uint voxelID = octreeR[coords];
            if ( voxelID != NODE_UNDEFINED )
            {
                DebugGOut gout[8];
                DebugGOut vin;
                
                Voxel v = voxelArrayR[voxelID];
                vin.posH = float4( OctreePosToWorldPos( v.position ), 1.0f );
                vin.wPosOrCol = UnpackUintToFloat4( v.color );
                
                float width = ( ( maxBB - minBB ) / octreeResolution ).x;

                gout[0].posH = vin.posH + float4(  0.0f,  0.0f,  0.0f, 0.0f );
                gout[1].posH = vin.posH + float4( width,  0.0f,  0.0f, 0.0f );
                gout[2].posH = vin.posH + float4(  0.0f, width,  0.0f, 0.0f );
                gout[3].posH = vin.posH + float4( width, width,  0.0f, 0.0f );
                gout[4].posH = vin.posH + float4(  0.0f,  0.0f, width, 0.0f );
                gout[5].posH = vin.posH + float4( width,  0.0f, width, 0.0f );
                gout[6].posH = vin.posH + float4( width, width, width, 0.0f );
                gout[7].posH = vin.posH + float4(  0.0f, width, width, 0.0f );

                [unroll]
                for ( uint i = 0; i < CHILDS_COUNT; i++ )
                {
                    gout[i].posH =  mul( float4( gout[i].posH.xyz, 1.0f ), gWorldViewProj );
                    gout[i].wPosOrCol = vin.wPosOrCol;
                }

                const uint cubeIndices[14] = { 2, 3, 7, 6, 5, 3, 1, 2, 0, 7, 4, 5, 0, 1 };

                [unroll]
                for ( i = 0; i < 14; i++ )
                {
                     triStream.Append( gout[cubeIndices[i]] );
                }

                triStream.RestartStrip();
            }
        }
    }
    else
    {
        uint octreeLevel = debugOctreeLevel;
        uint levelOffset = LevelOffset( octreeLevel );

        uint nodeID = levelOffset + gin[0].nodeOffset;
        uint nodeIndex = IDToIndex( nodeID );

        if ( octreeR[GetFlagC( nodeIndex )] != NODE_UNDEFINED )
        {
            // allocated nodes?
            float3 wPos = GetNodePosition( nodeIndex, octreeLevel );

            DebugGOut gout[8];
            DebugGOut vin;

            vin.posH = float4( wPos, 1.0f );

            uint brickSize = octreeResolution >> ( octreeHeight - octreeLevel );
            float width = ( maxBB.x - minBB.x - EPS ) / brickSize * 0.9;

            gout[0].wPosOrCol = vin.posH + float4(  0.0f,  0.0f,  0.0f, 0.0f );
            gout[1].wPosOrCol = vin.posH + float4( width,  0.0f,  0.0f, 0.0f );
            gout[2].wPosOrCol = vin.posH + float4(  0.0f, width,  0.0f, 0.0f );
            gout[3].wPosOrCol = vin.posH + float4( width, width,  0.0f, 0.0f );
            gout[4].wPosOrCol = vin.posH + float4(  0.0f,  0.0f, width, 0.0f );
            gout[5].wPosOrCol = vin.posH + float4( width,  0.0f, width, 0.0f );
            gout[6].wPosOrCol = vin.posH + float4( width, width, width, 0.0f );
            gout[7].wPosOrCol = vin.posH + float4(  0.0f, width, width, 0.0f );

            [unroll]
            for ( uint i = 0; i < CHILDS_COUNT; i++ )
            {
                gout[i].posH =  mul( float4( gout[i].wPosOrCol.xyz, 1.0f ), gWorldViewProj );
            }

            const uint cubeIndices[14] = { 2, 3, 7, 6, 5, 3, 1, 2, 0, 7, 4, 5, 0, 1 };

            [unroll]
            for ( i = 0; i < 14; i++ )
            {
                triStream.Append( gout[cubeIndices[i]] );
            }
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float3 WorldPosToBrickCoords( in uint nodeID, in int3 nodeStartCoords, in uint octreeLevel, in float3 wPos, out float3 coordsOffset )
{
    float3 coordsStart = float3( NodeIDToTextureCoords( nodeID ) ) / brickBufferSize;
    
    // convert nodeStartCoords to worldPos and calculate relative offset inside brick
    float nodeWidth = ( ( maxBB - minBB ) / ( octreeResolution >> ( octreeHeight - octreeLevel ) ) ).x;
    float3 startWPos = float3( nodeStartCoords ) / octreeResolution * ( maxBB - minBB ) + minBB;
    coordsOffset = ( wPos - startWPos ) / nodeWidth * SAMPLING_AREA_SIZE;

    return coordsStart + coordsOffset + SAMPLING_OFFSET;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
DebugRT DebugBuffersPS( DebugGOut pin, uniform bool showVoxels ) : SV_Target
{
    DebugRT color;

    if ( showVoxels )
    {
        color.Color = pin.wPosOrCol;
    }
    else
    {
        float3 outputColor = float3( 1.0f, 0.0f, 0.0f );

        uint octreePos = WorlPosToOctreePos( pin.wPosOrCol.xyz );
        int3 nodeStartCoords = 0;
        uint nodeIndex = 0;
        
        if ( TraverseOctreeR( octreePos, debugOctreeLevel, nodeIndex, nodeStartCoords ) )
        {
            uint nodeID = IndexToID( nodeIndex );
            
            float3 coordsOffset = 0.0f.xxx; // tmp value
            float3 brickCoords = WorldPosToBrickCoords( nodeID, nodeStartCoords, debugOctreeLevel, pin.wPosOrCol.xyz, coordsOffset );
            outputColor = brickBufferR.Sample( linearSampler, brickCoords ).rgb;
        }
        
        color.Color = float4( outputColor, 1.0f );
    }

    return color;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float3 GetWorldPosFromDepth( int3 screenCoords )
{
    float2 uvCoords = float2( screenCoords.xy ) / shadowMapResolution;
    float depth = shadowMap.Load( screenCoords ).r;

    float4 projCoords = float4( float2( uvCoords.x, 1.0f - uvCoords.y ) * 2.0f - 1.0f, depth, 1.0f);

    return GetWorldPos( projCoords, gShadowInverseProj, gShadowInverseView ).xyz;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FullScreenQuadOut FullScreenQuadOutVS( Vertex_3F3F3F2F vin )
{
    // process vs quad (like g-buffer combine pass)
    FullScreenQuadOut vout;
    
    vout.PosH = mul( float4( vin.Pos, 1.0f ), gWorldViewProj );
    vout.UV = vin.UV;

    return vout;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float CircularSegmentArea( in float d )
{
    d = saturate( d );
    return saturate( acos( d ) - d * sqrt( 1.0f - d * d ) );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float ProjectHalfsphereOnCircleSide( in float2 nProj )
{
    // project half sphere on a unit cube side
    // we calculate how much energy is reflected in a specific direction (NX, PX, NY, PY, NZ, PZ) from lit plane
    // sphere projection is a circle, so we calculate a percentage of circular segment area by project a plane normal
    //  (because we project not a half sphere but distribution from lit plane)

    // find intersection point with nProj and x = 1
    // assume nProj.x is positive

    float y = 0.0f;
    if ( nProj.x > EPS )
        y = nProj.y / nProj.x;
    else
        y = sign( nProj.y );

    float d = clamp( y, -1.0f, 1.0f );
    float result = 0.0f;
    if ( d > 0.0f )
    {
        float segA = CircularSegmentArea( d );
        result = ( PI * 0.5 + segA ) * PI_INV;
    }
    else
    {
        d = -d;
        float segA = CircularSegmentArea( d );
        result = segA * PI_INV;
    }

    return result * 0.33333f; // one full side is 1/3 of half sphere projection
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
emptyRT ProcessingShadowMapPS( FullScreenQuadOut pin ) : SV_Target
{
    emptyRT output;

    // get world position from shadowmap
    int3 screenCoords = int3( pin.PosH.xy, 0 );
    float3 worldPos = GetWorldPosFromDepth( screenCoords );

    // discard everything outside the octree
    if ( any( worldPos < minBB ) || any( worldPos >  maxBB ) )
        discard;

    uint octreePos = WorlPosToOctreePos( worldPos );

    // get left, top, left-top value and discard if it's octreePos is equal to current (to prevent performance degrade)
    float3 tmpWorldPos;
    uint tmpOctreePos = 0;
    if ( screenCoords.x > 0 )
    {
        tmpWorldPos = GetWorldPosFromDepth( screenCoords + int3( -1, 0, 0 ) );
        tmpOctreePos = WorlPosToOctreePos( tmpWorldPos );
        if ( tmpOctreePos == octreePos )
            discard;

        if ( screenCoords.y < 0 )
        {
            tmpWorldPos = GetWorldPosFromDepth( screenCoords + int3( -1, -1, 0 ) );
            tmpOctreePos = WorlPosToOctreePos( tmpWorldPos );
            if ( tmpOctreePos == octreePos )
                discard;
        }
    }
    if ( screenCoords.y < 0 )
    {
        tmpWorldPos = GetWorldPosFromDepth( screenCoords + int3( 0, -1, 0 ) );
        tmpOctreePos = WorlPosToOctreePos( tmpWorldPos );
        if ( tmpOctreePos == octreePos )
            discard;
    }

    // find voxel, calculate and write irradiance value to brick buffer if success
    uint voxelIndex = 0;
    uint voxelParentIndex = 0;
    uint3 voxelMask = 0;

    if ( PhotonTraverseOctree( octreePos, voxelIndex, voxelParentIndex, voxelMask ) )
    {
        if ( voxelIndex != NODE_UNDEFINED )
        {
            Voxel v = voxelArrayR[voxelIndex];
            float4 vCol = UnpackUintToFloat4( v.color );
            float3 vNrm = UnpackUintToFloat3( v.normal ) * 2.0f - 1.0f;
            float4 outputEnergy = float4( saturate( dot( vNrm, -lDirection.xyz ) ) * lColorRadius.rgb * vCol.rgb, 1.0f ); // assume there is no attenuation

            // write to brick corners (see ConstructOpacityVS scheme)
            uint3 brickCoords = NodeIDToTextureCoords( IndexToID( voxelParentIndex ) );
            uint3 localOffset = voxelMask * 2;
            irradianceBrickBufferRW[brickCoords + localOffset] = PackFloat4ToUint( outputEnergy );
        }
    }

    return output;
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// reset lit nodes flags
void ResetOctreeFlagsVS( uint nodeOffset: SV_VertexID )
{
    // get node id with local node id (nodeOffset) and current level nodes offset in the octree
    uint nodeID = nodeOffset;
    uint nodeIndex = IDToIndex( nodeID );

    uint2 flagCoords = GetFlagC( nodeIndex );
    uint flag = octreeRW[flagCoords];
    
    // if node is lit
    if ( flag != NODE_UNDEFINED && flag != NODE_ALLOCATED )
        octreeRW[flagCoords] = NODE_ALLOCATED;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

//
//  ConstructBrickBuffer
//
//  Write brick values for the last level of octree.
//  Octree is used as scheme for bricks addressing.
//
//  for (level = octreeHeight; level >= 0; level--)
//  {
//      Average bricks values from the previous level (X, Y, Z).
//      Gather values from previous level and write to current level .
//  }
//

technique11 GenerateBrickBuffer
{
    // use indirect buffer to place every leave to opacity brick buffer
    
    pass ConstructOpacity
    {
        SetVertexShader( CompileShader( vs_5_0, ConstructOpacityVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( NULL );
    }

    pass AverageAlongAxisX
    {
        SetVertexShader( CompileShader( vs_5_0, AverageAlongAxisVS( 0, false ) ) );
        SetGeometryShader( NULL );
        SetPixelShader( NULL );
    }

    pass AverageAlongAxisY
    {
        SetVertexShader( CompileShader( vs_5_0, AverageAlongAxisVS( 1, false ) ) );
        SetGeometryShader( NULL );
        SetPixelShader( NULL );
    }

    pass AverageAlongAxisZ
    {
        SetVertexShader( CompileShader( vs_5_0, AverageAlongAxisVS( 2, false ) ) );
        SetGeometryShader( NULL );
        SetPixelShader( NULL );
    }

    pass GatherOpacityFromLowLevel
    {
        SetVertexShader( CompileShader( vs_5_0, GatherValuesFromLowLevelVS( false ) ) );
        SetGeometryShader( NULL );
        SetPixelShader( NULL );
    }

    // use indirect buffer to draw octree
    pass DebugBricks
    {
        SetVertexShader( CompileShader( vs_5_0, DebugBuffersVS() ) );
        SetGeometryShader( CompileShader( gs_5_0, DebugBuffersGS( false ) ) );
        SetPixelShader( CompileShader( ps_5_0, DebugBuffersPS( false ) ) );
    }
    
    pass DebugVoxels
    {
        SetVertexShader( CompileShader( vs_5_0, DebugBuffersVS() ) );
        SetGeometryShader( CompileShader( gs_5_0, DebugBuffersGS( true ) ) );
        SetPixelShader( CompileShader( ps_5_0, DebugBuffersPS( true ) ) );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
technique11 GenerateLightBriks
{
    pass ResetOctreeFlags
    {
        SetVertexShader( CompileShader( vs_5_0, ResetOctreeFlagsVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( NULL );
    }

    pass ProcessingShadowMap
    {
        SetVertexShader( CompileShader( vs_5_0, FullScreenQuadOutVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, ProcessingShadowMapPS() ) );
    }

    pass AverageLitNodeValues
    {
        SetVertexShader( CompileShader( vs_5_0, AverageLitNodeValuesVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( NULL );
    }

    pass AverageAlongAxisX
    {
        SetVertexShader( CompileShader( vs_5_0, AverageAlongAxisVS( 0, true ) ) );
        SetGeometryShader( NULL );
        SetPixelShader( NULL );
    }

    pass AverageAlongAxisY
    {
        SetVertexShader( CompileShader( vs_5_0, AverageAlongAxisVS( 1, true ) ) );
        SetGeometryShader( NULL );
        SetPixelShader( NULL );
    }

    pass AverageAlongAxisZ
    {
        SetVertexShader( CompileShader( vs_5_0, AverageAlongAxisVS( 2, true ) ) );
        SetGeometryShader( NULL );
        SetPixelShader( NULL );
    }

    pass GatherValuesFromLowLevel
    {
        SetVertexShader( CompileShader( vs_5_0, GatherValuesFromLowLevelVS( true ) ) );
        SetGeometryShader( NULL );
        SetPixelShader( NULL );
    }
}
