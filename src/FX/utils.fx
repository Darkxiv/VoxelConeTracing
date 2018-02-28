
// FX specific header file
static const float PI = 3.14159265f;
static const float PI_INV = 1.0f / 3.14159265f;

// IA layout
struct Vertex_3F3F3F2F
{
    float3 Pos      : POSITION;
    float3 Normal   : NORMAL;
    float3 Binormal : BINORMAL;
    float2 UV       : TEXCOORD;
    //float4 Color  : COLOR;
};

struct FullVertexOut
{
    float4 PosH     : SV_POSITION;
    float3 Position : POSITION;
    float3 Normal   : NORMAL;
    float3 Binormal : BINORMAL;
    float2 UV       : TEXCOORD;
};

SamplerState linearSampler
{
    Filter = MIN_MAG_MIP_LINEAR;
    AddressU = WRAP;
    AddressV = WRAP;
    AddressW = WRAP;
};

SamplerState pointSamplerBorder
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = BORDER;
    AddressV = BORDER;
    AddressW = BORDER;
    BorderColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
};

SamplerState pointSamplerClamp
{
    Filter = MIN_MAG_MIP_POINT;
    AddressU = CLAMP;
    AddressV = CLAMP;
    AddressW = CLAMP;
};

SamplerComparisonState shadowSampler
{
    Filter = COMPARISON_MIN_MAG_LINEAR_MIP_POINT;

    AddressU = BORDER;
    AddressV = BORDER;
    AddressW = BORDER;
    BorderColor = float4(0.0f, 0.0f, 0.0f, 0.0f);
    
    ComparisonFunc = LESS_EQUAL;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float4 GetWorldPos( in float4 projCoords, in float4x4 inverseProj, in float4x4 inverseView )
{
    float4 viewCoords = mul( projCoords, inverseProj );
    viewCoords /= viewCoords.w;
    return mul( viewCoords, inverseView );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float3 CalculateNormal( float3 packedN, float3 worldN, float3 worldB )
{
    //  uncompress each component from [0, 1]  to  [-1, 1].
    float3 unpackedN = normalize( 2.0f * packedN - 1.0f );
    
    //  build orthonormal  basis.
    float3  N = worldN;
    float3  B = normalize (worldB - dot(worldB, N ) * N );
    float3  T = cross(B, N);
    float3x3 TBN = float3x3(T, B, N);

    // trans form from tangent space to world space.
    float3 bumpedNormalW = mul(unpackedN, TBN);
    return bumpedNormalW;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint PackFloat3ToUint( float3 color )
{
    float3 c = saturate( color );
    return ( uint( c.r * 255 ) )      |
           ( uint( c.g * 255 ) << 8 ) |
           ( uint( c.b * 255 ) << 16 );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float3 UnpackUintToFloat3( uint value )
{
    return float3( ( ( value )       & 0xff ) * 0.003922,
                   ( ( value >> 8  ) & 0xff ) * 0.003922,
                   ( ( value >> 16 ) & 0xff ) * 0.003922 );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint PackFloat4ToUint( float4 color )
{
    float4 c = saturate( color );
    return ( uint( c.r * 255 ) ) |
           ( uint( c.g * 255 ) << 8  ) |
           ( uint( c.b * 255 ) << 16 ) |
           ( uint( c.a * 255 ) << 24 );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float4 UnpackUintToFloat4( uint value )
{
    return float4( ( ( value )       & 0xff ) * 0.003922,
                   ( ( value >> 8 )  & 0xff ) * 0.003922,
                   ( ( value >> 16 ) & 0xff ) * 0.003922,
                   ( ( value >> 24 ) & 0xff ) * 0.003922 );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint PackUint3ToUint( uint3 value )
{
    return ( ( value.r & 0x3ff ) )       |
           ( ( value.g & 0x3ff ) << 10 ) |
             ( value.b & 0x3ff ) << 20;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint3 UnpackUintToUint3( uint value )
{
    return uint3( ( value ) & 0x3ff, 
                  ( value >> 10 ) & 0x3ff, 
                  ( value >> 20 ) & 0x3ff );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////