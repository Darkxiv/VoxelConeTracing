#ifndef __FX_CONE_TRACING_H
#define __FX_CONE_TRACING_H

#include <FXBindings/GeneralFX.h>
#include <FXBindings/FXOctreeVariables.h>

class VCT;

struct FXConeTracing : public GeneralFX
{
    FXConeTracing() = default;
    ~FXConeTracing();

    bool Load();
    void BindVCTResources( VCT &vctResources );

    FXType GetType();
    bool IsLoaded();

    ID3DX11EffectTechnique *mTech = nullptr;
    ID3DX11EffectPass *mfxConeTracing = nullptr;

    ID3DX11EffectMatrixVariable *mfxWorldViewProj = nullptr;

    ID3DX11EffectMatrixVariable *mfxInverseProj = nullptr;
    ID3DX11EffectMatrixVariable *mfxInverseView = nullptr;

    ID3DX11EffectShaderResourceVariable *mfxNormalTexture = nullptr;
    ID3DX11EffectShaderResourceVariable *mfxDepthTexture = nullptr;
    ID3DX11EffectVectorVariable *mfxResolutionScale = nullptr;

    ID3DX11EffectShaderResourceVariable *mfxVoxelArrayR = nullptr;
    ID3DX11EffectShaderResourceVariable *mfxOpacityBrickBufferR = nullptr;
    ID3DX11EffectShaderResourceVariable *mfxIrradianceBrickBufferR = nullptr;
    ID3DX11EffectScalarVariable *mfxBrickBufferSize = nullptr;
    FXOctreeVariables mOctreeVariables;
    ID3DX11EffectScalarVariable *mfxLambdaFalloff = nullptr;
    ID3DX11EffectScalarVariable *mfxLocalConeOffset = nullptr;
    ID3DX11EffectScalarVariable *mfxWorldConeOffset = nullptr;
    ID3DX11EffectScalarVariable *mfxIndirectAmplification = nullptr;
    ID3DX11EffectScalarVariable *mfxStepCorrection = nullptr;
    ID3DX11EffectScalarVariable *mfxUseOpacityBuffer = nullptr;

    ID3DX11EffectScalarVariable *mfxDebugView = nullptr;
    ID3DX11EffectScalarVariable *mfxDebugConeDir = nullptr;
    ID3DX11EffectScalarVariable *mfxDebugOctreeFirstLevel = nullptr;
    ID3DX11EffectScalarVariable *mfxDebugOctreeLastLevel = nullptr;
private:
    bool mIsLoaded = false;
    ID3DX11Effect *mFX = nullptr;

    const FXType mType = GeneralFX::FX_CONE_TRACING;
};


#endif