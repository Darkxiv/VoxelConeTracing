#include <GBuffer.h>
#include <D3DTextureBuffer2D.h>
#include <SceneGeometry.h>
#include <D3DRenderer.h>
#include <DirectXColors.h>
#include <d3dx11effect.h>
#include <Material.h>
#include <Light.h>
#include <ShadowMapper.h>
#include <Settings.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool GBuffer::Init( )
{
    if ( mIsReady )
        return mIsReady;

    mColor = D3DTextureBuffer2D::Create( true );
    mColor->SetMainRTResolutionScale( 1.0f );

    mNormal = D3DTextureBuffer2D::Create( true );
    mNormal->SetMainRTResolutionScale( 1.0f );

    mDepth = D3DTextureBuffer2D::Create( false, true );
    mDepth->SetMainRTResolutionScale( 1.0f );

    mIsReady = mfx.Load( );
    ASSERT( mIsReady );

    return mIsReady;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GBuffer::Clear()
{
    mIsReady = false;

    mfx.Clear();

    mColor.reset();
    mNormal.reset();
    mDepth.reset();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool GBuffer::IsReady()
{
    return mIsReady;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GBuffer::DrawGBuffer( const std::vector<SceneGeometry> &objs )
{
    ASSERT( mIsReady );
    if ( !mIsReady )
        return;

    // set gbuffer RTs
    D3DRenderer &renderer = D3DRenderer::Get( );
    auto immediateContext = renderer.GetContext( );
    auto colorRTV = mColor->GetRTV();
    auto normalRTV = mNormal->GetRTV();
    auto mainDSV = mDepth->GetDSV();
    ID3D11RenderTargetView* rts[] = { colorRTV, normalRTV };
    immediateContext->OMSetRenderTargets( 2, rts, mainDSV );
    immediateContext->ClearRenderTargetView( colorRTV, DirectX::Colors::SeaGreen );
    immediateContext->ClearRenderTargetView( normalRTV, DirectX::Colors::IndianRed );
    immediateContext->ClearDepthStencilView( mainDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );
    immediateContext->IASetInputLayout( renderer.GetDefaultInputLayout( ) );
    immediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    // set world view proj for scene
    renderer.GetViewProjMats( mSceneView, mSceneProj );
    DirectX::XMMATRIX worldViewProj = DirectX::XMLoadFloat4x4( &mSceneView ) * DirectX::XMLoadFloat4x4( &mSceneProj );
    mfx.mfxWorldViewProj->SetMatrix( reinterpret_cast< float* >( &worldViewProj ) );

    for each ( auto &obj in objs )
    {
        // set per material params
        const std::shared_ptr<Material> &mat = obj.mMaterial;
        if ( mat->tex0 )
            mfx.mfxAlbedoTexture->SetResource( mat->tex0->GetSRV() );
        else
            mfx.mfxAlbedoTexture->SetResource( renderer.GetDefaultTexture()->GetSRV( ) );

        if ( mat->tex1 )
        {
            mfx.mfxUseNormalMap->SetBool( true ); // performance hint: should be batched
            mfx.mfxNormalTexture->SetResource( mat->tex1->GetSRV() );
        }
        else
        {
            mfx.mfxUseNormalMap->SetBool( false );
        }

        mfx.mfxGPass->Apply( 0, immediateContext );

        renderer.DrawGeometry( obj.mGeometryBuffer );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GBuffer::DrawCombine( LightSource &lSource, ShadowMap &shadowMap )
{
    // TODO maybe combine shadowMaps and lSources idn; store scene matrix to get inverse

    ASSERT( mIsReady );
    if ( !mIsReady )
        return;

    D3DRenderer &renderer = D3DRenderer::Get( );
    Settings &settings = Settings::Get();

    // set fullscreen mats for quad
    DirectX::XMFLOAT4X4 view, proj;
    renderer.GetFullscreenQuadMats( view, proj );
    DirectX::XMMATRIX worldViewProj = DirectX::XMLoadFloat4x4( &view ) * DirectX::XMLoadFloat4x4( &proj );
    mfx.mfxWorldViewProj->SetMatrix( reinterpret_cast< float* >( &worldViewProj ) );

    // set inverse scene mats
    DirectX::XMMATRIX iProj = DirectX::XMMatrixInverse( nullptr, DirectX::XMLoadFloat4x4( &mSceneProj ) );
    DirectX::XMMATRIX iView = DirectX::XMMatrixInverse( nullptr, DirectX::XMLoadFloat4x4( &mSceneView ) );
    mfx.mfxInverseProj->SetMatrix( reinterpret_cast< float* >( &iProj ) );
    mfx.mfxInverseView->SetMatrix( reinterpret_cast< float* >( &iView ) );

    // set light mats and props
    DirectX::XMMATRIX worldToLightProj = DirectX::XMLoadFloat4x4( &shadowMap.mView ) * DirectX::XMLoadFloat4x4( &shadowMap.mProj );
    mfx.mfxLightProj->SetMatrix( reinterpret_cast< float* >( &worldToLightProj ) );

    DirectX::XMFLOAT3 &lColor = lSource.color;
    DirectX::XMFLOAT3 &lPos = lSource.position;
    DirectX::XMFLOAT3 &lDir = lSource.direction;
    DirectX::XMVECTOR vlColorRadius = DirectX::XMVectorSet( lColor.x, lColor.y, lColor.z, lSource.radius );
    DirectX::XMVECTOR vlPos = DirectX::XMVectorSet( lPos.x, lPos.y, lPos.z, 1.0f );
    DirectX::XMVECTOR vlDir = DirectX::XMVectorSet( lDir.x, lDir.y, lDir.z, 1.0f );
    mfx.mfxLightColorRadius->SetFloatVector( reinterpret_cast< float* >( &vlColorRadius ) );
    mfx.mfxLightPosition->SetFloatVector( reinterpret_cast< float* >( &vlPos ) );
    mfx.mfxLightDirection->SetFloatVector( reinterpret_cast< float* >( &lDir ) );
    mfx.mfxDirectInfluence->SetFloat( settings.mDirectInfluence );
    mfx.mfxShadowBias->SetFloat( settings.mShadowBias * 0.000001f );

    // set maps
    mfx.mfxAlbedoTexture->SetResource( mColor->GetSRV() );
    mfx.mfxNormalTexture->SetResource( mNormal->GetSRV() );
    mfx.mfxDepthTexture->SetResource( mDepth->GetSRV() );
    mfx.mfxShadowTexture->SetResource( shadowMap.mShadowTexture->GetSRV() );

    // set RT
    auto immediateContext = renderer.GetContext( );
    ID3D11RenderTargetView* tmpRT = renderer.GetMainRT( )->GetRTV();
    immediateContext->OMSetRenderTargets( 1, &tmpRT, nullptr );
    immediateContext->IASetInputLayout( renderer.GetDefaultInputLayout( ) );
    immediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    bool usedGI = false;
    if ( renderer.IsGIEnabled() )
    {
        auto &vct = renderer.GetVCT( );
        if ( settings.mVCTEnable && vct.IsReady( ) )
        {
            // set indirect ligth params
            mfx.mfxIndirectIrradiance->SetResource( vct.GetIndirectIrradiance( )->GetSRV( ) );

            mfx.mfxIndirectInfluence->SetFloat( settings.mIndirectInfluence );
            mfx.mfxAOInfluence->SetFloat( settings.mAOInfluence );

            mfx.mfxCombineWithVCTPass->Apply( 0, immediateContext );
            usedGI = true;
        }
    }

    if ( !usedGI )
    {
        mfx.mfxCombinePass->Apply( 0, immediateContext );
    }

    // actual draw
    renderer.DrawGeometry( renderer.GetQuad( ) );

    // TODO write more general way to clear pipeline!
    mfx.mfxAlbedoTexture->SetResource( nullptr );
    mfx.mfxNormalTexture->SetResource( nullptr );
    mfx.mfxDepthTexture->SetResource( nullptr );
    mfx.mfxShadowTexture->SetResource( nullptr );
    mfx.mfxIndirectIrradiance->SetResource( nullptr );
    mfx.mfxCombineWithVCTPass->Apply( 0, immediateContext );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DTextureBuffer2D> GBuffer::GetColor( )
{
    return mColor;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DTextureBuffer2D> GBuffer::GetNormal( )
{
    return mNormal;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DTextureBuffer2D> GBuffer::GetDepth( )
{
    return mDepth;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void GBuffer::GetSceneViewProj( DirectX::XMFLOAT4X4 &view, DirectX::XMFLOAT4X4 &proj )
{
    view = mSceneView;
    proj = mSceneProj;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
