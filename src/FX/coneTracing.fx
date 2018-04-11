#include "octreeUtils.fx"
#include "brickBufferUtils.fx"

// This FX generates indirect illumination texture

float4x4 gWorldViewProj;
float4x4 gInverseView;
float4x4 gInverseProj;

struct FullScreenQuadOut
{
    float4 PosH : SV_POSITION;
    float2 UV: TEXCOORD;
};

Texture2D normalTexture;
Texture2D depthTexture;
float2 resScale;

bool useOpacityBuffer;
float lambdaFalloff;
float localConeOffset;
float worldConeOffset;
float indirectAmplification;
float stepCorrection;

uint debugConeDir;
uint debugOctreeFirstLevel;
uint debugOctreeLastLevel;
bool debugView;

static const uint conesNum = 5;

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
float GetNodeWidth( in uint octreeLevel )
{
    return ( maxBB.x - minBB.x ) / ( octreeResolution >> ( octreeHeight - octreeLevel ) );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool WorldToBrickPosition( in float3 worldPos, in uint octreeLevel, out float3 brickPos )
{
    if ( any( worldPos > maxBB ) || any( worldPos < minBB ) )
        return false;

    uint octreePos = WorlPosToOctreePos( worldPos );
    uint nodeIndex;
    uint3 nodeStartCoords;
    
    if ( TraverseOctreeR( octreePos, octreeLevel, nodeIndex, nodeStartCoords ) )
    {
        uint nodeID = IndexToID( nodeIndex );

        float3 coordsStart = float3( NodeIDToTextureCoords( nodeID ) ) / brickBufferSize;
    
        // convert nodeStartCoords to worldPos and calculate relative offset inside brick
        float nodeWidth = GetNodeWidth( octreeLevel );
        float3 startWPos = float3( nodeStartCoords ) / octreeResolution * ( maxBB - minBB ) + minBB;
        float3 coordsOffset = ( worldPos - startWPos ) / nodeWidth * SAMPLING_AREA_SIZE;

        brickPos = coordsStart + coordsOffset + SAMPLING_OFFSET;
        return true;
    }

    return false;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void RotateConesDir( float3 normal, inout float3 coneDir[conesNum] )
{
    // find rotation between normal and half-sphere orientation (coneDir[0])
    float cosRotAngle = dot( normal, coneDir[0] );
    
    // also we can add random rotates to cones
    
    // don't rotate if vecs are co-directional
    if ( cosRotAngle < 0.995f )
    {
        float3 rotVec = float3( 1.0f, 0.0f, 0.0f );
        float sinRotAngle = 0.01f;

        // use default rotVec for rotation if vecs are opposite
        if ( cosRotAngle > -0.995f )
        {
            rotVec = cross( coneDir[0], normal );
            sinRotAngle = length( rotVec );
            rotVec = normalize( rotVec );
        }

        // rotate half-sphere
        [unroll]
        for ( uint i = 0; i < conesNum; i++ )
        {
            float3 a = coneDir[i];
            float3 v = rotVec;
            float cosAV = dot( a, v );

            // don't rotate if vecs are co-directional
            if ( cosAV < 0.995f )
            {
                float3 aParrV = v * cosAV;
                float3 aPerpV = a - aParrV;
                
                float3 aPerpVNorm = normalize( aPerpV );
                
                float3 w = normalize( cross( v, aPerpVNorm ) );
                float3 daPerpV = ( aPerpVNorm * cosRotAngle + w * sinRotAngle ) * length( aPerpV );

                coneDir[i] = aParrV + daPerpV;
            }
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float4 ConeTracingPS( FullScreenQuadOut pin ) : SV_Target
{
    float3 normal = normalTexture.Load( int3( pin.PosH.xy * resScale, 0 ) ).xyz * 2.0f - 1.0f;
    float  depth  =  depthTexture.Load( int3( pin.PosH.xy * resScale, 0 ) ).r;

    // get world position
    float4 projCoords = float4( float2( pin.UV.x, 1.0f - pin.UV.y ) * 2.0f - 1.0f, depth, 1.0f);
    float3 worldPos = GetWorldPos( projCoords, gInverseProj, gInverseView ).xyz;

    // discard everything outside the octree
    if ( any( worldPos < minBB ) || any( worldPos >  maxBB ) )
        discard;

    // generate severals cones
    float coneAO[conesNum] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
    float4 coneCol[conesNum] = { 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx, 0.0f.xxxx };
    
    // half-sphere direction distribution
    // assume each cone has 60 degree
    float3 coneDir[conesNum] = {
        float3(  0.0f,      1.0f,  0.0f      ),
        float3(  0.374999f, 0.5f,  0.374999f ),
        float3(  0.374999f, 0.5f, -0.374999f ),
        float3( -0.374999f, 0.5f,  0.374999f ),
        float3( -0.374999f, 0.5f, -0.374999f )
    };
    
    // 6 cones case
    // float3 coneDir[conesNum] = {
    //    float3(  0.0f,      1.0f,  0.0f      ), float3(  0.0f,      0.5f,  0.866025f ), float3(  0.823639f, 0.5f,  0.267617f ),
    //    float3(  0.509037f, 0.5f, -0.700629f ), float3( -0.509037f, 0.5f, -0.700629f ), float3( -0.823639f, 0.5f,  0.267617f ) };

    RotateConesDir( normal, coneDir );
    
    const uint firstLevel = debugOctreeFirstLevel;
    const uint lastLevel = debugOctreeLastLevel;
    
    // TODO stepCorrection should be connected somehow to power function
    const float coneStep = 1.4142f * stepCorrection; // 1.0 / sqrt(2) * sin(30)
    float4 sampleCol = 0.0f;
    float4 prevCol = 0.0f;
    float4 curCol = 0.0f;
    float3 opacityXYZ = 0.0f;
    float opacity = 0.0f;
    float3 worldSamplePos, brickSamplePos;

    for ( uint i = 0; i < conesNum; i++ )
    {
        [unroll(4)]
        for ( uint octreeLevel = firstLevel; octreeLevel > lastLevel; octreeLevel-- )
        {
            // calculate sample pos on certain level ( position, normal, octreeLevel )
            float nodeWidth = GetNodeWidth( octreeLevel + 1 );
            float sampleOffset = localConeOffset + nodeWidth * coneStep;

            float aoFalloff = 1.0f / ( 1.0f + sampleOffset * lambdaFalloff );

            float3 worldSamplePos = worldPos + worldConeOffset * normal + coneDir[i] * sampleOffset;
            float3 brickSamplePos;

            if ( WorldToBrickPosition( worldSamplePos, octreeLevel, brickSamplePos ) )
            {
                sampleCol = irradianceBrickBufferR.Sample( linearSampler, brickSamplePos );
                if ( useOpacityBuffer )
                {
                    opacityXYZ = opacityBrickBufferR.Sample( linearSampler, brickSamplePos ).rgb;
                    opacity = dot( abs( opacityXYZ * coneDir[i] ), 1.0f ); // projection
                }
                else
                {
                    opacity = sampleCol.a;
                }
                coneAO[i] += opacity * aoFalloff;

                curCol = float4( sampleCol.rgb * opacity, opacity );
                prevCol = coneCol[i];

                coneCol[i].rgb = prevCol.rgb + curCol.rgb * ( 1.0f - prevCol.a );
                coneCol[i].a = prevCol.a + ( 1.0f - prevCol.a ) * curCol.a;
            }
            else if ( any( worldSamplePos > maxBB ) || any( worldSamplePos < minBB ) )
            {
                coneAO[i] += aoFalloff;
                coneCol[i].a = 1.0f;
            }

            if ( coneCol[i].a >= 1.0f )
                break;
        }
    }

    float weight = 1.0f / conesNum;
    float4 result = 0.0f;

    for ( i = 0; i < conesNum; i++ )
    {
        result += float4( coneCol[i].rgb, saturate( coneAO[i] ) ) * weight;
    }

    float4 output = float4( result.rgb * indirectAmplification, 1.0f - result.a );
    if ( debugView )
    {
        output =  float4( coneCol[debugConeDir].rgb * indirectAmplification, 1.0f - coneCol[debugConeDir].a );
    }
    
    return output;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
technique11 ConeTracing
{
    // use depth and normal g-buffer textures to calculate indirect illumination
    pass ConeTracing
    {
        SetVertexShader( CompileShader( vs_5_0, FullScreenQuadOutVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_5_0, ConeTracingPS() ) );
    }
}
