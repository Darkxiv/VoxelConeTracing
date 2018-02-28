#include <VCT.h>
#include <GlobalUtils.h>
#include <imgui.h>
#include <D3DTextureBuffer2D.h>
#include <D3DTextureBuffer3D.h>
#include <D3DStructuredBuffer.h>
#include <D3DRenderer.h>
#include <d3dx11effect.h>
#include <directxcolors.h>
#include <SceneGeometry.h>
#include <Material.h>
#include <Settings.h>
#include <Light.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool VCT::Init( )
{
    mOctree.Init( );

    // init DefferedVoxelThread
    size_t voxelSize = 4; // uint position, uint color, uint normal, uint pad
    size_t numElem = 1024 * 1024;
    D3D11_BUFFER_DESC defferedFragBD = D3DStructuredBuffer::GenBufferDesc( D3D11_USAGE_DEFAULT, sizeof( int )* voxelSize * numElem,
        D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE, 0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, sizeof( int )* voxelSize );
    D3D11_UNORDERED_ACCESS_VIEW_DESC defferedFragUAVDesc = D3DStructuredBuffer::GenUAVDesc( 0, numElem, D3D11_BUFFER_UAV_FLAG_COUNTER, D3D11_UAV_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN );

    D3D11_SHADER_RESOURCE_VIEW_DESC defferedFragSRVDesc;
    defferedFragSRVDesc.Format = DXGI_FORMAT_UNKNOWN;
    defferedFragSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_BUFFEREX;
    defferedFragSRVDesc.BufferEx.FirstElement = 0;
    defferedFragSRVDesc.BufferEx.NumElements = numElem;
    defferedFragSRVDesc.BufferEx.Flags = 0;

    Settings &settings = Settings::Get( );
    D3DRenderer &renderer = D3DRenderer::Get( );

    mVoxelArray = D3DStructuredBuffer::CreateBuffer( true, true, &defferedFragBD, nullptr, &defferedFragSRVDesc, &defferedFragUAVDesc );

    mBrickBufferSize = settings.mBrickBufferRes;
    D3D11_TEXTURE3D_DESC brickBufferDesc;
    brickBufferDesc.Width = mBrickBufferSize;
    brickBufferDesc.Height = mBrickBufferSize;
    brickBufferDesc.Depth = mBrickBufferSize;
    brickBufferDesc.MipLevels = 1;
    brickBufferDesc.Usage = D3D11_USAGE_DEFAULT;
    brickBufferDesc.Format = DXGI_FORMAT_R8G8B8A8_TYPELESS;
    brickBufferDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    brickBufferDesc.CPUAccessFlags = 0;
    brickBufferDesc.MiscFlags = 0;

    D3D11_SHADER_RESOURCE_VIEW_DESC brickBufferSRVDesc;
    brickBufferSRVDesc.Texture3D.MipLevels = 1;
    brickBufferSRVDesc.Texture3D.MostDetailedMip = 0;
    brickBufferSRVDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    brickBufferSRVDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE3D;

    D3D11_UNORDERED_ACCESS_VIEW_DESC brickBufferUAVDesc;
    brickBufferUAVDesc.Texture3D.FirstWSlice = 0;
    brickBufferUAVDesc.Texture3D.MipSlice = 0;
    brickBufferUAVDesc.Texture3D.WSize = mBrickBufferSize;
    brickBufferUAVDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE3D;
    brickBufferUAVDesc.Format = DXGI_FORMAT_R32_UINT;

    mOpacityBrickBuffer = D3DTextureBuffer3D::Create( true, true, &brickBufferDesc, &brickBufferSRVDesc, &brickBufferUAVDesc );
    mIrradianceBrickBuffer = D3DTextureBuffer3D::Create( true, true, &brickBufferDesc, &brickBufferSRVDesc, &brickBufferUAVDesc );

    // init buffer for indirect draw calls
    size_t indirectBufferSize = 4 + mOctree.mHeight * 4;
    D3D11_BUFFER_DESC indirectBufferBD = D3DStructuredBuffer::GenBufferDesc( D3D11_USAGE_DEFAULT, sizeof( int )* indirectBufferSize,
        D3D11_BIND_UNORDERED_ACCESS, 0, D3D11_RESOURCE_MISC_DRAWINDIRECT_ARGS, 0 );

    D3D11_UNORDERED_ACCESS_VIEW_DESC indirectBufferUAVDesc =
        D3DStructuredBuffer::GenUAVDesc( 0, indirectBufferSize, 0, D3D11_UAV_DIMENSION_BUFFER, DXGI_FORMAT_R32_UINT );

    // see indirectBufferUtils.fx
    UINT *initialValue = new UINT[indirectBufferSize];
    memset( initialValue, 0, sizeof( int )* indirectBufferSize );
    for ( size_t i = 0; i < indirectBufferSize / 4; i++ )
        initialValue[i * 4 + 1] = 1; // instances count

    if ( mOctree.mHeight > 0 )
        initialValue[4 + 0] = 1; // nodes count for octree level 0

    D3D11_SUBRESOURCE_DATA subRes = D3DStructuredBuffer::GenSubresourceData( 0, 0, initialValue );
    mIndirectDrawBuffer = D3DStructuredBuffer::CreateBuffer( false, true, &indirectBufferBD, &subRes, nullptr, &indirectBufferUAVDesc );
    delete[] initialValue;

    // init final voxel cone tracing texs
    mIndirectIrradianceSmall = 
        D3DTextureBuffer2D::Create( true, false, true, false, 0, 0, 0, 0, 0.0f, settings.mVCTConeTracingRes, settings.mVCTConeTracingRes );

    mIndirectIrradianceBig = D3DTextureBuffer2D::Create( true, false, true, false, 0, 0, 0, 0, 1.0f, 0, 0 );
    mIndirectIrradianceBigTmp = D3DTextureBuffer2D::Create( true, false, true, false, 0, 0, 0, 0, 1.0f, 0, 0 );

    // should be max shadow resolution size
    mProcessShadowRT = 
        D3DTextureBuffer2D::Create( true, false, false, false, 0, 0, 0, 0, 1.0f, settings.mShadowMapRes, settings.mShadowMapRes );

    mIsReady = mfxGenOctree.Load();
    mIsReady &= mfxGenBrickBuffer.Load( );
    mIsReady &= mfxConeTracing.Load( );

    ASSERT( mIsReady );

    return mIsReady;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VCT::Clear()
{
    mOctree.Clear( );

    mVoxelArray.reset();
    mIndirectDrawBuffer.reset();
    mOpacityBrickBuffer.reset();
    mIrradianceBrickBuffer.reset();
    mIndirectIrradianceSmall.reset();
    mIndirectIrradianceBig.reset();
    mIndirectIrradianceBigTmp.reset();
    mProcessShadowRT.reset();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool VCT::IsReady( )
{
    return mIsReady;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool VCT::NeedsVoxelization()
{
    return mNeedsVoxelization;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool VCT::NeedsProcessLight( const LightSource &lsource )
{
    return mProcessedLight != lsource;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VCT::VoxelizeStaticScene( const std::vector<SceneGeometry> &objs )
{
    if ( !mIsReady )
        return;

    // setup fx
    mfxGenOctree.SetupViewProj( );
    mfxGenOctree.BindVCTResources( *this ); // should replace by bind vct resources in fx

    D3DRenderer &renderer = D3DRenderer::Get( );
    auto immediateContext = renderer.GetContext( );
    UINT tmpClearValue[4] = { 0, 0, 0, 0 };
    ID3D11UnorderedAccessView *uav = mVoxelArray->GetUAV();
    immediateContext->ClearUnorderedAccessViewUint( uav, tmpClearValue );
    immediateContext->OMSetRenderTargetsAndUnorderedAccessViews( 0, nullptr, nullptr, 0, 1, &uav, tmpClearValue ); // clear counters - effect11 lack

    mOctree.ClearOctree( );

    ID3D11RenderTargetView *mainRTV = renderer.GetMainRT( )->GetRTV();
    immediateContext->OMSetRenderTargets( 1, &mainRTV, nullptr );

    renderer.SetViewport( static_cast<float>( mOctree.mResolution ), static_cast<float>( mOctree.mResolution ), 0, 1.0f, 0, 0 );

    immediateContext->IASetInputLayout( renderer.GetDefaultInputLayout() );
    immediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    // voxelize scene
    for each ( auto &obj in objs )
    {
        const std::shared_ptr<Material> &mat = obj.mMaterial;
        if ( mat->tex0 )
            mfxGenOctree.mfxAlbedoTexture->SetResource( mat->tex0->GetSRV( ) );
        else
            mfxGenOctree.mfxAlbedoTexture->SetResource( renderer.GetDefaultTexture( )->GetSRV( ) );

        if ( mat->tex1 )
        {
            mfxGenOctree.mfxUseNormalMap->SetBool( true ); // performance hint: should be batched
            mfxGenOctree.mfxNormalTexture->SetResource( mat->tex1->GetSRV() );
        }
        else
        {
            mfxGenOctree.mfxUseNormalMap->SetBool( false );
        }

        mfxGenOctree.mGenOctree.mCreateVoxelArray->Apply( 0, immediateContext );
        renderer.DrawGeometry( obj.mGeometryBuffer );
    }

    // clear
    mfxGenOctree.mfxVoxelArrayRW->SetUnorderedAccessView( nullptr );
    mfxGenOctree.mGenOctree.mCreateVoxelArray->Apply( 0, immediateContext );

    renderer.SetIndirectLayout( );
    ID3D11Buffer *indirectBuffer = GetIndirectDrawBuffer( )->GetBuffer();

    // generate octree
    size_t octreeHeight = mOctree.mHeight;
    for ( size_t octreeLevel = 0; octreeLevel < octreeHeight - 1; octreeLevel++ )
    {
        mfxGenOctree.mfxCurrentOctreeLevel->SetInt( octreeLevel );

        mfxGenOctree.mGenOctree.mFlagNodes->Apply( 0, immediateContext );
        immediateContext->DrawInstancedIndirect( indirectBuffer, 0 ); // can save extra gpu time if run Draw( 1, 0 ) for 0 octree level

        // 16 bytes is the size of indirect buffer, first 16 bytes - for voxels count
        UINT indirectBufferOffset = GetIndirectBufferOffset( octreeLevel );

        mfxGenOctree.mGenOctree.mSubdivideNodes->Apply( 0, immediateContext );
        immediateContext->DrawInstancedIndirect( indirectBuffer, indirectBufferOffset );

        mfxGenOctree.mGenOctree.mConnectNeighbors->Apply( 0, immediateContext );
        immediateContext->DrawInstancedIndirect( indirectBuffer, indirectBufferOffset );
    }

    mfxGenOctree.mGenOctree.mConnectNodesToVoxels->Apply( 0, immediateContext );
    immediateContext->DrawInstancedIndirect( indirectBuffer, 0 );

    // now we get octree and we can build brick buffer
    // performance hit: return value immediately, can stall GPU
    uint32_t nodesPackCount = renderer.GetValueFromCounter( mOctree.mNodesPackCounter );
    mOctree.mNodesCount = nodesPackCount * 8 + 1; // 8 - node childs count, 1 is for preallocated root node

    GenOpacityBrickBuffer( );

    mNeedsVoxelization = false;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VCT::ClearIrradianceBrickBuffer()
{
    if ( !mIsReady )
        return;

    D3DRenderer &renderer = D3DRenderer::Get();
    auto context = renderer.GetContext( );

    // clear brick buffer texture
    UINT tmpClearValue[4] = { 0, 0, 0, 0 };
    ID3D11UnorderedAccessView *uav = mIrradianceBrickBuffer->GetUAV();
    if ( uav )
        context->ClearUnorderedAccessViewUint( uav, tmpClearValue );

    // clear octree light information
    mfxGenBrickBuffer.mOctreeVariables.BindOctree( mOctree );

    renderer.SetIndirectLayout( );
    mfxGenBrickBuffer.mProcessingShadowMap.mResetOctreeFlags->Apply( 0, context );
    context->Draw( mOctree.mNodesCount, 0 );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VCT::ProcessShadowMap( LightSource &lsource, ShadowMap &shadowMap )
{
    if ( !mIsReady || NeedsVoxelization( ) || !shadowMap.mShadowTexture )
        return;

    D3DRenderer &renderer = D3DRenderer::Get( );
    auto context = renderer.GetContext();
    ID3D11RenderTargetView *rt = mProcessShadowRT->GetRTV();
    context->ClearRenderTargetView( rt, DirectX::Colors::AliceBlue );
    context->OMSetRenderTargets( 1, &rt, nullptr );

    DirectX::XMFLOAT4X4 view, proj;
    renderer.GetFullscreenQuadMats( view, proj );
    DirectX::XMMATRIX worldViewProj = DirectX::XMLoadFloat4x4( &view ) * DirectX::XMLoadFloat4x4( &proj );
    mfxGenBrickBuffer.mfxWorldViewProj->SetMatrix( reinterpret_cast< float* >( &worldViewProj ) );

    DirectX::XMMATRIX iProj = DirectX::XMMatrixInverse( nullptr, DirectX::XMLoadFloat4x4( &shadowMap.mProj ) );
    DirectX::XMMATRIX iView = DirectX::XMMatrixInverse( nullptr, DirectX::XMLoadFloat4x4( &shadowMap.mView ) );
    mfxGenBrickBuffer.mfxShadowInverseProj->SetMatrix( reinterpret_cast< float* >( &iProj ) );
    mfxGenBrickBuffer.mfxShadowInverseView->SetMatrix( reinterpret_cast< float* >( &iView ) );
    // get resolution from shadowMap

    DirectX::XMFLOAT3 &lColor = lsource.color;
    DirectX::XMFLOAT3 &lPos = lsource.position;
    DirectX::XMFLOAT3 &lDir = lsource.direction;
    DirectX::XMVECTOR vlColorRadius = DirectX::XMVectorSet( lColor.x, lColor.y, lColor.z, lsource.radius );
    DirectX::XMVECTOR vlPos = DirectX::XMVectorSet( lPos.x, lPos.y, lPos.z, 1.0f );
    DirectX::XMVECTOR vlDir = DirectX::XMVectorSet( lDir.x, lDir.y, lDir.z, 1.0f );
    mfxGenBrickBuffer.mfxLightColorRadius->SetFloatVector( reinterpret_cast< float* >( &vlColorRadius ) );
    mfxGenBrickBuffer.mfxLightPosition->SetFloatVector( reinterpret_cast< float* >( &vlPos ) );
    mfxGenBrickBuffer.mfxLightDirection->SetFloatVector( reinterpret_cast< float* >( &lDir ) );

    mfxGenBrickBuffer.BindVCTResources( *this );

    auto &shadowTex = shadowMap.mShadowTexture;
    const int shadowRes[] = { shadowTex->GetWidth( ), shadowTex->GetHeight( ) };
    mfxGenBrickBuffer.mfxShadowMapResolution->SetIntVector( shadowRes );
    renderer.SetViewport( static_cast< float >( shadowRes[0] ), static_cast< float >( shadowRes[1] ), 0.0f, 1.0f, 0, 0 );

    auto immediateContext = renderer.GetContext( );
    immediateContext->IASetInputLayout( renderer.GetDefaultInputLayout( ) );
    immediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    mfxGenBrickBuffer.mfxShadowMap->SetResource( shadowTex->GetSRV() );

    mfxGenBrickBuffer.mProcessingShadowMap.mProcessPass->Apply( 0, immediateContext );

    renderer.DrawGeometry( renderer.GetQuad( ) );

    mfxGenBrickBuffer.mfxShadowMap->SetResource( nullptr );
    mfxGenBrickBuffer.mProcessingShadowMap.mProcessPass->Apply( 0, immediateContext );

    GenRadianceBrickBuffer( mIrradianceBrickBuffer );

    renderer.SetDefaultViewport();

    // TODO write more general way to clear effect11 pipeline!
    mfxGenBrickBuffer.mOctreeVariables.mfxOctreeR->SetResource( nullptr );
    mfxGenBrickBuffer.mOctreeVariables.mfxOctreeRW->SetUnorderedAccessView( nullptr );
    mfxGenBrickBuffer.mfxBrickBufferRW->SetUnorderedAccessView( nullptr );
    mfxGenBrickBuffer.mProcessingShadowMap.mAverageLitNodeValues->Apply( 0, immediateContext );

    mProcessedLight = lsource;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VCT::VoxelConeTracing( )
{
    if ( !mIsReady )
        return;

    // set viewport
    D3DRenderer &renderer = D3DRenderer::Get( );
    renderer.SetViewport( static_cast< float >( mIndirectIrradianceSmall->GetWidth( ) ), 
        static_cast< float >( mIndirectIrradianceSmall->GetHeight( ) ), 0.0f, 1.0f, 0, 0 );

    DirectX::XMFLOAT4X4 view, proj;
    renderer.GetFullscreenQuadMats( view, proj );
    DirectX::XMMATRIX worldViewProj = DirectX::XMLoadFloat4x4( &view ) * DirectX::XMLoadFloat4x4( &proj );
    mfxConeTracing.mfxWorldViewProj->SetMatrix( reinterpret_cast< float* >( &worldViewProj ) );

    GBuffer &gbuffer = renderer.GetGBuffer();
    gbuffer.GetSceneViewProj( view, proj );
    DirectX::XMMATRIX iProj = DirectX::XMMatrixInverse( nullptr, DirectX::XMLoadFloat4x4( &proj ) );
    DirectX::XMMATRIX iView = DirectX::XMMatrixInverse( nullptr, DirectX::XMLoadFloat4x4( &view ) );
    mfxConeTracing.mfxInverseProj->SetMatrix( reinterpret_cast< float* >( &iProj ) );
    mfxConeTracing.mfxInverseView->SetMatrix( reinterpret_cast< float* >( &iView ) );

    mfxConeTracing.BindVCTResources( *this );

    auto immediateContext = renderer.GetContext( );
    ID3D11RenderTargetView* tmpRT = mIndirectIrradianceSmall->GetRTV();
    immediateContext->ClearRenderTargetView( tmpRT, DirectX::Colors::Black );
    immediateContext->OMSetRenderTargets( 1, &tmpRT, nullptr );
    immediateContext->IASetInputLayout( renderer.GetDefaultInputLayout() );
    immediateContext->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    mfxConeTracing.mfxNormalTexture->SetResource( gbuffer.GetNormal()->GetSRV() );
    mfxConeTracing.mfxDepthTexture->SetResource( gbuffer.GetDepth()->GetSRV() );

    // cone tracing to small texture
    mfxConeTracing.mfxConeTracing->Apply( 0, immediateContext );
    renderer.DrawGeometry( renderer.GetQuad( ) );

    // upscale mIndirectIrradianceSmall texture considering depth
    renderer.SetDefaultViewport( );

    // TODO write more general way to clear effect11 pipeline!
    mfxConeTracing.mfxNormalTexture->SetResource( nullptr );
    mfxConeTracing.mfxDepthTexture->SetResource( nullptr );

    mfxConeTracing.mfxConeTracing->Apply( 0, immediateContext );

    auto &blur = renderer.GetBlur( );
    blur.UpscaleBlur( gbuffer.GetDepth( ), mIndirectIrradianceSmall,
        mIndirectIrradianceBigTmp, mIndirectIrradianceBig );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VCT::DrawBuffers( bool showVoxels )
{
    if ( !mIsReady )
        return;

    D3DRenderer &renderer = D3DRenderer::Get( );

    mfxGenBrickBuffer.BindVCTResources( *this );
    ID3D11ShaderResourceView *brickBufferSRV;

    switch ( Settings::Get( ).mVCTDebugBuffer )
    {
    case DBUF_IRRADIANCE:
        brickBufferSRV = mIrradianceBrickBuffer->GetSRV();
        break;
    case DBUF_OPACITY:
    default:
        brickBufferSRV = mOpacityBrickBuffer->GetSRV();
        break;
    }
    
    mfxGenBrickBuffer.mfxBrickBufferR->SetResource( brickBufferSRV );

    DirectX::XMFLOAT4X4 view, proj;
    renderer.GetViewProjMats( view, proj );
    DirectX::XMMATRIX worldViewProj = DirectX::XMLoadFloat4x4( &view ) * DirectX::XMLoadFloat4x4( &proj );
    mfxGenBrickBuffer.mfxWorldViewProj->SetMatrix( reinterpret_cast< float* >( &worldViewProj ) );

    renderer.SetIndirectLayout( );
    ID3D11Buffer *indirectBuffer = GetIndirectDrawBuffer( )->GetBuffer();
    size_t octreeLevel = GetDebugOctreeLevel( );
    if ( showVoxels )
        octreeLevel = mOctree.mHeight - 1;
    UINT indirectBufferOffset = ( 4 + 4 * octreeLevel ) * sizeof( int );

    auto immediateContext = renderer.GetContext( );
    auto tech = showVoxels ? mfxGenBrickBuffer.mGenBrickBuffer.mDebugVoxels : 
        mfxGenBrickBuffer.mGenBrickBuffer.mDebugBricks;
    tech->Apply( 0, immediateContext );

    immediateContext->DrawInstancedIndirect( indirectBuffer, indirectBufferOffset );

    mfxGenBrickBuffer.mfxVoxelArrayR->SetResource( nullptr );
    mfxGenBrickBuffer.mOctreeVariables.mfxOctreeR->SetResource( nullptr );
    mfxGenBrickBuffer.mOctreeVariables.mfxOctreeRW->SetUnorderedAccessView( nullptr );
    mfxGenBrickBuffer.mOctreeVariables.mfxNodesPackCounter->SetUnorderedAccessView( nullptr );

    tech->Apply( 0, immediateContext );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Octree& VCT::GetOctree( )
{
    return mOctree;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t VCT::GetFragmentListSize( )
{
    return mFragmentListSize;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DStructuredBuffer> VCT::GetIndirectDrawBuffer()
{
    return mIndirectDrawBuffer;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DStructuredBuffer> VCT::GetVoxelArray( )
{
    return mVoxelArray;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DTextureBuffer3D> VCT::GetOpacityBrickBuffer( )
{
    return mOpacityBrickBuffer;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DTextureBuffer3D> VCT::GetIrradianceBrickBuffer( )
{
    return mIrradianceBrickBuffer;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DTextureBuffer2D> VCT::GetIndirectIrradianceSmall( )
{
    return mIndirectIrradianceSmall;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DTextureBuffer2D> VCT::GetIndirectIrradiance( )
{
    return mIndirectIrradianceBig;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t VCT::GetBrickBufferSize( )
{
    return mBrickBufferSize;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float VCT::GetLambdaFalloff( )
{
    Settings &settings = Settings::Get();
    return settings.mVCTLambdaFalloff;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float VCT::GetLocalConeOffset( )
{
    Settings &settings = Settings::Get( );
    return settings.mVCTLocalConeOffset;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float VCT::GetWorldConeOffset( )
{
    Settings &settings = Settings::Get( );
    return settings.mVCTWorldConeOffset;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float VCT::GetIndirectInfluence()
{
    Settings &settings = Settings::Get( );
    return settings.mVCTIndirectAmplification;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float VCT::GetStepCorrection()
{
    Settings &settings = Settings::Get( );
    return settings.mVCTStepCorrection;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool VCT::GetUseOpacityBuffer()
{
    Settings &settings = Settings::Get( );
    return settings.mVCTUseOpacityBuffer;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int VCT::GetDebugOctreeLevel( )
{
    Settings &settings = Settings::Get( );
    return settings.mVCTDebugOctreeLevel;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool VCT::GetDebugView( )
{
    Settings &settings = Settings::Get( );
    return settings.mVCTDebugView;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int VCT::GetDebugConeDir( )
{
    Settings &settings = Settings::Get( );
    return settings.mVCTDebugConeDir;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int VCT::GetDebugOctreeFirstLevel( )
{
    Settings &settings = Settings::Get( );
    return settings.mVCTDebugOctreeFirstLevel;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int VCT::GetDebugOctreeLastLevel( )
{
    Settings &settings = Settings::Get( );
    return settings.mVCTDebugOctreeLastLevel;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
size_t VCT::GetIndirectBufferOffset( size_t level ) const
{
    ASSERT( level < mOctree.mHeight );
    return ( 4 + 4 * level ) * sizeof( int ); // 4 - size of structure; first 4 - voxels data; others - octree data
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VCT::GenOpacityBrickBuffer()
{
    mfxGenBrickBuffer.BindVCTResources( *this );

    D3DRenderer &renderer = D3DRenderer::Get( );
    auto opacityBrickBufferUAV = mOpacityBrickBuffer->GetUAV();
    auto immediateContext = renderer.GetContext( );
    UINT tmpClearValue[4] = { 0, 0, 0, 0 };
    immediateContext->ClearUnorderedAccessViewUint( opacityBrickBufferUAV, tmpClearValue );
    mfxGenBrickBuffer.mfxBrickBufferRW->SetUnorderedAccessView( opacityBrickBufferUAV );

    ID3D11Buffer *indirectBuffer = mIndirectDrawBuffer->GetBuffer();
    size_t octreeHeight = mOctree.mHeight;
    size_t currentLevel = octreeHeight - 1;

    UINT indirectBufferOffset = GetIndirectBufferOffset( currentLevel );
    mfxGenBrickBuffer.mfxCurrentOctreeLevel->SetInt( currentLevel );
    renderer.SetIndirectLayout( );

    // construct opacity buffer
    mfxGenBrickBuffer.mGenBrickBuffer.mConstructOpacity->Apply( 0, immediateContext );
    immediateContext->DrawInstancedIndirect( indirectBuffer, indirectBufferOffset );

    // average opacity buffer
    AverageBrickAlias( &mfxGenBrickBuffer, indirectBuffer, indirectBufferOffset );

    // generate mips for opacity buffer
    for ( currentLevel = octreeHeight - 2; currentLevel > 0; currentLevel-- )
    {
        // set mip level
        mfxGenBrickBuffer.mfxCurrentOctreeLevel->SetInt( currentLevel );
        indirectBufferOffset = GetIndirectBufferOffset( currentLevel );

        // gather opacity from lower level
        mfxGenBrickBuffer.mGenBrickBuffer.mGatherOpacityFromLowLevel->Apply( 0, immediateContext );
        immediateContext->DrawInstancedIndirect( indirectBuffer, indirectBufferOffset );

        // average opacity for current level
        if ( currentLevel != 0 )
            AverageBrickAlias( &mfxGenBrickBuffer, indirectBuffer, indirectBufferOffset );
    }

    // STATE_SETTING WARNING #2097346
    // should find the way to clear effect11 pipeline
    mfxGenBrickBuffer.mOctreeVariables.mfxOctreeR->SetResource( nullptr );
    mfxGenBrickBuffer.mGenBrickBuffer.mAverageAlongAxisZ->Apply( 0, immediateContext );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VCT::GenRadianceBrickBuffer( std::shared_ptr<D3DTextureBuffer3D> &texbuffer )
{
    D3DRenderer &renderer = D3DRenderer::Get();
    auto immediateContext = renderer.GetContext();

    mfxGenBrickBuffer.mfxBrickBufferRW->SetUnorderedAccessView( texbuffer->GetUAV( ) );

    ID3D11Buffer *indirectBuffer = mIndirectDrawBuffer->GetBuffer();
    size_t octreeHeight = mOctree.mHeight;
    size_t currentLevel = octreeHeight - 1;

    UINT indirectBufferOffset = GetIndirectBufferOffset( currentLevel );
    mfxGenBrickBuffer.mfxCurrentOctreeLevel->SetInt( currentLevel );

    renderer.SetIndirectLayout( );

    // fill the lit bricks
    mfxGenBrickBuffer.mProcessingShadowMap.mAverageLitNodeValues->Apply( 0, immediateContext );
    immediateContext->DrawInstancedIndirect( indirectBuffer, indirectBufferOffset );

    // average irradiance buffer
    AverageLitBrickAlias( &mfxGenBrickBuffer, indirectBuffer, indirectBufferOffset );

    // generate mips for brick buffer
    for ( currentLevel = octreeHeight - 2; currentLevel > 0; currentLevel-- )
    {
        // set mip level
        mfxGenBrickBuffer.mfxCurrentOctreeLevel->SetInt( currentLevel );
        indirectBufferOffset = GetIndirectBufferOffset( currentLevel );

        mfxGenBrickBuffer.mProcessingShadowMap.mGatherValuesFromLowLevel->Apply( 0, immediateContext );
        immediateContext->DrawInstancedIndirect( indirectBuffer, indirectBufferOffset );

        if ( currentLevel != 0 )
            AverageLitBrickAlias( &mfxGenBrickBuffer, indirectBuffer, indirectBufferOffset );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VCT::AverageBrickAlias( FXGenerateBrickBuffer *fx, ID3D11Buffer *indirectBuffer, size_t indirectBufferOffset )
{
    D3DRenderer &renderer = D3DRenderer::Get( );
    auto immediateContext = renderer.GetContext( );

    fx->mGenBrickBuffer.mAverageAlongAxisX->Apply( 0, immediateContext );
    immediateContext->DrawInstancedIndirect( indirectBuffer, indirectBufferOffset );

    fx->mGenBrickBuffer.mAverageAlongAxisY->Apply( 0, immediateContext );
    immediateContext->DrawInstancedIndirect( indirectBuffer, indirectBufferOffset );

    fx->mGenBrickBuffer.mAverageAlongAxisZ->Apply( 0, immediateContext );
    immediateContext->DrawInstancedIndirect( indirectBuffer, indirectBufferOffset );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void VCT::AverageLitBrickAlias( FXGenerateBrickBuffer *fx, ID3D11Buffer *indirectBuffer, size_t indirectBufferOffset )
{
    D3DRenderer &renderer = D3DRenderer::Get( );
    auto immediateContext = renderer.GetContext( );

    fx->mProcessingShadowMap.mAverageAlongAxisX->Apply( 0, immediateContext );
    immediateContext->DrawInstancedIndirect( indirectBuffer, indirectBufferOffset );

    fx->mProcessingShadowMap.mAverageAlongAxisY->Apply( 0, immediateContext );
    immediateContext->DrawInstancedIndirect( indirectBuffer, indirectBufferOffset );

    fx->mProcessingShadowMap.mAverageAlongAxisZ->Apply( 0, immediateContext );
    immediateContext->DrawInstancedIndirect( indirectBuffer, indirectBufferOffset );
}
