#include "utils.fx"

float4x4 gWorldViewProj;

struct ShadowMapVertexOut
{
    float4 PosH : SV_POSITION;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ShadowMapVertexOut ShadowMapVS( Vertex_3F3F3F2F vin )
{
    // cast shadowmap
    ShadowMapVertexOut vout;
    vout.PosH = mul( float4( vin.Pos, 1.0f ), gWorldViewProj );

    return vout;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
technique11 ShadowMap
{
    pass ShadowMapPass
    {
        SetVertexShader( CompileShader( vs_4_0, ShadowMapVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( NULL );
    }
}
