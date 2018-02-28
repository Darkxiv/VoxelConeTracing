#ifndef __FX_OCTREEVARIABLES_H
#define __FX_OCTREEVARIABLES_H

#include <utility>

namespace DirectX
{
    struct XMFLOAT3;
}

struct Octree;
struct ID3DX11Effect;
struct ID3DX11EffectVectorVariable;
struct ID3DX11EffectUnorderedAccessViewVariable;
struct ID3DX11EffectShaderResourceVariable;
struct ID3DX11EffectScalarVariable;

struct FXOctreeVariables
{
    // container for octree fx variables
    // see octreeUtils.fx

    FXOctreeVariables( ) = default;

    ID3DX11EffectVectorVariable *mfxMinBB = nullptr;
    ID3DX11EffectVectorVariable *mfxMaxBB = nullptr;

    ID3DX11EffectScalarVariable *mfxOctreeHeight = nullptr;
    ID3DX11EffectScalarVariable *mfxOctreeResolution = nullptr;
    ID3DX11EffectScalarVariable *mfxOctreeBufferSize = nullptr;
    ID3DX11EffectUnorderedAccessViewVariable *mfxNodesPackCounter = nullptr;
    ID3DX11EffectUnorderedAccessViewVariable *mfxOctreeRW = nullptr;
    ID3DX11EffectShaderResourceVariable *mfxOctreeR = nullptr;

    bool LoadFromFx( ID3DX11Effect *fx );

    // should be careful because these doesn't store any data
    void BindSceneBB( std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3> &sceneBB );
    void BindOctree( Octree &octree );

private:
    bool mIsLoaded = false;
};

#endif