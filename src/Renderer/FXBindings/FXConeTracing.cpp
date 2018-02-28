#include <FXBindings/FXConeTracing.h>
#include <d3dx11effect.h>
#include <GlobalUtils.h>
#include <VCT.h>
#include <D3DRenderer.h>
#include <D3DTextureBuffer2D.h>
#include <D3DTextureBuffer3D.h>
#include <D3DStructuredBuffer.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FXConeTracing::~FXConeTracing( )
{
    COMSafeRelease( mFX );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FXConeTracing::Load( )
{
    D3DRenderer &renderer = D3DRenderer::Get( );
    HRESULT hr = renderer.CreateEffect( "coneTracing.cso", &mFX );

    if ( hr == S_OK )
    {
        bool fxCheck = true;

        GET_FX_VAR( fxCheck, mTech, mFX->GetTechniqueByName( "ConeTracing" ) );
        if ( mTech->IsValid( ) )
        {
            GET_FX_VAR( fxCheck, mfxConeTracing, mTech->GetPassByName( "ConeTracing" ) );
        }

        GET_FX_VAR( fxCheck, mfxWorldViewProj, mFX->GetVariableByName( "gWorldViewProj" )->AsMatrix( ) );

        GET_FX_VAR( fxCheck, mfxInverseProj, mFX->GetVariableByName( "gInverseProj" )->AsMatrix( ) );
        GET_FX_VAR( fxCheck, mfxInverseView, mFX->GetVariableByName( "gInverseView" )->AsMatrix( ) );

        GET_FX_VAR( fxCheck, mfxNormalTexture, mFX->GetVariableByName( "normalTexture" )->AsShaderResource( ) );
        GET_FX_VAR( fxCheck, mfxDepthTexture, mFX->GetVariableByName( "depthTexture" )->AsShaderResource( ) );
        GET_FX_VAR( fxCheck, mfxResolutionScale, mFX->GetVariableByName( "resScale" )->AsVector( ) );

        GET_FX_VAR( fxCheck, mfxVoxelArrayR, mFX->GetVariableByName( "voxelArrayR" )->AsShaderResource( ) );
        GET_FX_VAR( fxCheck, mfxOpacityBrickBufferR, mFX->GetVariableByName( "opacityBrickBufferR" )->AsShaderResource( ) );

        GET_FX_VAR( fxCheck, mfxIrradianceBrickBufferR, mFX->GetVariableByName( "irradianceBrickBufferR" )->AsShaderResource( ) );
        GET_FX_VAR( fxCheck, mfxBrickBufferSize, mFX->GetVariableByName( "brickBufferSize" )->AsScalar( ) );

        GET_FX_VAR( fxCheck, mfxLambdaFalloff, mFX->GetVariableByName( "lambdaFalloff" )->AsScalar( ) );
        GET_FX_VAR( fxCheck, mfxLocalConeOffset, mFX->GetVariableByName( "localConeOffset" )->AsScalar( ) );
        GET_FX_VAR( fxCheck, mfxWorldConeOffset, mFX->GetVariableByName( "worldConeOffset" )->AsScalar( ) );
        GET_FX_VAR( fxCheck, mfxIndirectAmplification, mFX->GetVariableByName( "indirectAmplification" )->AsScalar( ) );
        GET_FX_VAR( fxCheck, mfxStepCorrection, mFX->GetVariableByName( "stepCorrection" )->AsScalar( ) );
        GET_FX_VAR( fxCheck, mfxUseOpacityBuffer, mFX->GetVariableByName( "useOpacityBuffer" )->AsScalar( ) );

        GET_FX_VAR( fxCheck, mfxDebugConeDir, mFX->GetVariableByName( "debugConeDir" )->AsScalar( ) );
        GET_FX_VAR( fxCheck, mfxDebugView, mFX->GetVariableByName( "debugView" )->AsScalar( ) );

        GET_FX_VAR( fxCheck, mfxDebugOctreeFirstLevel, mFX->GetVariableByName( "debugOctreeFirstLevel" )->AsScalar( ) );
        GET_FX_VAR( fxCheck, mfxDebugOctreeLastLevel, mFX->GetVariableByName( "debugOctreeLastLevel" )->AsScalar( ) );

        fxCheck &= mOctreeVariables.LoadFromFx( mFX );
        mIsLoaded = fxCheck;
    }
    ASSERT( mIsLoaded );

    return mIsLoaded;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FXConeTracing::BindVCTResources( VCT &vctResources )
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
    mfxOpacityBrickBufferR->SetResource( vctResources.GetOpacityBrickBuffer( )->GetSRV() );

    mfxBrickBufferSize->SetInt( vctResources.GetBrickBufferSize( ) );
    mfxLambdaFalloff->SetFloat( vctResources.GetLambdaFalloff( ) );
    mfxLocalConeOffset->SetFloat( vctResources.GetLocalConeOffset( ) );
    mfxWorldConeOffset->SetFloat( vctResources.GetWorldConeOffset() );
    mfxIndirectAmplification->SetFloat( vctResources.GetIndirectInfluence( ) );
    mfxStepCorrection->SetFloat( vctResources.GetStepCorrection( ) );
    mfxUseOpacityBuffer->SetBool( vctResources.GetUseOpacityBuffer( ) );

    auto &indirectTex = vctResources.GetIndirectIrradianceSmall( );
    float resScale[] = { static_cast<float>( renderer.GetWidth() ) / indirectTex->GetWidth(),
        float( renderer.GetHeight() ) / indirectTex->GetHeight() };
    mfxResolutionScale->SetFloatVector( resScale );

    mfxDebugView->SetBool( vctResources.GetDebugView( ) );
    mfxDebugConeDir->SetInt( vctResources.GetDebugConeDir( ) );
    mfxDebugOctreeFirstLevel->SetInt( vctResources.GetDebugOctreeFirstLevel( ) );
    mfxDebugOctreeLastLevel->SetInt( vctResources.GetDebugOctreeLastLevel( ) );

    mfxIrradianceBrickBufferR->SetResource( vctResources.GetIrradianceBrickBuffer( )->GetSRV() );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
GeneralFX::FXType FXConeTracing::GetType()
{
    return mType;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FXConeTracing::IsLoaded()
{
    return mIsLoaded;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
