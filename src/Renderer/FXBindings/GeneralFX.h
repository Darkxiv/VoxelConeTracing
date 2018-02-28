#ifndef __GENERAL_FX_H
#define __GENERAL_FX_H

struct ID3DX11Effect;
struct ID3DX11EffectPass;
struct ID3DX11EffectTechnique;
struct ID3DX11EffectScalarVariable;
struct ID3DX11EffectVectorVariable;
struct ID3DX11EffectMatrixVariable;
struct ID3DX11EffectShaderResourceVariable;
struct ID3DX11EffectUnorderedAccessViewVariable;

struct GeneralFX
{
    enum FXType
    {
        FX_DEFAULT,
        FX_GBUFFER,
        FX_GEN_OCTREE,
        FX_BRICK_BUFFER,
        FX_CONE_TRACING,
        FX_UI,
        FX_SHADOW_MAP,
        FX_BLUR
    };

    virtual FXType GetType( ) = 0;
    virtual bool Load( ) = 0;
    virtual bool IsLoaded( ) = 0;
    virtual ~GeneralFX() {};
};

#endif