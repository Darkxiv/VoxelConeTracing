#include "utils.fx"

float4x4 gWorldViewProj;
float4x4 gInverseView;
float4x4 gInverseProj;

struct GBufferPixelOut
{
    float4 AlbedoRT;
    float4 NormalRT;
};

struct LightingPixelOut
{
    float4 Color;
};

Texture2D albedoTexture;
Texture2D normalTexture;

Texture2D depthTexture;
Texture2D shadowTexture;

Texture2D indirectIrradianceTexture;

cbuffer ShadowMap
{
    float4x4 gLightProj;
    float4 lColorRadius;
    float4 lPos;
    float4 lDirection;
};

bool useNormalMap;
float shadowBias;
float aoInfluence;
float directInfluence;
float indirectInfluence;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FullVertexOut GBufferVS(Vertex_3F3F3F2F vin)
{
    FullVertexOut vout;
    
    vout.Position = vin.Pos;

    vout.PosH = mul( float4(vin.Pos, 1.0f), gWorldViewProj );
    vout.UV = vin.UV;

    // todo mul by inverse world
    vout.Normal = vin.Normal;
    vout.Binormal = vin.Binormal;

    return vout;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
GBufferPixelOut GBufferPS(FullVertexOut pin) : SV_Target
{
    float4 albedo = albedoTexture.Sample(linearSampler, pin.UV);
    
    if ( albedo.a < 0.5f )
        discard;

    float3 normal = normalize( pin.Normal );
    if ( useNormalMap )
    {
        float4 localNormal = normalTexture.Sample( linearSampler, pin.UV );
        normal = CalculateNormal( localNormal.xyz, normal, normalize( pin.Binormal ) );
    }
    // pack signed normal to unsigned space
    normal = ( normal + 1.0f ) * 0.5f;

    GBufferPixelOut output;
    output.AlbedoRT = float4( albedo.rgb, 1.0f );
    output.NormalRT = float4( normal.xyz, 1.0f );

    return output;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LightingPixelOut CombinePS( FullVertexOut pin, uniform bool useVoxelConeTracing ) : SV_Target
{
    LightingPixelOut output;
    
    float3 albedo = albedoTexture.Load( int3( pin.PosH.xy, 0 ) ).xyz;
    float3 normal = normalTexture.Load( int3( pin.PosH.xy, 0 ) ).xyz * 2.0f - 1.0f;
    float  depth  =  depthTexture.Load( int3( pin.PosH.xy, 0 ) ).r;
    
    float4 projCoords = float4( float2( pin.UV.x, 1.0f - pin.UV.y ) * 2.0f - 1.0f, depth, 1.0f);
    float4 worldPos = GetWorldPos( projCoords, gInverseProj, gInverseView );
    
    float4 lProj = mul( worldPos, gLightProj );
    lProj.xy = lProj.xy * 0.5f + 0.5f;
    lProj.y = 1.0f - lProj.y;
    
    float percentLit = 0.0f;
    if ( lProj.z >= 0.001f )
    {
        // PCF
        float shadowW = 0.0f, shadowH = 0.0f;
        shadowTexture.GetDimensions( shadowW, shadowH );
        float xOffset = 1.0 / shadowW;
        float yOffset = 1.0 / shadowH;
        
        float average = 0.0f;
        
        for ( float x = -1.0f; x < 1.1f; x += 1.0f )
        {
            for ( float y = -1.0f; y < 1.1f; y += 1.0f )
            {
                float2 offset = float2( x * xOffset, y * yOffset );
                average += shadowTexture.SampleCmpLevelZero( shadowSampler, lProj.xy + offset, lProj.z - shadowBias ).r;
            }
        }

        percentLit = average / 9;
    }

    float3 diffuse = albedo * clamp( dot( normal, normalize(-lDirection.xyz) ), 0.0f, 1.0f ) * lColorRadius.rgb;
    output.Color.rgb = diffuse * directInfluence * percentLit.r; // assume light without attenuation

    if ( useVoxelConeTracing )
    {
        // true ambient
        float4 indirectIrradiance = indirectIrradianceTexture.Sample( linearSampler, pin.UV );
        output.Color.rgb += lerp( 0.0f.xxx, indirectIrradiance.rgb, indirectInfluence ) * albedo;
        output.Color.rgb *= lerp( 1.0f, indirectIrradiance.a, aoInfluence );
    }

    output.Color.a = 1.0f;

    return output;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

technique11 GBuffer
{
    pass GPass
    {
        SetVertexShader( CompileShader( vs_4_0, GBufferVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, GBufferPS() ) );
    }
    
    pass CombinePass
    {
        SetVertexShader( CompileShader( vs_4_0, GBufferVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, CombinePS( false ) ) );
    }
    
    pass CombinePassWithVCT
    {
        SetVertexShader( CompileShader( vs_4_0, GBufferVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, CombinePS( true ) ) );
    }
}

