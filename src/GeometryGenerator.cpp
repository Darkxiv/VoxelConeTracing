#include <GeometryGenerator.h>
#include <GlobalUtils.h>
#include <algorithm>

using namespace DirectX;

float AngleFromXY( float x, float y );

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GeometryGenerator::CalculateTBN( GGMeshData &data, const std::vector<std::vector<std::pair<int, int> > > &vertexCoherence )
{
    // TODO there is an error in the tangent calculation (see sponza columns)

    int vsize = vertexCoherence.size();
    for ( int iv0 = 0; iv0 < vsize; iv0++ )
    {
        auto &vcBegin = vertexCoherence[iv0].cbegin();
        auto &vcEnd = vertexCoherence[iv0].cend( );
        auto &curVertex = data.verticies[iv0];
        XMVECTOR binormal( XMVectorZero( ) ), normal( XMVectorZero( ) );

        for ( auto &vPair = vcBegin; vPair != vcEnd; ++vPair )
        {
            int iv1 = vPair->first, iv2 = vPair->second;
            binormal += XMLoadFloat3( &CalculateBinormal( curVertex, data.verticies[iv1], data.verticies[iv2] ) );
            normal += XMLoadFloat3( &curVertex.normal );
        }
        binormal = XMVector3Normalize( binormal );
        normal = XMVector3Normalize( normal );
        XMStoreFloat3( &curVertex.binormal, binormal );
        XMStoreFloat3( &curVertex.normal, normal );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GeometryGenerator::GeneratePlane( float width, float depth, uint32_t n, uint32_t m, GGMeshData &meshData )
{
    ASSERT( n > 1 && m > 1 );
    uint32_t vertexCount = n * m;
    uint32_t faceCount = 2 * (n - 1) * (m - 1);

    meshData.verticies.resize( vertexCount );
    meshData.indicies.resize( faceCount * 3 );

    float dx = static_cast<float>( width ) / (n - 1);
    float dz = static_cast<float>( depth ) / (m - 1);

    float du = 1.0f / ( n - 1 );
    float dv = 1.0f / ( m - 1 );

    for ( uint32_t i = 0; i < n; i++ )
    {
        float x = -width * 0.5f + i * dx;

        for ( uint32_t j = 0; j < m; j++ )
        {
            float z = -depth * 0.5f + j * dz;

            GGVertex &v = meshData.verticies[i * m + j];
            v.position = XMFLOAT3( x, 0.0f, z );
            v.normal = XMFLOAT3( 0.0f, 1.0f, 0.0f );
            v.binormal = XMFLOAT3( 0.0f, 0.0f, 1.0f );
            v.UVW = XMFLOAT3( du * i, dv * j, 0.0f );
        }
    }

    int c = 0;
    for ( uint32_t i = 0; i < n - 1; i++ )
    {
        for ( uint32_t j = 0; j < m - 1; j++ )
        {
            meshData.indicies[c] = i * m + j;
            meshData.indicies[c + 1] = i * m + j + 1;
            meshData.indicies[c + 2] = (i + 1) * m + j;
            meshData.indicies[c + 3] = (i + 1) * m + j;
            meshData.indicies[c + 4] = i * m + j + 1;
            meshData.indicies[c + 5] = ( i + 1 ) * m + j + 1;
            c += 6; // next quad
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GeometryGenerator::GenerateCube( GGMeshData &meshData )
{
    meshData.verticies.resize( 8 );

    std::vector<GGVertex> &v = meshData.verticies;
    v[0].position = XMFLOAT3( -1.0f, -1.0f, -1.0f );
    v[1].position = XMFLOAT3( -1.0f, 1.0f, -1.0f );
    v[2].position = XMFLOAT3( 1.0f, 1.0f, -1.0f );
    v[3].position = XMFLOAT3( 1.0f, -1.0f, -1.0f );
    v[4].position = XMFLOAT3( -1.0f, -1.0f, 1.0f );
    v[5].position = XMFLOAT3( -1.0f, 1.0f, 1.0f );
    v[6].position = XMFLOAT3( 1.0f, 1.0f, 1.0f );
    v[7].position = XMFLOAT3( 1.0f, -1.0f, 1.0f );

    // TODO normal, tangent, uvw

    meshData.indicies = {
        // front face
        0, 1, 2,
        0, 2, 3,
        //back face
        4, 6, 5,
        4, 7, 6,
        // left face
        4, 5, 1,
        4, 1, 0,
        // right face
        3, 2, 6,
        3, 6, 7,
        // top face
        1, 5, 6,
        1, 6, 2,
        // bottom face
        4, 0, 3,
        4, 3, 7
    };
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GeometryGenerator::GenerateCylinder( float height, float topRadius, float bottomRadius, uint32_t sliceCount, uint32_t stackCount, GGMeshData &meshData )
{
    ASSERT( !( topRadius < 0.0f || bottomRadius < 0.0f ) &&
        ( topRadius != 0.0f || bottomRadius != 0.0f ) &&
        height > 0.0f && sliceCount >= 3 && stackCount > 0 );

    meshData.verticies.clear( );
    meshData.indicies.clear( );

    float dr = ( topRadius - bottomRadius ) / stackCount;
    float dh = height / stackCount;
    float da = XM_2PI / sliceCount;

    // generate middle
    //       /\ v
    // (0, 0) +---+---+ (1, 0)
    //        |   |   |
    //        +---+---+
    //        |   |   |
    // (0, 1) +---+---+ (1, 1) --> u
    uint32_t ringCount = stackCount + 1;
    for ( uint32_t i = 0; i < ringCount; i++ )
    {
        float r = bottomRadius + dr * i;
        float y = -height * 0.5f + dh * i;

        for ( uint32_t j = 0; j <= sliceCount; j++ )
        {
            GGVertex v;

            float alpha = da * j;
            float c = cosf( alpha );
            float s = sinf( alpha );

            v.position = XMFLOAT3( r * c, y, r * s );

            // note: possibly incorrect
            v.UVW.x = static_cast<float>( j ) / sliceCount;
            v.UVW.y = 1.0f - static_cast<float>( i ) / stackCount;

            v.binormal = XMFLOAT3( -s, 0.0f, c ); // TODO this is tangent calculation and should be replaced by binormal calculatuion
            XMFLOAT3 bitangent( dr * c, -height, dr * s );

            XMVECTOR T = XMLoadFloat3( &v.binormal );
            XMVECTOR B = XMLoadFloat3( &bitangent );
            XMVECTOR N = XMVector3Normalize( XMVector3Cross( T, B ) );
            XMStoreFloat3( &v.normal, N );

            meshData.verticies.push_back( v );
        }
    }

    uint32_t ringVertexCount = sliceCount + 1;
    for ( uint32_t i = 0; i < stackCount; ++i )
    {
        for ( uint32_t j = 0; j < sliceCount; ++j )
        {
            meshData.indicies.push_back( i * ringVertexCount + j );
            meshData.indicies.push_back( ( i + 1 ) * ringVertexCount + j );
            meshData.indicies.push_back( ( i + 1 ) * ringVertexCount + j + 1 );
            meshData.indicies.push_back( i * ringVertexCount + j );
            meshData.indicies.push_back( ( i + 1 ) * ringVertexCount + j + 1 );
            meshData.indicies.push_back( i * ringVertexCount + j + 1 );
        }
    }

    // generate cap
    // note: possible to change uv to one plane
    // o_o
    // |_|
    GenerateCap( sliceCount, 0.5f * height, true, topRadius, meshData );
    GenerateCap( sliceCount, -0.5f * height, false, bottomRadius, meshData );

}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GeometryGenerator::GenerateGeoSphere( float radius, uint32_t subDivNum, GGMeshData &meshData )
{
    const float pX = 0.525731f;
    const float pZ = 0.850651f;

    XMFLOAT3 pos[12] =
    {
        XMFLOAT3( -pX, 0.0f, pZ ), XMFLOAT3( pX, 0.0f, pZ ),
        XMFLOAT3( -pX, 0.0f, -pZ ), XMFLOAT3( pX, 0.0f, -pZ ),
        XMFLOAT3( 0.0f, pZ, pX ), XMFLOAT3( 0.0f, pZ, -pX ),
        XMFLOAT3( 0.0f, -pZ, pX ), XMFLOAT3( 0.0f, -pZ, -pX ),
        XMFLOAT3( pZ, pX, 0.0f ), XMFLOAT3( -pZ, pX, 0.0f ),
        XMFLOAT3( pZ, -pX, 0.0f ), XMFLOAT3( -pZ, -pX, 0.0f ),
    };

    meshData.verticies.resize( 12 );
    for ( size_t i = 0; i < 12; ++i )
        meshData.verticies[i].position = pos[i];

    meshData.indicies =
    {
        1, 4, 0, 4, 9, 0, 4, 5, 9, 8, 5, 4, 1, 8, 4,
        1, 10, 8, 10, 3, 8, 8, 3, 5, 3, 2, 5, 3, 7, 2,
        3, 10, 7, 10, 6, 7, 6, 11, 7, 6, 0, 11, 6, 1, 0,
        10, 1, 6, 11, 0, 9, 2, 11, 9, 5, 2, 9, 11, 2, 7
    };

    for ( size_t i = 0; i < subDivNum; ++i )
        Subdivide( meshData );

    for ( size_t i = 0; i < meshData.verticies.size( ); ++i )
    {
        GGVertex &v = meshData.verticies[i];

        XMVECTOR n = XMVector3Normalize( XMLoadFloat3( &v.position ) );
        XMVECTOR p = radius * n;
        XMStoreFloat3( &meshData.verticies[i].position, p );
        XMStoreFloat3( &meshData.verticies[i].normal, n );

        float theta = AngleFromXY( v.position.x, v.position.z );
        float phi = acosf( v.normal.y );

        v.UVW.x = theta / XM_2PI;
        v.UVW.y = phi / XM_PI;

        // TODO this is tangent calculation and should be replaced by binormal calculatuion
        v.binormal = XMFLOAT3( -radius * sinf( phi )* sinf( theta ),
            0.0f,
            radius * sinf( phi ) * cos( theta ) );

        XMVECTOR T = XMLoadFloat3( &v.binormal );
        XMStoreFloat3( &v.binormal, XMVector3Normalize( T ) );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
XMFLOAT3 GeometryGenerator::CalculateBinormal( const GGVertex &v0, const GGVertex &v1, const GGVertex &v2 )
{
    XMFLOAT3 e0 = XMFLOAT3( v1.position.x - v0.position.x, v1.position.y - v0.position.y, v1.position.z - v0.position.z );
    XMFLOAT3 e1 = XMFLOAT3( v2.position.x - v0.position.x, v2.position.y - v0.position.y, v2.position.z - v0.position.z );

    float du0 = v1.UVW.x - v0.UVW.x;
    float du1 = v2.UVW.x - v0.UVW.x;
    float dv0 = v1.UVW.y - v0.UVW.y;
    float dv1 = v2.UVW.y - v0.UVW.y;
    
    float determ = 1.0f / ( du0 * dv1 - du1 * dv0 );
    return XMFLOAT3( ( -du1 * e0.x + du0 * e1.x ) * determ,
        ( -du1 * e0.y + du0 * e1.y ) * determ,
        ( -du1 * e0.z + du0 * e1.z ) * determ );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GeometryGenerator::GenerateCap( uint32_t sliceCount, float y, bool topCap, float radius, GGMeshData &meshData )
{
    uint32_t baseIndex = meshData.verticies.size( );

    float da = XM_2PI / sliceCount;
    for ( uint32_t i = 0; i <= sliceCount; ++i )
    {
        float alpha = da * i;
        float c = cosf( alpha );
        float s = sinf( alpha );

        meshData.verticies.push_back( GGVertex( radius * c, y, radius * s,
                                            0.0f, topCap ? 1.0f : -1.0f, 0.0f,
                                            1.0f, 0.0f, 0.0f,
                                            c * 0.5f + 0.5f, s * 0.5f + 0.5f, 0.0f ) );
    }

    meshData.verticies.push_back( GGVertex( 0.0f, y, 0.0f,
                                        0.0f, topCap ? 1.0f : -1.0f, 0.0f,
                                        1.0f, 0.0f, 0.0f,
                                        0.5f, 0.5f, 0.0 ) );
    uint32_t centerIndex = meshData.verticies.size( ) - 1;

    for ( uint32_t i = 0; i < sliceCount; ++i )
    {
        meshData.indicies.push_back( centerIndex );
        if ( topCap )
        {
            meshData.indicies.push_back( baseIndex + i + 1 );
            meshData.indicies.push_back( baseIndex + i );
        }
        else
        {
            meshData.indicies.push_back( baseIndex + i );
            meshData.indicies.push_back( baseIndex + i + 1 );
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GeometryGenerator::Subdivide( GGMeshData& meshData )
{
    // Save a copy of the input geometry.
    GGMeshData inputCopy = meshData;

    meshData.verticies.resize( 0 );
    meshData.indicies.resize( 0 );

    //       v1
    //       *
    //      / \
    //     /   \
    //  m0*-----*m1
    //   / \   / \
    //  /   \ /   \
    // *-----*-----*
    // v0    m2     v2

    size_t numTris = inputCopy.indicies.size() / 3;
    for ( size_t i = 0; i < numTris; ++i )
    {
        GGVertex v0 = inputCopy.verticies[inputCopy.indicies[i * 3 + 0]];
        GGVertex v1 = inputCopy.verticies[inputCopy.indicies[i * 3 + 1]];
        GGVertex v2 = inputCopy.verticies[inputCopy.indicies[i * 3 + 2]];

        //
        // Generate the midpoints.
        //

        GGVertex m0, m1, m2;

        // For subdivision, we just care about the position component.  We derive the other
        // vertex components in CreateGeosphere.

        m0.position = XMFLOAT3(
            0.5f*( v0.position.x + v1.position.x ),
            0.5f*( v0.position.y + v1.position.y ),
            0.5f*( v0.position.z + v1.position.z ) );

        m1.position = XMFLOAT3(
            0.5f*( v1.position.x + v2.position.x ),
            0.5f*( v1.position.y + v2.position.y ),
            0.5f*( v1.position.z + v2.position.z ) );

        m2.position = XMFLOAT3(
            0.5f*( v0.position.x + v2.position.x ),
            0.5f*( v0.position.y + v2.position.y ),
            0.5f*( v0.position.z + v2.position.z ) );

        //
        // Add new geometry.
        //

        meshData.verticies.push_back( v0 ); // 0
        meshData.verticies.push_back( v1 ); // 1
        meshData.verticies.push_back( v2 ); // 2
        meshData.verticies.push_back( m0 ); // 3
        meshData.verticies.push_back( m1 ); // 4
        meshData.verticies.push_back( m2 ); // 5

        meshData.indicies.push_back( i * 6 + 0 );
        meshData.indicies.push_back( i * 6 + 3 );
        meshData.indicies.push_back( i * 6 + 5 );

        meshData.indicies.push_back( i * 6 + 3 );
        meshData.indicies.push_back( i * 6 + 4 );
        meshData.indicies.push_back( i * 6 + 5 );

        meshData.indicies.push_back( i * 6 + 5 );
        meshData.indicies.push_back( i * 6 + 4 );
        meshData.indicies.push_back( i * 6 + 2 );

        meshData.indicies.push_back( i * 6 + 3 );
        meshData.indicies.push_back( i * 6 + 1 );
        meshData.indicies.push_back( i * 6 + 4 );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float AngleFromXY( float x, float y )
{
    // note: not complete
    float theta = 0.0f;

    // Quadrant I or IV
    if ( x >= 0.0f )
    {
        // If x = 0, then atanf(y/x) = +pi/2 if y > 0
        //                atanf(y/x) = -pi/2 if y < 0
        theta = atanf( y / x ); // in [-pi/2, +pi/2]

        if ( theta < 0.0f )
            theta += 2.0f * XM_PI; // in [0, 2*pi).
    }

    // Quadrant II or III
    else
        theta = atanf( y / x ) + XM_PI; // in [0, 2*pi).

    return theta;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
