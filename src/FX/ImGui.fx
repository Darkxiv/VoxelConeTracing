// default ImGui fx-shader

#include "utils.fx"

Texture2D texture0;

cbuffer vertexBuffer : register(b0)
{
    float4x4 ProjectionMatrix;
};

struct VS_INPUT
{
    float2 pos : POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};

struct PS_INPUT
{
    float4 pos : SV_POSITION;
    float4 col : COLOR0;
    float2 uv  : TEXCOORD0;
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
PS_INPUT DrawInterfaceVS(VS_INPUT input)
{
    PS_INPUT output;
    output.pos = mul( ProjectionMatrix, float4(input.pos.xy, 0.f, 1.f));
    output.col = input.col;
    output.uv  = input.uv;
    return output;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float4 DrawInterfacePS(PS_INPUT input) : SV_Target
{
    float4 out_col = input.col * texture0.Sample(linearSampler, input.uv);
    return out_col;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

RasterizerState ImGuiRS
{
    FILLMODE                = SOLID;
    CULLMODE                = NONE;
    DEPTHCLIPENABLE         = true;
    SCISSORENABLE           = true;
};

BlendState ImGuiBS
{
    AlphaToCoverageEnable   = false;
    BlendEnable[0]          = true;
    SrcBlend[0]             = SRC_ALPHA;
    DestBlend[0]            = INV_SRC_ALPHA;
    BlendOp[0]              = ADD;
    SrcBlendAlpha[0]        = INV_SRC_ALPHA;
    DestBlendAlpha[0]       = ZERO;
    BlendOpAlpha[0]         = ADD;
    RenderTargetWriteMask[0]= 0x0f; // COLOR_WRITE_ENABLE_ALL
};

DepthStencilState ImGuiDS
{
    DepthEnable = false;
    StencilEnable = false;
};

technique11 ImGui
{
    pass DrawInterface
    {
        SetVertexShader( CompileShader( vs_4_0, DrawInterfaceVS() ) );
        SetGeometryShader( NULL );
        SetPixelShader( CompileShader( ps_4_0, DrawInterfacePS() ) );
        
        SetRasterizerState( ImGuiRS );
        SetBlendState( ImGuiBS, float4(0.0f, 0.0f, 0.0f, 0.0f), 0xffffffff );
        SetDepthStencilState( ImGuiDS, 0 );
    }
}