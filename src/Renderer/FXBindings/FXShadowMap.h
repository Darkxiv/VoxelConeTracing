#ifndef __FX_SHADOW_MAP_H
#define __FX_SHADOW_MAP_H

#include <FXBindings/GeneralFX.h>

struct FXShadowMap : public GeneralFX
{
    FXShadowMap() = default;
    ~FXShadowMap();

    bool Load();
    void Clear();

    FXType GetType();
    bool IsLoaded();

    ID3DX11EffectTechnique *mShadowMapTech = nullptr;
    ID3DX11EffectPass *mfxShadowMapPass = nullptr;
    ID3DX11EffectMatrixVariable *mfxWorldViewProj = nullptr;

private:
    bool mIsLoaded = false;
    ID3DX11Effect *mFX = nullptr;

    const FXType mType = GeneralFX::FX_SHADOW_MAP;
};


#endif