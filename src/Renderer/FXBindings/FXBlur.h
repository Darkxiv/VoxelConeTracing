#ifndef __FX_BLUR_H
#define __FX_BLUR_H

#include <FXBindings/GeneralFX.h>

struct FXBlur : public GeneralFX
{
    FXBlur( ) = default;
    ~FXBlur( );

    bool Load( );

    FXType GetType( );
    bool IsLoaded( );

    ID3DX11EffectTechnique *mTech = nullptr;
    ID3DX11EffectPass *mfxPassX = nullptr;
    ID3DX11EffectPass *mfxPassY = nullptr;

    ID3DX11EffectMatrixVariable *mfxWorldViewProj = nullptr;
    ID3DX11EffectShaderResourceVariable *mfxDepthTexture = nullptr;
    ID3DX11EffectShaderResourceVariable *mfxColorTex = nullptr;
    ID3DX11EffectShaderResourceVariable *mfxBlurXTex = nullptr;
    ID3DX11EffectScalarVariable *mfxFalloff = nullptr;
    ID3DX11EffectScalarVariable *mfxSharpness = nullptr;
    ID3DX11EffectScalarVariable *mfxRadius = nullptr;
    ID3DX11EffectVectorVariable *mfxPixelOffset = nullptr;

private:
    bool mIsLoaded = false;
    ID3DX11Effect *mFX = nullptr;

    const FXType mType = FX_BLUR;
};

#endif
