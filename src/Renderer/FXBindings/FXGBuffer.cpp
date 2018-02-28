#include <FXBindings/FXGBuffer.h>
#include <GlobalUtils.h>
#include <d3dx11effect.h>
#include <D3DRenderer.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FXGBuffer::~FXGBuffer( )
{
    Clear();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FXGBuffer::Load( )
{
    D3DRenderer &renderer = D3DRenderer::Get( );
    HRESULT hr = renderer.CreateEffect( "gbuffer.cso", &mFX );

    if ( hr == S_OK )
    {
        bool loadingCheck = true;

        GET_FX_VAR( loadingCheck, mTech, mFX->GetTechniqueByName( "GBuffer" ) );
        if ( mTech->IsValid( ) )
        {
            GET_FX_VAR( loadingCheck, mfxGPass, mTech->GetPassByName( "GPass" ) );
            GET_FX_VAR( loadingCheck, mfxCombinePass, mTech->GetPassByName( "CombinePass" ) );
            GET_FX_VAR( loadingCheck, mfxCombineWithVCTPass, mTech->GetPassByName( "CombinePassWithVCT" ) );
        }

        GET_FX_VAR( loadingCheck, mfxUseNormalMap, mFX->GetVariableByName( "useNormalMap" )->AsScalar( ) );

        GET_FX_VAR( loadingCheck, mfxWorldViewProj, mFX->GetVariableByName( "gWorldViewProj" )->AsMatrix( ) );
        GET_FX_VAR( loadingCheck, mfxAlbedoTexture, mFX->GetVariableByName( "albedoTexture" )->AsShaderResource( ) );
        GET_FX_VAR( loadingCheck, mfxNormalTexture, mFX->GetVariableByName( "normalTexture" )->AsShaderResource( ) );
        GET_FX_VAR( loadingCheck, mfxIndirectIrradiance, mFX->GetVariableByName( "indirectIrradianceTexture" )->AsShaderResource( ) );

        GET_FX_VAR( loadingCheck, mfxDepthTexture, mFX->GetVariableByName( "depthTexture" )->AsShaderResource( ) );
        GET_FX_VAR( loadingCheck, mfxInverseProj, mFX->GetVariableByName( "gInverseProj" )->AsMatrix( ) );
        GET_FX_VAR( loadingCheck, mfxInverseView, mFX->GetVariableByName( "gInverseView" )->AsMatrix( ) );

        GET_FX_VAR( loadingCheck, mfxLightProj, mFX->GetVariableByName( "gLightProj" )->AsMatrix( ) );
        GET_FX_VAR( loadingCheck, mfxLightColorRadius, mFX->GetVariableByName( "lColorRadius" )->AsVector( ) );
        GET_FX_VAR( loadingCheck, mfxLightPosition, mFX->GetVariableByName( "lPos" )->AsVector( ) );
        GET_FX_VAR( loadingCheck, mfxLightDirection, mFX->GetVariableByName( "lDirection" )->AsVector( ) );
        GET_FX_VAR( loadingCheck, mfxShadowTexture, mFX->GetVariableByName( "shadowTexture" )->AsShaderResource( ) );
        GET_FX_VAR( loadingCheck, mfxShadowBias, mFX->GetVariableByName( "shadowBias" )->AsScalar( ) );
        GET_FX_VAR( loadingCheck, mfxAOInfluence, mFX->GetVariableByName( "aoInfluence" )->AsScalar( ) );
        GET_FX_VAR( loadingCheck, mfxDirectInfluence, mFX->GetVariableByName( "directInfluence" )->AsScalar( ) );
        GET_FX_VAR( loadingCheck, mfxIndirectInfluence, mFX->GetVariableByName( "indirectInfluence" )->AsScalar( ) );

        mIsLoaded = loadingCheck;
    }
    ASSERT( mIsLoaded );

    return mIsLoaded;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FXGBuffer::Clear( )
{
    mIsLoaded = false;
    COMSafeRelease( mFX );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
GeneralFX::FXType FXGBuffer::GetType()
{
    return mType;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FXGBuffer::IsLoaded()
{
    return mIsLoaded;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

