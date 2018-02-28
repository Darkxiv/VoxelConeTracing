#ifndef __FX_IMGUI_H
#define __FX_IMGUI_H

#include <FXBindings/GeneralFX.h>

struct ImDrawData;
struct ID3D11InputLayout;
struct ID3D11Buffer;

struct FXImGui : public GeneralFX
{
    FXImGui() = default;
    ~FXImGui();

    bool Load();
    void Draw( ImDrawData* draw_data );

    FXType GetType();
    bool IsLoaded();

    ID3DX11EffectTechnique *mTech = nullptr;
    ID3DX11EffectPass *mfxDrawInterface = nullptr;
    ID3DX11EffectMatrixVariable *mfxProjectionMatrix = nullptr;
    ID3DX11EffectShaderResourceVariable *mfxTexture = nullptr;

private:
    bool mIsLoaded = false;
    ID3DX11Effect *mFX = nullptr;

    const FXType mType = FX_UI;

    ID3D11InputLayout *mInputLayoutP2T2C4 = nullptr;
    ID3D11Buffer* mVB = nullptr;
    ID3D11Buffer* mIB = nullptr;
    int mVBufferSize = 5000;
    int mIBufferSize = 10000;
};

#endif
