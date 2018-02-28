#include <ShadowMapper.h>
#include <D3DTextureBuffer2D.h>
#include <SceneGeometry.h>
#include <D3DRenderer.h>
#include <DirectXColors.h>
#include <d3dx11effect.h>
#include <Material.h>
#include <Camera.h>
#include <Light.h>
#include <Settings.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ShadowMapper::Init( )
{
    if ( mIsReady )
        return mIsReady;

    D3DRenderer &renderer = D3DRenderer::Get( );
    Settings &settings = Settings::Get( );
    bool isLoaded = mfx.Load( );

    mShadowMap.mShadowTexture = D3DTextureBuffer2D::Create( false, true, true, false,
        nullptr, nullptr, nullptr, nullptr, 0.0f, settings.mShadowMapRes, settings.mShadowMapRes );

    mIsReady = isLoaded;
    ASSERT( mIsReady );

    return mIsReady;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ShadowMapper::Clear( )
{
    mIsReady = false;
    mfx.Clear( );
    mShadowMap.mShadowTexture.reset();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ShadowMapper::IsReady( )
{
    return mIsReady;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ShadowMapper::Draw( const std::vector<SceneGeometry> &objs, const LightSource &light,
    const std::pair<float, float> &spaceSize )
{
    ASSERT( mIsReady );
    if ( !mIsReady )
        return;

    D3DRenderer &renderer = D3DRenderer::Get( );
    Settings &settings = Settings::Get();

    auto &shTex = mShadowMap.mShadowTexture;
    renderer.SetViewport( float( shTex->GetWidth( ) ), float( shTex->GetHeight( ) ), 0.0f, 1.0f, 0, 0 );

    // create camera from light source position
    Camera lightCamera;
    lightCamera.SetPosition( light.position );
    lightCamera.SetDirection( light.direction );
    mShadowMap.mView = lightCamera.GetWorldToViewTransform( );
    DirectX::XMStoreFloat4x4( &mShadowMap.mProj, DirectX::XMMatrixOrthographicRH(
        spaceSize.first, spaceSize.second, settings.mShadowMapZNear, settings.mShadowMapZFar ) );

    DirectX::XMMATRIX worldViewProj = DirectX::XMLoadFloat4x4( &mShadowMap.mView ) * DirectX::XMLoadFloat4x4( &mShadowMap.mProj );
    mfx.mfxWorldViewProj->SetMatrix( reinterpret_cast< float* >( &worldViewProj ) );

    auto dsv = mShadowMap.mShadowTexture->GetDSV();
    auto immediateContext = renderer.GetContext( );
    ID3D11RenderTargetView* shmap[] = { nullptr };
    immediateContext->ClearDepthStencilView( dsv, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );
    immediateContext->OMSetRenderTargets( 1, shmap, dsv );
    immediateContext->IASetInputLayout( renderer.GetDefaultInputLayout() );
    immediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    mfx.mfxShadowMapPass->Apply( 0, immediateContext );
    for each ( auto &obj in objs )
    {
        renderer.DrawGeometry( obj.mGeometryBuffer );
    }

    renderer.SetDefaultViewport( );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ShadowMap& ShadowMapper::GetShadowMap()
{
    return mShadowMap;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
