#include <Blur.h>
#include <D3DTextureBuffer2D.h>
#include <D3DRenderer.h>
#include <d3dx11effect.h>
#include <Settings.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Blur::Init( )
{
    if ( mIsReady )
        return mIsReady;

    mIsReady = mfx.Load( );
    ASSERT( mIsReady );

    return mIsReady;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Blur::IsReady()
{
    return mIsReady;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Blur::UpscaleBlur( std::shared_ptr<D3DTextureBuffer2D> &depth, std::shared_ptr<D3DTextureBuffer2D> &smallTex,
    std::shared_ptr<D3DTextureBuffer2D> &bigTexTmp, std::shared_ptr<D3DTextureBuffer2D> &bigTexRT )
{
    if ( !mIsReady )
        return;

    D3DRenderer &renderer = D3DRenderer::Get( );
    auto immediateContext = renderer.GetContext( );

    // prepare blur
    DirectX::XMFLOAT4X4 view, proj;
    renderer.GetFullscreenQuadMats( view, proj );
    DirectX::XMMATRIX worldViewProj = DirectX::XMLoadFloat4x4( &view ) * DirectX::XMLoadFloat4x4( &proj );
    mfx.mfxWorldViewProj->SetMatrix( reinterpret_cast< float* >( &worldViewProj ) );

    Settings &settings = Settings::Get( );
    float radius = settings.mBlurRadius;
    mfx.mfxSharpness->SetFloat( settings.mBlurSharpness );
    mfx.mfxRadius->SetFloat( radius );
    float sigma = ( radius + 1.0f ) * 0.5f;
    mfx.mfxFalloff->SetFloat( 1.0f / ( 2.0f * sigma * sigma ) );
    mfx.mfxDepthTexture->SetResource( depth->GetSRV() );

    float pixelOffset[] = { 1.0f / depth->GetWidth( ), 1.0f / depth->GetHeight( ) };
    mfx.mfxPixelOffset->SetFloatVector( pixelOffset );

    // prepare X pass
    renderer.SetViewport( static_cast< float >( bigTexTmp->GetWidth( ) ), 
        static_cast< float >( bigTexTmp->GetHeight( ) ), 0.0f, 1.0f, 0, 0 );

    ID3D11RenderTargetView* tmpRT = bigTexTmp->GetRTV();
    immediateContext->OMSetRenderTargets( 1, &tmpRT, nullptr );
    mfx.mfxColorTex->SetResource( smallTex->GetSRV() );

    mfx.mfxPassX->Apply( 0, immediateContext );
    renderer.DrawGeometry( renderer.GetQuad( ) );

    // prepare Y pass
    tmpRT = bigTexRT->GetRTV( );
    immediateContext->OMSetRenderTargets( 1, &tmpRT, nullptr );
    mfx.mfxPassY->Apply( 0, immediateContext ); // apply rt here to set bigTexTmp as srv (effect11 lack)

    mfx.mfxBlurXTex->SetResource( bigTexTmp->GetSRV() );

    mfx.mfxPassY->Apply( 0, immediateContext );
    renderer.DrawGeometry( renderer.GetQuad( ) );

    // clear pipeline
    mfx.mfxDepthTexture->SetResource( nullptr );
    mfx.mfxColorTex->SetResource( nullptr );
    mfx.mfxBlurXTex->SetResource( nullptr );
    mfx.mfxPassY->Apply( 0, immediateContext );

    renderer.SetDefaultViewport( );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
