#ifndef __FX_BRICK_BUFFER_H
#define __FX_BRICK_BUFFER_H

#include <GlobalUtils.h>
#include <FXBindings/GeneralFX.h>
#include <FXBindings/FXOctreeVariables.h>

class VCT;

struct FXGenerateBrickBuffer : public GeneralFX
{
    FXGenerateBrickBuffer() = default;
    ~FXGenerateBrickBuffer();

    bool Load();
    void BindVCTResources( VCT &vctResources );

    FXType GetType();
    bool IsLoaded();

    struct BrickBufferTechnique
    {
        BrickBufferTechnique() = default;
        ID3DX11EffectTechnique *mTech = nullptr;
        ID3DX11EffectPass *mConstructOpacity = nullptr;
        ID3DX11EffectPass *mAverageAlongAxisX = nullptr;
        ID3DX11EffectPass *mAverageAlongAxisY = nullptr;
        ID3DX11EffectPass *mAverageAlongAxisZ = nullptr;
        ID3DX11EffectPass *mGatherOpacityFromLowLevel = nullptr;
        ID3DX11EffectPass *mDebugBricks = nullptr;
        ID3DX11EffectPass *mDebugVoxels = nullptr;
    } mGenBrickBuffer;

    struct ProcessingShadowMapTechnique
    {
        ProcessingShadowMapTechnique( ) = default;
        ID3DX11EffectTechnique *mTech = nullptr;
        ID3DX11EffectPass *mProcessPass = nullptr;
        ID3DX11EffectPass *mResetOctreeFlags = nullptr;
        ID3DX11EffectPass *mAverageLitNodeValues = nullptr;
        ID3DX11EffectPass *mAverageAlongAxisX = nullptr;
        ID3DX11EffectPass *mAverageAlongAxisY = nullptr;
        ID3DX11EffectPass *mAverageAlongAxisZ = nullptr;
        ID3DX11EffectPass *mGatherValuesFromLowLevel = nullptr;
    } mProcessingShadowMap;

    ID3DX11EffectShaderResourceVariable *mfxVoxelArrayR = nullptr;
    ID3DX11EffectMatrixVariable *mfxWorldViewProj = nullptr;
    FXOctreeVariables mOctreeVariables;

    ID3DX11EffectScalarVariable *mfxBrickBufferSize = nullptr;
    ID3DX11EffectUnorderedAccessViewVariable *mfxOpacityBrickBufferRW    = nullptr;
    ID3DX11EffectShaderResourceVariable      *mfxOpacityBrickBufferR     = nullptr;
    ID3DX11EffectUnorderedAccessViewVariable *mfxBrickBufferRW           = nullptr;
    ID3DX11EffectShaderResourceVariable      *mfxBrickBufferR            = nullptr;
    ID3DX11EffectUnorderedAccessViewVariable *mfxIrradianceBrickBufferRW = nullptr;
    ID3DX11EffectShaderResourceVariable      *mfxIrradianceBrickBufferR  = nullptr;

    ID3DX11EffectScalarVariable *mfxCurrentOctreeLevel = nullptr;

    ID3DX11EffectUnorderedAccessViewVariable *mfxIndirectDrawBuffer = nullptr;

    // create FXBrickBuffer structure . . . . . . . . . . . . . . . . . rename someones to GenerateBrickBuffer?
    ID3DX11EffectScalarVariable *mfxDebugOctreeLevel = nullptr;

    ID3DX11EffectVectorVariable *mfxLightColorRadius = nullptr;
    ID3DX11EffectVectorVariable *mfxLightPosition = nullptr;
    ID3DX11EffectVectorVariable *mfxLightDirection = nullptr;

    ID3DX11EffectShaderResourceVariable *mfxShadowMap = nullptr;
    ID3DX11EffectVectorVariable *mfxShadowMapResolution = nullptr;

    ID3DX11EffectMatrixVariable *mfxShadowInverseProj = nullptr;
    ID3DX11EffectMatrixVariable *mfxShadowInverseView = nullptr;

private:
    bool mIsLoaded = false;
    ID3DX11Effect *mFX = nullptr;

    const FXType mType = FX_BRICK_BUFFER;
};

#endif
