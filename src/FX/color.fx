#include "utils.fx"

struct VertexOut
{
    float4 PosH : SV_POSITION;
    float2 UV: TEXCOORD;
};

float4x4 gWorldViewProj;

Texture2D defaultTexture;
bool showAlpha;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VertexOut VS(Vertex_3F3F3F2F vin)
{
    VertexOut vout;
    
    vout.PosH = mul( float4( vin.Pos, 1.0f ), gWorldViewProj );
    vout.UV = vin.UV;

    return vout;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float4 PS(VertexOut pin) : SV_Target
{
    float4 sampleColor = defaultTexture.Sample( linearSampler, pin.UV );
    if ( showAlpha )
        sampleColor.rgb = sampleColor.aaa;

    return sampleColor;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

technique11 ColorTech
{
    pass SimplePass
    {
        SetVertexShader( CompileShader( vs_4_0, VS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, PS() ) );
    }
}
