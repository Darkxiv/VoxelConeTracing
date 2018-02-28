#ifndef __FX_GBUFFER_H
#define __FX_GBUFFER_H

#include <FXBindings/GeneralFX.h>

struct FXGBuffer : public GeneralFX
{
    FXGBuffer() = default;
    ~FXGBuffer();

    bool Load();
    void Clear();

    FXType GetType();
    bool IsLoaded();

    ID3DX11EffectTechnique *mTech = nullptr;
    ID3DX11EffectPass *mfxGPass = nullptr;
    ID3DX11EffectPass *mfxCombinePass = nullptr;
    ID3DX11EffectPass *mfxCombineWithVCTPass = nullptr;
    ID3DX11EffectMatrixVariable *mfxWorldViewProj = nullptr;

    ID3DX11EffectScalarVariable *mfxUseNormalMap = nullptr;

    ID3DX11EffectShaderResourceVariable *mfxAlbedoTexture = nullptr;
    ID3DX11EffectShaderResourceVariable *mfxNormalTexture = nullptr;
    ID3DX11EffectShaderResourceVariable *mfxDepthTexture = nullptr;
    ID3DX11EffectShaderResourceVariable *mfxIndirectIrradiance = nullptr;

    ID3DX11EffectMatrixVariable *mfxInverseProj = nullptr;
    ID3DX11EffectMatrixVariable *mfxInverseView = nullptr;

    ID3DX11EffectMatrixVariable *mfxLightProj = nullptr;
    ID3DX11EffectVectorVariable *mfxLightColorRadius = nullptr;
    ID3DX11EffectVectorVariable *mfxLightPosition = nullptr;
    ID3DX11EffectVectorVariable *mfxLightDirection = nullptr;
    ID3DX11EffectShaderResourceVariable *mfxShadowTexture = nullptr;
    ID3DX11EffectScalarVariable *mfxShadowBias = nullptr;
    ID3DX11EffectScalarVariable *mfxAOInfluence = nullptr;
    ID3DX11EffectScalarVariable *mfxDirectInfluence = nullptr;
    ID3DX11EffectScalarVariable *mfxIndirectInfluence = nullptr;

private:
    bool mIsLoaded = false;
    ID3DX11Effect *mFX = nullptr;

    const FXType mType = FX_GBUFFER;
};


#endif