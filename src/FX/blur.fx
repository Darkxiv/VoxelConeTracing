#include "utils.fx"

// This FX contains blur technique from NVIDIA SSAO paper

struct FullScreenQuadOut
{
    float4 PosH : SV_POSITION;
    float2 UV: TEXCOORD;
};

float4x4 gWorldViewProj;
float falloff;
float sharpness;
float radius;
float2 pixelOffset;

Texture2D depthTexture;
Texture2D colorTexture;
Texture2D blurXTexture;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FullScreenQuadOut FullScreenQuadOutVS( Vertex_3F3F3F2F vin )
{
    FullScreenQuadOut vout;
    
    vout.PosH = mul( float4( vin.Pos, 1.0f ), gWorldViewProj );
    vout.UV = vin.UV;

    return vout;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float4 BlurFunction( float2 uv, float r, float centerDepth, inout float weightSum )
{
    float4 col = colorTexture.Sample( pointSamplerClamp, uv );
    float depth = depthTexture.Sample( pointSamplerClamp, uv ).r;

    float ddiff = depth - centerDepth;
    float weight = exp( -r * r * falloff - ddiff * ddiff * sharpness );
    weightSum += weight;

    return weight * col;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float4 BlurX( FullScreenQuadOut pin ) : SV_Target
{
    float4 colSum = 0;
    float weightSum = 0;
    float centerDepth = depthTexture.Sample( pointSamplerClamp, pin.UV ).r;

    for ( float r = -radius; r <= radius; ++r )
    {
        float2 uv = pin.UV.xy + float2( r * pixelOffset.x , 0 );
        colSum += BlurFunction( uv, r, centerDepth, weightSum );
    }

    return colSum / weightSum;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float4 BlurY( FullScreenQuadOut pin ) : SV_Target
{
    float4 colSum = 0;
    float weightSum = 0;
    float centerDepth = depthTexture.Sample( pointSamplerClamp, pin.UV ).r;

    for ( float r = -radius; r <= radius; ++r )
    {
        float2 uv = pin.UV.xy + float2( 0, r * pixelOffset.y ); 
        colSum += BlurFunction( uv, r, centerDepth, weightSum );
    }

    float4 blurX = blurXTexture.Sample( pointSamplerClamp, pin.UV );
    float4 finalColor = ( colSum / weightSum + blurX ) * 0.5f;

    return finalColor;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
technique11 UpscaleBlur
{
    pass BlurX
    {
        SetVertexShader( CompileShader( vs_4_0, FullScreenQuadOutVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, BlurX() ) );
    }

    pass BlurY
    {
        SetVertexShader( CompileShader( vs_4_0, FullScreenQuadOutVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, BlurY() ) );
    }
}
