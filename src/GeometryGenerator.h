#ifndef __GEOMETRY_GENERATOR_H
#define  __GEOMETRY_GENERATOR_H

#include <string>
#include <vector>
#include <DirectXMath.h>

struct GGVertex
{
    GGVertex( ) :
    position( 0.0f, 0.0f, 0.0f ),
    normal( 0.0f, 0.0f, 0.0f ),
    binormal( 0.0f, 0.0f, 0.0f ),
    UVW( 0.0f, 0.0f, 0.0f )
    {}

    GGVertex( float px, float py, float pz,
        float nx, float ny, float nz,
        float tx, float ty, float tz,
        float u, float v, float w ) :
        position( px, py, pz ),
        normal( nx, ny, nz ),
        binormal( tx, ty, tz ),
        UVW( u, v, w )
    {}

    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 normal;
    DirectX::XMFLOAT3 binormal;
    DirectX::XMFLOAT3 UVW;
};

struct GGMeshData
{
    std::vector<GGVertex> verticies;
    std::vector<uint32_t> indicies;
};

class GeometryGenerator
{
public:
    static void CalculateTBN( GGMeshData &data, const std::vector<std::vector<std::pair<int, int> > > &vertexCoherence );

    static void GeneratePlane( float width, float depth, uint32_t wVertices, uint32_t dVerticies, GGMeshData &meshData );
    static void GenerateCube( GGMeshData &meshData );
    static void GenerateCylinder( float height, float topRadius, float bottomRadius, uint32_t sliceCount, uint32_t stackCount, GGMeshData &meshData );
    static void GenerateGeoSphere( float radius, uint32_t subDivNum, GGMeshData &meshData );

private:
    static DirectX::XMFLOAT3 GeometryGenerator::CalculateBinormal( const GGVertex &v0, const GGVertex &v1, const GGVertex &v2 );
    static void GenerateCap( uint32_t sliceCount, float y, bool topCap, float radius, GGMeshData &meshData );
    static void Subdivide( GGMeshData& meshData );
};

#endif