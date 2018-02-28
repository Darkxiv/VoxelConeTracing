#include <FXBindings/FXDefault.h>
#include <d3dx11effect.h>
#include <GlobalUtils.h>
#include <D3DRenderer.h>

// FX-specific implementations

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FXDefault::~FXDefault( )
{
    Clear();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FXDefault::Load( )
{
    D3DRenderer &renderer = D3DRenderer::Get( );
    HRESULT hr = renderer.CreateEffect( "color.cso", &mFX );

    if ( hr == S_OK )
    {
        bool loadingCheck = true;

        GET_FX_VAR( loadingCheck, mTech, mFX->GetTechniqueByName( "ColorTech" ) );
        if ( mTech->IsValid() )
            GET_FX_VAR( loadingCheck, mfxPass, mTech->GetPassByName( "SimplePass" ) );

        GET_FX_VAR( loadingCheck, mfxWorldViewProj, mFX->GetVariableByName( "gWorldViewProj" )->AsMatrix( ) );
        GET_FX_VAR( loadingCheck, mfxDefaultTexture, mFX->GetVariableByName( "defaultTexture" )->AsShaderResource( ) );
        GET_FX_VAR( loadingCheck, mfxShowAlpha, mFX->GetVariableByName( "showAlpha" )->AsScalar( ) );

        mIsLoaded = loadingCheck;
    }
    ASSERT( mIsLoaded );

    return mIsLoaded;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FXDefault::Clear()
{
    mIsLoaded = false;
    COMSafeRelease( mFX );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
GeneralFX::FXType FXDefault::GetType()
{
    return mType;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FXDefault::IsLoaded()
{
    return mIsLoaded;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

