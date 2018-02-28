#include <FXBindings/FXOctreeVariables.h>
#include <d3dx11effect.h>
#include <DirectXMath.h>
#include <GlobalUtils.h>
#include <Octree.h>
#include <D3DRenderer.h>
#include <D3DTextureBuffer2D.h>
#include <D3DStructuredBuffer.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FXOctreeVariables::LoadFromFx( ID3DX11Effect *fx )
{
    bool fxCheck = true;
    GET_FX_VAR( fxCheck, mfxMinBB, fx->GetVariableByName( "minBB" )->AsVector( ) );
    GET_FX_VAR( fxCheck, mfxMaxBB, fx->GetVariableByName( "maxBB" )->AsVector( ) );

    GET_FX_VAR( fxCheck, mfxOctreeHeight, fx->GetVariableByName( "octreeHeight" )->AsScalar( ) );
    GET_FX_VAR( fxCheck, mfxOctreeResolution, fx->GetVariableByName( "octreeResolution" )->AsScalar( ) );
    GET_FX_VAR( fxCheck, mfxOctreeBufferSize, fx->GetVariableByName( "octreeBufferSize" )->AsScalar( ) );

    GET_FX_VAR( fxCheck, mfxNodesPackCounter, fx->GetVariableByName( "nodesPackCounter" )->AsUnorderedAccessView( ) );
    GET_FX_VAR( fxCheck, mfxOctreeRW, fx->GetVariableByName( "octreeRW" )->AsUnorderedAccessView( ) );
    GET_FX_VAR( fxCheck, mfxOctreeR, fx->GetVariableByName( "octreeR" )->AsShaderResource( ) );
    mIsLoaded = fxCheck;

    return mIsLoaded;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FXOctreeVariables::BindSceneBB( std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3> &sceneBB )
{
    ASSERT( mIsLoaded );
    if ( !mIsLoaded )
        return;

    mfxMinBB->SetFloatVector( reinterpret_cast< float* >( &sceneBB.first ) );
    mfxMaxBB->SetFloatVector( reinterpret_cast< float* >( &sceneBB.second ) );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FXOctreeVariables::BindOctree( Octree &octree )
{
    ASSERT( mIsLoaded );
    if ( !mIsLoaded )
        return;

    mfxOctreeHeight->SetInt( octree.mHeight );
    mfxOctreeResolution->SetInt( octree.mResolution );
    mfxOctreeBufferSize->SetInt( octree.mBufferSize );

    D3DRenderer &renderer = D3DRenderer::Get( );

    auto octreeTexSRV = octree.mOctreeTex->GetSRV();
    auto octreeTexUAV = octree.mOctreeTex->GetUAV();
    auto nodesCounter = octree.mNodesPackCounter->GetUAV();
    ASSERT( octreeTexSRV && octreeTexUAV && nodesCounter );
    if ( octreeTexSRV && octreeTexUAV && nodesCounter )
    {
        mfxOctreeRW->SetUnorderedAccessView( octreeTexUAV );
        mfxOctreeR->SetResource( octreeTexSRV );
        mfxNodesPackCounter->SetUnorderedAccessView( nodesCounter );
        return;
    }

    mfxOctreeRW->SetUnorderedAccessView( nullptr );
    mfxOctreeR->SetResource( nullptr );
    mfxNodesPackCounter->SetUnorderedAccessView( nullptr );
}