#include <FXBindings/FXGenerateBrickBuffer.h>
#include <GlobalUtils.h>
#include <d3dx11effect.h>
#include <VCT.h>
#include <D3DRenderer.h>
#include <D3DTextureBuffer2D.h>
#include <D3DTextureBuffer3D.h>
#include <D3DStructuredBuffer.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FXGenerateBrickBuffer::~FXGenerateBrickBuffer( )
{
    COMSafeRelease( mFX );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FXGenerateBrickBuffer::Load( )
{
    D3DRenderer &renderer = D3DRenderer::Get( );
    HRESULT hr = renderer.CreateEffect( "brickBuffer.cso", &mFX );

    if ( hr == S_OK )
    {
        bool fxCheck = true;

        GET_FX_VAR( fxCheck, mGenBrickBuffer.mTech, mFX->GetTechniqueByName( "GenerateBrickBuffer" ) );
        if ( mGenBrickBuffer.mTech->IsValid( ) )
        {
            auto &mgb = mGenBrickBuffer;
            GET_FX_VAR( fxCheck, mgb.mConstructOpacity, mgb.mTech->GetPassByName( "ConstructOpacity" ) );
            GET_FX_VAR( fxCheck, mgb.mAverageAlongAxisX, mgb.mTech->GetPassByName( "AverageAlongAxisX" ) );
            GET_FX_VAR( fxCheck, mgb.mAverageAlongAxisY, mgb.mTech->GetPassByName( "AverageAlongAxisY" ) );
            GET_FX_VAR( fxCheck, mgb.mAverageAlongAxisZ, mgb.mTech->GetPassByName( "AverageAlongAxisZ" ) );
            GET_FX_VAR( fxCheck, mgb.mGatherOpacityFromLowLevel, mgb.mTech->GetPassByName( "GatherOpacityFromLowLevel" ) );
            GET_FX_VAR( fxCheck, mgb.mDebugBricks, mgb.mTech->GetPassByName( "DebugBricks" ) );
            GET_FX_VAR( fxCheck, mgb.mDebugVoxels, mgb.mTech->GetPassByName( "DebugVoxels" ) );
        }

        GET_FX_VAR( fxCheck, mProcessingShadowMap.mTech, mFX->GetTechniqueByName( "GenerateLightBriks" ) );
        if ( mProcessingShadowMap.mTech->IsValid( ) )
        {
            auto &psm = mProcessingShadowMap;
            GET_FX_VAR( fxCheck, psm.mProcessPass, psm.mTech->GetPassByName( "ProcessingShadowMap" ) );
            GET_FX_VAR( fxCheck, psm.mResetOctreeFlags, psm.mTech->GetPassByName( "ResetOctreeFlags" ) );
            GET_FX_VAR( fxCheck, psm.mAverageLitNodeValues, psm.mTech->GetPassByName( "AverageLitNodeValues" ) );
            GET_FX_VAR( fxCheck, psm.mAverageAlongAxisX, psm.mTech->GetPassByName( "AverageAlongAxisX" ) );
            GET_FX_VAR( fxCheck, psm.mAverageAlongAxisY, psm.mTech->GetPassByName( "AverageAlongAxisY" ) );
            GET_FX_VAR( fxCheck, psm.mAverageAlongAxisZ, psm.mTech->GetPassByName( "AverageAlongAxisZ" ) );
            GET_FX_VAR( fxCheck, psm.mGatherValuesFromLowLevel, psm.mTech->GetPassByName( "GatherValuesFromLowLevel" ) );
        }

        GET_FX_VAR( fxCheck, mfxVoxelArrayR, mFX->GetVariableByName( "voxelArrayR" )->AsShaderResource( ) );
        GET_FX_VAR( fxCheck, mfxWorldViewProj, mFX->GetVariableByName( "gWorldViewProj" )->AsMatrix( ) );
        GET_FX_VAR( fxCheck, mfxIndirectDrawBuffer, mFX->GetVariableByName( "indirectDrawBuffer" )->AsUnorderedAccessView( ) );
        GET_FX_VAR( fxCheck, mfxCurrentOctreeLevel, mFX->GetVariableByName( "currentOctreeLevel" )->AsScalar( ) );

        GET_FX_VAR( fxCheck, mfxBrickBufferSize, mFX->GetVariableByName( "brickBufferSize" )->AsScalar( ) );
        GET_FX_VAR( fxCheck, mfxOpacityBrickBufferRW, mFX->GetVariableByName( "opacityBrickBufferRW" )->AsUnorderedAccessView( ) );
        GET_FX_VAR( fxCheck, mfxBrickBufferRW, mFX->GetVariableByName( "brickBufferRW" )->AsUnorderedAccessView( ) );
        GET_FX_VAR( fxCheck, mfxBrickBufferR, mFX->GetVariableByName( "brickBufferR" )->AsShaderResource( ) );
        GET_FX_VAR( fxCheck, mfxOpacityBrickBufferR, mFX->GetVariableByName( "opacityBrickBufferR" )->AsShaderResource( ) );
        GET_FX_VAR( fxCheck, mfxIrradianceBrickBufferRW, mFX->GetVariableByName( "irradianceBrickBufferRW" )->AsUnorderedAccessView( ) );
        GET_FX_VAR( fxCheck, mfxIrradianceBrickBufferR, mFX->GetVariableByName( "irradianceBrickBufferR" )->AsShaderResource( ) );

        GET_FX_VAR( fxCheck, mfxDebugOctreeLevel, mFX->GetVariableByName( "debugOctreeLevel" )->AsScalar( ) );
        GET_FX_VAR( fxCheck, mfxLightColorRadius, mFX->GetVariableByName( "lColorRadius" )->AsVector( ) );
        GET_FX_VAR( fxCheck, mfxLightPosition, mFX->GetVariableByName( "lPos" )->AsVector( ) );
        GET_FX_VAR( fxCheck, mfxLightDirection, mFX->GetVariableByName( "lDirection" )->AsVector( ) );
        GET_FX_VAR( fxCheck, mfxShadowMap, mFX->GetVariableByName( "shadowMap" )->AsShaderResource( ) );
        GET_FX_VAR( fxCheck, mfxShadowMapResolution, mFX->GetVariableByName( "shadowMapResolution" )->AsVector( ) );

        GET_FX_VAR( fxCheck, mfxShadowInverseProj, mFX->GetVariableByName( "gShadowInverseProj" )->AsMatrix( ) );
        GET_FX_VAR( fxCheck, mfxShadowInverseView, mFX->GetVariableByName( "gShadowInverseView" )->AsMatrix( ) );

        fxCheck &= mOctreeVariables.LoadFromFx( mFX );
        mIsLoaded = fxCheck;
    }
    ASSERT( mIsLoaded );

    return mIsLoaded;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FXGenerateBrickBuffer::BindVCTResources( VCT &vctResources )
{
    ASSERT( mIsLoaded );
    if ( !mIsLoaded )
        return;

    D3DRenderer &renderer = D3DRenderer::Get( );

    std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3> sceneBB = renderer.GetStaticSceneBBRect( );
    Octree &octree = vctResources.GetOctree( );
    mOctreeVariables.BindSceneBB( sceneBB );
    mOctreeVariables.BindOctree( octree );

    mfxVoxelArrayR->SetResource( vctResources.GetVoxelArray( )->GetSRV( ) );
    mfxIndirectDrawBuffer->SetUnorderedAccessView( vctResources.GetIndirectDrawBuffer( )->GetUAV() );

    auto opacityBrickBuffer = vctResources.GetOpacityBrickBuffer();
    mfxOpacityBrickBufferRW->SetUnorderedAccessView( opacityBrickBuffer->GetUAV( ) );
    mfxOpacityBrickBufferR->SetResource( opacityBrickBuffer->GetSRV( ) );

    auto irradianceBuf = vctResources.GetIrradianceBrickBuffer();
    mfxIrradianceBrickBufferRW->SetUnorderedAccessView( irradianceBuf->GetUAV( ) );
    mfxIrradianceBrickBufferR->SetResource( irradianceBuf->GetSRV( ) );

    mfxBrickBufferSize->SetInt( vctResources.GetBrickBufferSize( ) );
    mfxDebugOctreeLevel->SetInt( vctResources.GetDebugOctreeLevel( ) );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
GeneralFX::FXType FXGenerateBrickBuffer::GetType()
{
    return mType;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FXGenerateBrickBuffer::IsLoaded()
{
    return mIsLoaded;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

