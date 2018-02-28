#ifndef __FX_GENERATE_OCTREE_H
#define __FX_GENERATE_OCTREE_H

#include <FXBindings/GeneralFX.h>
#include <FXBindings/FXOctreeVariables.h>

class VCT;

struct FXGenerateOctree : public GeneralFX
{
    FXGenerateOctree() = default;
    ~FXGenerateOctree();

    bool Load();

    void SetupViewProj();
    void BindVCTResources( VCT &vctResources );

    FXType GetType();
    bool IsLoaded();

    struct GenerateOctreeTechnique
    {
        GenerateOctreeTechnique() = default;
        ID3DX11EffectTechnique *mTech = nullptr;
        ID3DX11EffectPass *mCreateVoxelArray = nullptr;
        ID3DX11EffectPass *mFlagNodes = nullptr;
        ID3DX11EffectPass *mSubdivideNodes = nullptr;
        ID3DX11EffectPass *mConnectNeighbors = nullptr;
        ID3DX11EffectPass *mConnectNodesToVoxels = nullptr;
    } mGenOctree;

    ID3DX11EffectMatrixVariable *mfxWorldView = nullptr;
    ID3DX11EffectMatrixVariable *mfxOrthoProj = nullptr;

    ID3DX11EffectShaderResourceVariable *mfxAlbedoTexture = nullptr;
    ID3DX11EffectShaderResourceVariable *mfxNormalTexture = nullptr;

    ID3DX11EffectScalarVariable *mfxUseNormalMap = nullptr;

    ID3DX11EffectScalarVariable *mfxFragmentBufferSize = nullptr;

    ID3DX11EffectUnorderedAccessViewVariable *mfxVoxelArrayRW = nullptr;
    ID3DX11EffectShaderResourceVariable *mfxVoxelArrayR = nullptr;
    ID3DX11EffectUnorderedAccessViewVariable *mfxIndirectDrawBuffer = nullptr;

    FXOctreeVariables mOctreeVariables;
    ID3DX11EffectScalarVariable *mfxCurrentOctreeLevel = nullptr;

private:
    bool mIsLoaded = false;
    ID3DX11Effect *mFX = nullptr;

    const FXType mType = FX_GEN_OCTREE;
};

#endif