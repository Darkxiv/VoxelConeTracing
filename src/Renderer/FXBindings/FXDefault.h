#ifndef __FX_DEFAULT_H
#define __FX_DEFAULT_H

#include <FXBindings/GeneralFX.h>

struct FXDefault : public GeneralFX
{
    FXDefault() = default;
    ~FXDefault();

    bool Load();
    void Clear();

    FXType GetType();
    bool IsLoaded();

    ID3DX11EffectTechnique *mTech = nullptr;
    ID3DX11EffectPass *mfxPass = nullptr;
    ID3DX11EffectMatrixVariable *mfxWorldViewProj = nullptr;
    ID3DX11EffectShaderResourceVariable *mfxDefaultTexture = nullptr;
    ID3DX11EffectScalarVariable *mfxShowAlpha = nullptr;

private:
    bool mIsLoaded = false;
    ID3DX11Effect *mFX = nullptr;

    const FXType mType = FX_DEFAULT;
};

#endif
