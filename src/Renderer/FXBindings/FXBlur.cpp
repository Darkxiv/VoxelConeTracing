#include <FXBindings/FXBlur.h>
#include <d3dx11effect.h>
#include <GlobalUtils.h>
#include <D3DRenderer.h>

// FX-specific implementations

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FXBlur::~FXBlur()
{
    COMSafeRelease( mFX );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FXBlur::Load( )
{
    D3DRenderer &renderer = D3DRenderer::Get( );
    HRESULT hr = renderer.CreateEffect( "blur.cso", &mFX );

    if ( hr == S_OK )
    {
        bool loadingCheck = true;

        GET_FX_VAR( loadingCheck, mTech, mFX->GetTechniqueByName( "UpscaleBlur" ) );
        if ( mTech->IsValid() )
        {
            GET_FX_VAR( loadingCheck, mfxPassX, mTech->GetPassByName( "BlurX" ) );
            GET_FX_VAR( loadingCheck, mfxPassY, mTech->GetPassByName( "BlurY" ) );
        }

        GET_FX_VAR( loadingCheck, mfxWorldViewProj, mFX->GetVariableByName( "gWorldViewProj" )->AsMatrix( ) );
        GET_FX_VAR( loadingCheck, mfxDepthTexture, mFX->GetVariableByName( "depthTexture" )->AsShaderResource( ) );
        GET_FX_VAR( loadingCheck, mfxColorTex, mFX->GetVariableByName( "colorTexture" )->AsShaderResource( ) );
        GET_FX_VAR( loadingCheck, mfxBlurXTex, mFX->GetVariableByName( "blurXTexture" )->AsShaderResource( ) );
        GET_FX_VAR( loadingCheck, mfxFalloff, mFX->GetVariableByName( "falloff" )->AsScalar( ) );
        GET_FX_VAR( loadingCheck, mfxSharpness, mFX->GetVariableByName( "sharpness" )->AsScalar( ) );
        GET_FX_VAR( loadingCheck, mfxRadius, mFX->GetVariableByName( "radius" )->AsScalar( ) );
        GET_FX_VAR( loadingCheck, mfxPixelOffset, mFX->GetVariableByName( "pixelOffset" )->AsVector( ) );

        mIsLoaded = loadingCheck;
    }
    ASSERT( mIsLoaded );

    return mIsLoaded;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
GeneralFX::FXType FXBlur::GetType( )
{
    return mType;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FXBlur::IsLoaded( )
{
    return mIsLoaded;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
