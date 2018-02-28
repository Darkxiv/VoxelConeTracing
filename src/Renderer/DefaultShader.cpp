#include <DefaultShader.h>
#include <D3DTextureBuffer2D.h>
#include <SceneGeometry.h>
#include <D3DRenderer.h>
#include <DirectXColors.h>
#include <d3dx11effect.h>
#include <Material.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool DefaultShader::Init( )
{
    if ( mIsReady )
        return mIsReady;

    bool isLoaded = mfx.Load();
    if ( isLoaded )
    {
        isLoaded = BuildLayout();
    }

    mIsReady = isLoaded;
    ASSERT( mIsReady );

    return mIsReady;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DefaultShader::Clear()
{
    mfx.Clear();
    COMSafeRelease( mInputLayout );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool DefaultShader::IsReady()
{
    return mIsReady;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DefaultShader::Draw( std::shared_ptr<D3DTextureBuffer2D> &colorRT, std::shared_ptr<D3DTextureBuffer2D> &depthRT,
    const std::vector<SceneGeometry> &objs, bool showAlpha )
{
    if ( !mIsReady )
        return;

    D3DRenderer &renderer = D3DRenderer::Get( );
    auto immediateContext = renderer.GetContext( );
    auto colorRTV = colorRT->GetRTV();
    auto depthDSV = depthRT->GetDSV();
    ID3D11RenderTargetView* rts[] = { colorRTV };
    immediateContext->OMSetRenderTargets( 1, rts, depthDSV );
    immediateContext->ClearRenderTargetView( colorRTV, DirectX::Colors::AliceBlue );
    immediateContext->ClearDepthStencilView( depthDSV, D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );

    DirectX::XMFLOAT4X4 view, proj;
    renderer.GetViewProjMats( view, proj );

    DirectX::XMMATRIX loadedView = DirectX::XMLoadFloat4x4( &view );
    DirectX::XMMATRIX worldViewProj = loadedView * DirectX::XMLoadFloat4x4( &proj );
    mfx.mfxWorldViewProj->SetMatrix( reinterpret_cast< float* >( &worldViewProj ) );
    mfx.mfxShowAlpha->SetBool( showAlpha );

    // set per material params
    immediateContext->IASetInputLayout( mInputLayout );
    immediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );
    //immediateContext->RSSetState( mNoCullRS );

    for each ( auto &obj in objs )
    {
        const std::shared_ptr<Material> &mat = obj.mMaterial;
        if ( mat->tex0 )
            mfx.mfxDefaultTexture->SetResource( mat->tex0->GetSRV() );
        else
            mfx.mfxDefaultTexture->SetResource( renderer.GetDefaultTexture( )->GetSRV( ) );

        mfx.mfxPass->Apply( 0, immediateContext );

        renderer.DrawGeometry( obj.mGeometryBuffer );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void DefaultShader::Draw( std::shared_ptr<D3DTextureBuffer2D> &colorRT, std::shared_ptr<D3DTextureBuffer2D> &depthRT,
    SceneGeometry &obj, bool showAlpha )
{
    std::vector<SceneGeometry> vec{ obj };
    Draw( colorRT, depthRT, vec, showAlpha );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11InputLayout* DefaultShader::GetDefaultInputLayout()
{
    return mInputLayout;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool DefaultShader::BuildLayout()
{
    // this layout will be the same for the most shaders

    D3DRenderer &renderer = D3DRenderer::Get( );
    auto d3dDevice = renderer.GetDevice( );

    D3D11_INPUT_ELEMENT_DESC vertexDesc[] =
    {
        { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "BINORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D11_INPUT_PER_VERTEX_DATA, 0 },
        //{ "COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 20, D3D11_INPUT_PER_VERTEX_DATA, 0 },
    };

    D3DX11_PASS_DESC passDesc;
    mfx.mTech->GetPassByIndex( 0 )->GetDesc( &passDesc );
    HRESULT hr = d3dDevice->CreateInputLayout(
        vertexDesc, ARRAYSIZE( vertexDesc ), passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &mInputLayout );

    ASSERT( hr == S_OK );

    return hr == S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
