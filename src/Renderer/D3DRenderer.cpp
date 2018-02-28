#include <D3DRenderer.h>
#include <D3DTextureBuffer2D.h>
#include <D3DTextureBuffer3D.h>
#include <D3DStructuredBuffer.h>
#include <d3dx11effect.h>
#include <directxcolors.h>
#include <cmath>
#include <stdlib.h>
#include <Camera.h>
#include <GeometryGenerator.h>
#include <SceneGeometry.h>
#include <Material.h>
#include <Light.h>
#include <D3DGeometryBuffer.h>
#include <Settings.h>

#include <fstream>
#include <vector>
#include <algorithm>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3DRenderer& D3DRenderer::Get( )
{
    static D3DRenderer renderer;
    return renderer;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool D3DRenderer::Init( HWND hWnd )
{
    if ( !DirectX::XMVerifyCPUSupport( ) )
        return false;

    RECT rc;
    GetClientRect( hWnd, &rc );
    mWidth = rc.right - rc.left;
    mHeight = rc.bottom - rc.top;

    D3D_FEATURE_LEVEL featureLevels[] =
    {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    UINT numFeatureLevels = ARRAYSIZE( featureLevels );

    HRESULT hr = CreateDevice( featureLevels, numFeatureLevels );
    if ( hr != S_OK )
    {
        WARNING( hr != S_OK, "Can't create dx11.1 device, trying dx10.0" );
        mGIEnabled = false;

        D3D_FEATURE_LEVEL featureLevels[] = { D3D_FEATURE_LEVEL_10_0 };
        UINT numFeatureLevels = ARRAYSIZE( featureLevels );

        hr = CreateDevice( featureLevels, numFeatureLevels );
        if ( hr != S_OK )
            return false;
    }

    hr = CreateSwapChain( hWnd, mWidth, mHeight );
    if ( hr != S_OK )
    {
        ASSERT( false );
        return false;
    }

    // Create a render target view
    ID3D11Texture2D* pBackBuffer = nullptr;
    hr = mSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), reinterpret_cast<void**>( &pBackBuffer ) );
    if ( FAILED( hr ) )
        return false;

    ID3D11RenderTargetView *renderTargetView;
    hr = md3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &renderTargetView );
    pBackBuffer->Release( );
    if ( FAILED( hr ) )
        return false;

    D3D11_TEXTURE2D_DESC bufferDesc;
    pBackBuffer->GetDesc( &bufferDesc );

    mMainRT = D3DTextureBuffer2D::CreateFromRTV( renderTargetView );
    mMainDepth = D3DTextureBuffer2D::Create( false, true );
    mMainDepth->SetMainRTResolutionScale( 1.0f );
    mImmediateContext->OMSetRenderTargets( 1, &renderTargetView, mMainDepth->GetDSV( ) );

    // Setup the viewport
    SetDefaultViewport( );

    //BuildGeometryBuffer();
    CreateDefaultGeometry( );

    D3D11_RASTERIZER_DESC rsDesc;
    ZeroMemory( &rsDesc, sizeof( D3D11_RASTERIZER_DESC ) );
    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_NONE;
    rsDesc.FrontCounterClockwise = false;
    rsDesc.DepthClipEnable = true;
    hr = md3dDevice->CreateRasterizerState( &rsDesc, &mNoCullRS );
    ASSERT( hr == S_OK );

    rsDesc.FillMode = D3D11_FILL_SOLID;
    rsDesc.CullMode = D3D11_CULL_BACK;
    rsDesc.FrontCounterClockwise = true;
    rsDesc.DepthClipEnable = true;
    hr = md3dDevice->CreateRasterizerState( &rsDesc, &mCullRS );
    ASSERT( hr == S_OK );

    D3D11_DEPTH_STENCIL_DESC dsDesc;
    ZeroMemory( &dsDesc, sizeof( D3D11_RASTERIZER_DESC ) );
    hr = md3dDevice->CreateDepthStencilState( &dsDesc, &mNoDepthNoStencilDS );
    ASSERT( hr == S_OK );

    dsDesc.DepthEnable = true;
    dsDesc.DepthFunc = D3D11_COMPARISON_LESS;
    dsDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    hr = md3dDevice->CreateDepthStencilState( &dsDesc, &mDepthNoStencilDS );
    ASSERT( hr == S_OK );

    CreateDefaultTexture( );
    CreateDefaultMaterial( );

    bool initialized = true;
    initialized &= CreateCopyCounterBuffer( );
    initialized &= CreateSyncQueries( );

    // load shaders
    initialized &= mDefaultShader.Init( );
    initialized &= mGBuffer.Init( );
    initialized &= mShadowMapper.Init( );
    initialized &= mUIDrawer.Init( hWnd );

    ASSERT( initialized );
    if ( !initialized )
        return false;

    if ( mGIEnabled )
    {
        mGIEnabled &= mVCT.Init();
        mGIEnabled &= mBlur.Init();
    }
    ASSERT( mGIEnabled );

    ResetView();

    return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool D3DRenderer::Resize( HWND hWnd, UINT width, UINT height )
{
    bool ready = mSwapChain != nullptr && md3dDevice != nullptr && mImmediateContext != nullptr;
    ASSERT( ready );
    if ( !ready )
        return false;

    if ( width == 0 && height == 0 )
        return true;

    mMainRT.reset();

    mWidth = width;
    mHeight = height;

    // note: mSwapChain->ResizeBuffers cast DXGI_ERROR_INVALID_CALL if we use RenderDoc
    // this happens due to refCounting problems, see https://github.com/baldurk/renderdoc/wiki/D3D11-back-end-details

    // Create a render target view
    HRESULT hr = mSwapChain->ResizeBuffers( 0, width, height, DXGI_FORMAT_UNKNOWN, 0 );
    if ( hr == S_OK )
    {
        // Create a render target view
        ID3D11Texture2D* pBackBuffer = nullptr;
        hr = mSwapChain->GetBuffer( 0, __uuidof( ID3D11Texture2D ), reinterpret_cast<void**>( &pBackBuffer ) );
        if ( hr == S_OK )
        {
            ID3D11RenderTargetView *renderTargetView;
            hr = md3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &renderTargetView );
            pBackBuffer->Release( );

            D3D11_TEXTURE2D_DESC bufferDesc;
            pBackBuffer->GetDesc( &bufferDesc );

            if ( width != 0 && height != 0 )
            {
                // set valid state only
                std::set<D3DTextureBuffer2D*> &storage = D3DTextureBuffer2D::GetStorage( );
                for ( auto it : storage )
                {
                    // resize view-dependent textures only
                    if ( it->GetMainRTResolutionScale( ) > 0.0f )
                        it->Resize( );
                }
            }

            mMainRT = D3DTextureBuffer2D::CreateFromRTV( renderTargetView );

            ResetView( );
        }
    }

    ASSERT( !FAILED( hr ), "Error during swapChain resize" );
    if ( FAILED( hr ) )
    {
        return false;
    }

    return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::Cleanup( )
{
    mGBuffer.Clear();
    mShadowMapper.Clear( );
    mVCT.Clear();
    mUIDrawer.Clear();

    mMainRT.reset();
    mMainDepth.reset();
    mDefaultTexture.reset( );
    mDefaultMaterial.reset( );
    mQuad.reset( );

    COMSafeRelease( mSyncQueryA );
    COMSafeRelease( mSyncQueryB );
    COMSafeRelease( mCopyCounterBuffer );
    COMSafeRelease( mNoCullRS );
    COMSafeRelease( mCullRS );
    COMSafeRelease( mNoDepthNoStencilDS );
    COMSafeRelease( mDepthNoStencilDS );

    if ( mImmediateContext )
    {
        mImmediateContext->ClearState( );
        mImmediateContext = nullptr;
    }

    COMSafeRelease( mSwapChain1 );
    COMSafeRelease( mSwapChain );
    COMSafeRelease( mImmediateContext1 );
    COMSafeRelease( mImmediateContext );
    COMSafeRelease( md3dDevice1 );

#ifdef _DEBUG
    ReportLiveObjects();
#endif
    COMSafeRelease( md3dDevice );

    ASSERT( D3DGeometryBuffer::GetStorage().size() == 0 );
    ASSERT( D3DTextureBuffer2D::GetStorage().size() == 0 );
    ASSERT( D3DTextureBuffer3D::GetStorage().size() == 0 );
    ASSERT( D3DStructuredBuffer::GetStorage().size() == 0 );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//
// there is a scheme of specialized render pipeline
// this render pipeline a bit hardcoded - this marked
//
// set default viewport, camera mats, rs state, ds state blend state
//        |
// clear main RT and DS
//        |
// draw all geometry to shadowMap with a first scene light (sun for now)
//        |
// draw all geometry to g-buffer
//        |
// if voxel cone tracing enabled
//        |         | true
//        |         --> if voxelization needs --> voxelize static scene geometry, gen octree and opactiy brick buffer
//        |                  |
//        |             if light buffer needs to be rebuild --> rebuild light brick buffer for voxel cone tracing
//        |                  |
//        |             render voxel cone tracing texture using octree, light brick buffer, g-buffer normal, depth
//        |                  |
//        |             upscale voxel cone tracing texture using depth
//        |         ---------|
//        |         |
// switch by render output
//        |
//        | RO_COLOR
//        |-------------> g-buffer combine pass with single light, shadow map and voxel cone tracing texture (if enabled)
//        |
//        | RO_INDIRECT
//        |-------------> if vct enabled --> draw voxel cone tracing texture only (color or AO)
//        |
//        | RO_BRICKS
//        |-------------> if vct enabled --> draw bricks buffers
//        |
//        | RO_VOXELS
//        |-------------> if vct enabled --> draw scene voxels
//        |
//     draw UI
//        |
//  send sync query
//        |
//     present
//        |
// wait for previous frame
//
void D3DRenderer::RenderTick( )
{
    if ( mLastPresentResult == DXGI_STATUS_OCCLUDED )
        mLastPresentResult = mSwapChain->Present( 1, DXGI_PRESENT_TEST ); // check minimized window

    bool ready = mSwapChain != nullptr && md3dDevice != nullptr && mImmediateContext != nullptr
        && mLastPresentResult != DXGI_STATUS_OCCLUDED;

    if ( !ready )
    {
        ClearScene( );
        return;
    }

    SetDefaultViewport( );
    SetDefaultMats();

    mImmediateContext->RSSetState( mCullRS );
    mImmediateContext->OMSetDepthStencilState( mDepthNoStencilDS, 0 );
    mImmediateContext->OMSetBlendState( NULL, 0, 0xffffffff );

    // render sun shadow map
    float dLength = 0.0f;
    DirectX::XMStoreFloat( &dLength, DirectX::XMVector3Length( 
        DirectX::XMVectorSubtract( DirectX::XMLoadFloat3( &mStaticSceneBB.second ),
        DirectX::XMLoadFloat3( &mStaticSceneBB.first ) ) ) );
    mShadowMapper.Draw( mGeometryToRender, mLightToRender[0], std::make_pair( dLength * 0.7f, dLength * 0.7f ) );

    // render g-buffer
    mGBuffer.DrawGBuffer( mGeometryToRender );

    mImmediateContext->OMSetDepthStencilState( mNoDepthNoStencilDS, 0 );

    // voxelize scene and generate irradiance brick buffer
    Settings &settings = Settings::Get( );
    if ( mGIEnabled && settings.mVCTEnable )
    {
        if ( mVCT.NeedsVoxelization( ) )
        {
            mImmediateContext->RSSetState( mNoCullRS );
            mVCT.VoxelizeStaticScene( mGeometryToRender );
            SetDefaultViewport( );

            mImmediateContext->RSSetState( mCullRS );
        }
        if ( mVCT.NeedsProcessLight( mLightToRender[0] ) )
        {
            mVCT.ClearIrradianceBrickBuffer( ); // clear previous light information
            mVCT.ProcessShadowMap( mLightToRender[0], mShadowMapper.GetShadowMap( ) );
        }
        if ( mVCT.IsReady() )
        {
            mVCT.VoxelConeTracing( );
        }
    }

    // output
    switch ( settings.mRenderOutput )
    {
    case RenderOutput::RO_COLOR:
        {
            if ( mShadowMapper.IsReady( ) && mGBuffer.IsReady() )
            {
                ShadowMap &smap = mShadowMapper.GetShadowMap( );
                mGBuffer.DrawCombine( mLightToRender[0], smap );
            }
        }
        break;
    case RenderOutput::RO_INDIRECT:
        if ( mGIEnabled )
        {
            std::shared_ptr<Material> mat = std::make_shared<Material>( );
            mat->tex0 = mVCT.GetIndirectIrradiance();
            SceneGeometry obj( "Irradiance", mQuad, mat );
            SetFullscreenQuadMats( );
            mDefaultShader.Draw( mMainRT, mMainDepth, obj, settings.mShowAO );
        }
        break;
    case RenderOutput::RO_BRICKS:
    case RenderOutput::RO_VOXELS:
        if ( mGIEnabled )
        {
            ID3D11RenderTargetView* tmpRT = mMainRT->GetRTV();
            mImmediateContext->ClearRenderTargetView( tmpRT, DirectX::Colors::AliceBlue );
            mImmediateContext->OMSetDepthStencilState( mDepthNoStencilDS, 0 );
            mImmediateContext->ClearDepthStencilView( mMainDepth->GetDSV(), D3D11_CLEAR_DEPTH | D3D11_CLEAR_STENCIL, 1.0f, 0 );
            mImmediateContext->OMSetRenderTargets( 1, &tmpRT, mMainDepth->GetDSV( ) );
            mVCT.DrawBuffers( settings.mRenderOutput == RenderOutput::RO_VOXELS );
        }
        break;
    default:
        ASSERT( false );
        break;
    }

    mUIDrawer.Draw();

    SendSyncQuery();
    mLastPresentResult = mSwapChain->Present( 1, 0 );
    ASSERT( mLastPresentResult == S_OK || mLastPresentResult == DXGI_STATUS_OCCLUDED );
    SyncFence(); // wait for previous frame

    ClearScene();

    mFirstFrame = false;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::SetDefaultViewport( )
{
    if ( ( curVP.Width != ( FLOAT )mWidth ) || ( curVP.Height != ( FLOAT )mHeight ) ||
        ( curVP.MinDepth != 0.0f ) || ( curVP.MaxDepth != 1.0f ) ||
        ( curVP.TopLeftX != 0.0f ) || ( curVP.TopLeftY != 0.0f ) )
    {
        curVP.Width = ( FLOAT )mWidth;
        curVP.Height = ( FLOAT )mHeight;
        curVP.MinDepth = 0.0f;
        curVP.MaxDepth = 1.0f;
        curVP.TopLeftX = 0;
        curVP.TopLeftY = 0;
        mImmediateContext->RSSetViewports( 1, &curVP );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::SetIndirectLayout( )
{
    // set layout for indirect drawcall ( DrawInstancedIndirect )
    mImmediateContext->IASetInputLayout( nullptr );
    mImmediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_POINTLIST );
    mImmediateContext->IASetVertexBuffers( 0, 0, nullptr, nullptr, nullptr );
    mImmediateContext->IASetIndexBuffer( nullptr, DXGI_FORMAT_R32_UINT, 0 );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void D3DRenderer::SetViewport( float w, float h, float minD, float maxD, float topLeftX, float topLeftY )
{
    if ( ( curVP.Width != w ) || ( curVP.Height != h ) ||
        ( curVP.MinDepth != minD ) || ( curVP.MaxDepth = maxD ) ||
        ( curVP.TopLeftX != topLeftX ) || ( curVP.TopLeftY != topLeftY ) )
    {
        curVP.Width = w;
        curVP.Height = h;
        curVP.MinDepth = 0.0f;
        curVP.MaxDepth = 1.0f;
        curVP.TopLeftX = 0;
        curVP.TopLeftY = 0;
        mImmediateContext->RSSetViewports( 1, &curVP );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::SetViewTransform( const DirectX::XMFLOAT4X4 &view )
{
    mDefaultView = view;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::PushSceneGeometryToRender( const SceneGeometry &geometry )
{
    // NOTE place for batching
    mGeometryToRender.push_back( geometry );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::PushLigthToRender( const LightSource &light )
{
    mLightToRender.push_back( light );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::DrawGeometry( const std::shared_ptr<D3DGeometryBuffer> &geom )
{
    UINT stride = sizeof( Vertex3F3F3F2F ); // does Vertex3F3F3F2F will be everywhere?
    UINT offset = 0;

    if ( geom )
    {
        auto vb = geom->GetVB();
        mImmediateContext->IASetVertexBuffers( 0, 1, &vb, &stride, &offset );
        mImmediateContext->IASetIndexBuffer( geom->GetIB(), DXGI_FORMAT_R32_UINT, 0 );
        mImmediateContext->DrawIndexed( geom->GetIndexCount(), 0, 0 );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT D3DRenderer::CreateEffect( const char *shaderName, ID3DX11Effect **fx )
{
    Settings &settings = Settings::Get();
    std::string fn = std::string( settings.mShaderDir ) + std::string( shaderName );

    HRESULT hr;
    std::ifstream fin( fn, std::ios::binary );
    fin.seekg( 0, std::ios_base::end );
    int size = ( int )fin.tellg( );
    ASSERT( size > 0 );
    if ( size <= 0 )
        return S_FALSE;

    fin.seekg( 0, std::ios_base::beg );
    std::vector<char> compiledShaderFile( size );
    fin.read( &compiledShaderFile[0], size );
    fin.close( );

    // use D3D10_SHADER_DEBUG | D3D10_SHADER_SKIP_OPTIMIZATION if needed
    hr = D3DX11CreateEffectFromMemory( &compiledShaderFile[0], size, 0, md3dDevice, fx );
    ASSERT( hr == S_OK );

    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11Device* D3DRenderer::GetDevice( )
{
    ASSERT( md3dDevice != nullptr );
    return md3dDevice;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11DeviceContext* D3DRenderer::GetContext( )
{
    ASSERT( mImmediateContext != nullptr );
    return mImmediateContext;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::GetResolution( int &width, int &height )
{
    width = mWidth;
    height = mHeight;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int D3DRenderer::GetWidth( )
{
    return mWidth;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int D3DRenderer::GetHeight( )
{
    return mHeight;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3> D3DRenderer::GetStaticSceneBB( )
{
    return mStaticSceneBB;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3> D3DRenderer::GetStaticSceneBBRect( )
{
    float maxSide = max( max( mStaticSceneBB.second.x - mStaticSceneBB.first.x,
        mStaticSceneBB.second.y - mStaticSceneBB.first.y ),
        mStaticSceneBB.second.z - mStaticSceneBB.first.z );

    ASSERT( maxSide > 0.0f );
    DirectX::XMFLOAT3 cubeVertex = DirectX::XMFLOAT3( mStaticSceneBB.first.x + maxSide,
        mStaticSceneBB.first.y + maxSide, 
        mStaticSceneBB.first.z + maxSide );

    return std::make_pair( mStaticSceneBB.first, cubeVertex );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DTextureBuffer2D> D3DRenderer::GetDefaultTexture( )
{
    return mDefaultTexture;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DTextureBuffer2D> D3DRenderer::GetMainRT( )
{
    return mMainRT;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DTextureBuffer2D> D3DRenderer::GetMainDepth( )
{
    return mMainDepth;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11InputLayout* D3DRenderer::GetDefaultInputLayout( )
{
    ASSERT( mDefaultShader.IsReady() );
    return mDefaultShader.GetDefaultInputLayout( );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::GetViewProjMats( DirectX::XMFLOAT4X4 &view, DirectX::XMFLOAT4X4 &proj )
{
    view = mView;
    proj = mProj;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::GetFullscreenQuadMats( DirectX::XMFLOAT4X4 &view, DirectX::XMFLOAT4X4 &proj )
{
    view = mFullscreenQuadView;
    proj = mFullscreenQuadProj;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<Material> D3DRenderer::GetDefaultMaterial( )
{
    return mDefaultMaterial;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DGeometryBuffer> D3DRenderer::GetQuad( )
{
    return mQuad;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
GBuffer& D3DRenderer::GetGBuffer( )
{
    ASSERT( mGBuffer.IsReady() );
    return mGBuffer;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
VCT& D3DRenderer::GetVCT( )
{
    ASSERT( mVCT.IsReady( ) );
    return mVCT;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
UIDrawer& D3DRenderer::GetUIDrawer( )
{
    ASSERT( mUIDrawer.IsReady( ) );
    return mUIDrawer;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Blur& D3DRenderer::GetBlur()
{
    ASSERT( mBlur.IsReady( ) );
    return mBlur;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint32_t D3DRenderer::GetValueFromCounter( std::shared_ptr<D3DStructuredBuffer> &buffer )
{
    ID3D11UnorderedAccessView *counter = buffer->GetUAV();
    ASSERT( counter != nullptr && mCopyCounterBuffer != nullptr );
    if ( counter == nullptr || mCopyCounterBuffer == nullptr )
        return 0;

    // performance hit: return value immediately, can stall GPU
    mImmediateContext->CopyStructureCount( mCopyCounterBuffer, 0, counter );

    D3D11_MAPPED_SUBRESOURCE subRes;
    HRESULT hr = mImmediateContext->Map( mCopyCounterBuffer, 0, D3D11_MAP_READ, 0, &subRes );
    ASSERT( hr == S_OK );
    if ( hr == S_OK )
    {
        uint32_t counterValue = static_cast<uint32_t*>( subRes.pData )[0];
        mImmediateContext->Unmap( mCopyCounterBuffer, 0 );
        return counterValue;
    }
    return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool D3DRenderer::IsGIEnabled()
{
    return mGIEnabled;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3DRenderer::D3DRenderer():
    mDriverType( D3D_DRIVER_TYPE_NULL ),
    mFeatureLevel( D3D_FEATURE_LEVEL_11_0 ),
    md3dDevice( nullptr ),
    md3dDevice1( nullptr ),
    mImmediateContext( nullptr ),
    mImmediateContext1( nullptr ),
    mSwapChain( nullptr ),
    mSwapChain1( nullptr ),

    mNoCullRS( nullptr ),
    mCullRS( nullptr ),
    mNoDepthNoStencilDS( nullptr ),
    mDepthNoStencilDS( nullptr ),

    mCopyCounterBuffer( nullptr ),
    mSyncQueryA( nullptr ),
    mSyncQueryB( nullptr ),
    mFirstFrame( true ),

    mGIEnabled( true )
{
    // max value
    mStaticSceneBB.first = DirectX::XMFLOAT3( 999999.0f, 999999.0f, 999999.0f );
    mStaticSceneBB.second = DirectX::XMFLOAT3( -999999.0f, -999999.0f, -999999.0f );

    // get adapter info
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT D3DRenderer::CreateSwapChain( HWND hWnd, UINT width, UINT height )
{
    // Obtain DXGI factory from device (since we used nullptr for pAdapter above)
    IDXGIFactory1* dxgiFactory = nullptr;
    IDXGIDevice* dxgiDevice = nullptr;
    HRESULT hr = md3dDevice->QueryInterface( __uuidof( IDXGIDevice ), reinterpret_cast<void**>( &dxgiDevice ) );
    if ( SUCCEEDED( hr ) )
    {
        IDXGIAdapter* adapter = nullptr;
        hr = dxgiDevice->GetAdapter( &adapter );
        if ( SUCCEEDED( hr ) )
        {
            hr = adapter->GetParent( __uuidof( IDXGIFactory1 ), reinterpret_cast<void**>( &dxgiFactory ) );
            adapter->Release( );
        }
        dxgiDevice->Release( );
    }
    if ( !dxgiFactory )
        return false;

    IDXGIFactory2* dxgiFactory2 = nullptr;
    hr = dxgiFactory->QueryInterface( __uuidof( IDXGIFactory2 ), reinterpret_cast<void**>( &dxgiFactory2 ) );
    if ( dxgiFactory2 )
    {
        // DirectX 11.1 or later
        hr = md3dDevice->QueryInterface( __uuidof( ID3D11Device1 ), reinterpret_cast<void**>( &md3dDevice1 ) );
        if ( SUCCEEDED( hr ) )
        {
            ( void )mImmediateContext->QueryInterface( __uuidof( ID3D11DeviceContext1 ), reinterpret_cast<void**>( &mImmediateContext1 ) );
        }

        DXGI_SWAP_CHAIN_DESC1 sd;
        ZeroMemory( &sd, sizeof( sd ) );
        sd.Width = width;
        sd.Height = height;
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.BufferCount = 1;

        hr = dxgiFactory2->CreateSwapChainForHwnd( md3dDevice, hWnd, &sd, nullptr, nullptr, &mSwapChain1 );
        if ( SUCCEEDED( hr ) )
        {
            hr = mSwapChain1->QueryInterface( __uuidof( IDXGISwapChain ), reinterpret_cast<void**>( &mSwapChain ) );
        }

        dxgiFactory2->Release( );
    }
    else
    {
        // DirectX 11.0 systems
        DXGI_SWAP_CHAIN_DESC sd;
        ZeroMemory( &sd, sizeof( sd ) );
        sd.BufferCount = 1;
        sd.BufferDesc.Width = mWidth;
        sd.BufferDesc.Height = mHeight;
        sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        sd.BufferDesc.RefreshRate.Numerator = 60;
        sd.BufferDesc.RefreshRate.Denominator = 1;
        sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        sd.OutputWindow = hWnd;
        sd.SampleDesc.Count = 1;
        sd.SampleDesc.Quality = 0;
        sd.Windowed = TRUE;

        hr = dxgiFactory->CreateSwapChain( md3dDevice, &sd, &mSwapChain );
    }

    // dxgiFactory->MakeWindowAssociation( hWnd, DXGI_MWA_NO_ALT_ENTER ); // disable fullscreen and ALT+ENTER shortcut

    dxgiFactory->Release( );

    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT D3DRenderer::CreateDevice( D3D_FEATURE_LEVEL *featureLevels, UINT numFeatureLevels )
{
    UINT createDeviceFlags = 0;
#ifdef _DEBUG
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif

    D3D_DRIVER_TYPE driverTypes[] =
    {
        D3D_DRIVER_TYPE_HARDWARE,
        D3D_DRIVER_TYPE_WARP,
        D3D_DRIVER_TYPE_REFERENCE,
    };
    UINT numDriverTypes = ARRAYSIZE( driverTypes );

    HRESULT hr = S_FALSE;
    for ( UINT driverTypeIndex = 0; driverTypeIndex < numDriverTypes; driverTypeIndex++ )
    {
        mDriverType = driverTypes[driverTypeIndex];
        hr = D3D11CreateDevice( nullptr, mDriverType, nullptr, createDeviceFlags, featureLevels, numFeatureLevels,
            D3D11_SDK_VERSION, &md3dDevice, &mFeatureLevel, &mImmediateContext );

        if ( SUCCEEDED( hr ) )
            break;
    }

    return hr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::ResetView()
{
    DirectX::XMMATRIX I = DirectX::XMMatrixIdentity( );
    DirectX::XMStoreFloat4x4( &mWorld, I );
    DirectX::XMStoreFloat4x4( &mView, I );
    DirectX::XMStoreFloat4x4( &mProj, I );

    DirectX::XMMATRIX P = DirectX::XMMatrixPerspectiveFovRH( 0.25f * DirectX::XM_PI, static_cast<float>( mWidth ) / mHeight, 1.0f, 10000.0f );
    DirectX::XMStoreFloat4x4( &mDefaultProj, P );

    DirectX::XMMATRIX Vortho = DirectX::XMMatrixSet( 1.0, 0.0, 0.0, 0.0,
                                                    0.0, 0.0, -1.0, 0.0,
                                                    0.0, -1.0, 0.0, 0.0,
                                                    0.0, 0.0, -1.0, 1.0 );
    DirectX::XMStoreFloat4x4( &mFullscreenQuadView, Vortho );

    DirectX::XMMATRIX Portho = DirectX::XMMatrixOrthographicRH( 1.0f, 1.0f, 0.0f, 1000.0f );
    DirectX::XMStoreFloat4x4( &mFullscreenQuadProj, Portho );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::ClearScene()
{
    // after present - remove scene and wait for new objects
    mGeometryToRender.clear( );
    mLightToRender.clear( );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::CreateDefaultTexture( )
{
    ASSERT( mSwapChain != nullptr && md3dDevice != nullptr && mImmediateContext != nullptr );
    mDefaultTexture = D3DTextureBuffer2D::CreateDefault();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::CreateDefaultMaterial( )
{
    mDefaultMaterial = std::make_shared<Material>( );
    mDefaultMaterial->tex0 = mDefaultTexture;
    mDefaultMaterial->tex1 = mDefaultTexture;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::CreateDefaultGeometry()
{
    GGMeshData generatedMesh;
    GeometryGenerator::GeneratePlane( 1.0f, 1.0f, 2, 2, generatedMesh );
    mQuad = D3DGeometryBuffer::Create( generatedMesh );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool D3DRenderer::CreateCopyCounterBuffer( )
{
    D3D11_BUFFER_DESC copyBd = D3DStructuredBuffer::GenBufferDesc(
        D3D11_USAGE_STAGING, sizeof( uint32_t ), 0, D3D11_CPU_ACCESS_READ, 0, sizeof( uint32_t ) );

    HRESULT hr = md3dDevice->CreateBuffer( &copyBd, 0, &mCopyCounterBuffer );
    ASSERT( hr == S_OK );

    return hr == S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool D3DRenderer::CreateSyncQueries()
{
    D3D11_QUERY_DESC queryDesc;
    queryDesc.Query = D3D11_QUERY_EVENT;
    queryDesc.MiscFlags = 0;

    HRESULT hr = md3dDevice->CreateQuery( &queryDesc, &mSyncQueryA );
    ASSERT( hr == S_OK );
    if ( hr != S_OK )
        return false;

    hr = md3dDevice->CreateQuery( &queryDesc, &mSyncQueryB );
    ASSERT( hr == S_OK );

    return hr == S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::SetFullscreenQuadMats()
{
    mView = mFullscreenQuadView;
    mProj = mFullscreenQuadProj;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::SetDefaultMats()
{
    mView = mDefaultView;
    mProj = mDefaultProj;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::CalcStaticSceneBB( const DirectX::XMFLOAT3 &vtx )
{
    auto calcBB = []( const float &v, float &BBvalue, bool minBB )
    {
        // TODO temp test - ignore VCT corner cases
        float value = 0.0f;
        if ( minBB )
            value = v - 1.5f;
        else
            value = v + 1.5f;

        if ( minBB && value < BBvalue || !minBB && value > BBvalue )
            BBvalue = value;
    };

    calcBB( vtx.x, mStaticSceneBB.first.x, true );
    calcBB( vtx.y, mStaticSceneBB.first.y, true );
    calcBB( vtx.z, mStaticSceneBB.first.z, true );
    calcBB( vtx.x, mStaticSceneBB.second.x, false );
    calcBB( vtx.y, mStaticSceneBB.second.y, false );
    calcBB( vtx.z, mStaticSceneBB.second.z, false );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::ReportLiveObjects()
{
    if ( !md3dDevice )
        return;

    ID3D11Debug *debugDevice;
    HRESULT hr = md3dDevice->QueryInterface( __uuidof( ID3D11Debug ), reinterpret_cast<void**>(&debugDevice) );
    if ( hr == S_OK )
    {
        debugDevice->ReportLiveDeviceObjects( D3D11_RLDO_DETAIL );
    }

    COMSafeRelease( debugDevice );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::SendSyncQuery()
{
    std::swap( mSyncQueryA, mSyncQueryB );

    mImmediateContext->End( mSyncQueryA );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DRenderer::SyncFence()
{
    if ( !mFirstFrame )
    {
        // wait for previous frame
        UINT32 queryData;
        while ( S_OK != mImmediateContext->GetData( mSyncQueryB, &queryData, sizeof( UINT32 ), 0 ) )
        {
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
