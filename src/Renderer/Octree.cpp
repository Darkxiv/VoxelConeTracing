#include <Octree.h>
#include <GlobalUtils.h>
#include <D3DTextureBuffer2D.h>
#include <D3DStructuredBuffer.h>
#include <D3DRenderer.h>
#include <Settings.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Octree::Init()
{
    Settings &settings = Settings::Get();

    mHeight = settings.mOctreeHeight;
    mResolution = 1 << mHeight;

    mBufferSize = settings.mOctreeBufferRes;
    ASSERT( mBufferSize % 16 == 0, "mBufferSize should be divider of 16" );

    mNodesPackCounter = D3DStructuredBuffer::CreateAtomicCounter( );

    D3D11_TEXTURE2D_DESC posBufDesc = D3DTextureBuffer2D::GenTexture2DDesc( mBufferSize, mBufferSize, DXGI_FORMAT_R32_UINT );
    posBufDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    mOctreeTex = D3DTextureBuffer2D::Create( 0, 0, 1, 1, &posBufDesc, 0, 0, 0, 0 );

    // create temp buffer to read mOctreeTex or mIndirectDrawBuffer values through copying and map/unmap for debug purpose
    // D3D11_TEXTURE2D_DESC tmpDesc = D3DTextureBuffer2D::GenTexture2DDesc( 
    //    mBufferSize, mBufferSize, DXGI_FORMAT_R32_UINT, 1, 1, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ, 0, 0, 1, 0 );
    // auto device = renderer.GetDevice( );
    // HRESULT hr = device->CreateTexture2D( &tmpDesc, 0, &tmpCopyBuffer );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Octree::Clear( )
{
    mNodesPackCounter.reset( );
    mOctreeTex.reset();

    mNodesCount = 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Octree::ClearOctree()
{
    D3DRenderer &renderer = D3DRenderer::Get();

    auto octreeTex = mOctreeTex->GetUAV();
    auto nodesPackCounter = mNodesPackCounter->GetUAV();

    ASSERT( octreeTex && nodesPackCounter );
    if ( octreeTex && nodesPackCounter )
    {
        D3DRenderer &renderer = D3DRenderer::Get( );
        auto immediateContext = renderer.GetContext( );

        UINT clearCounter[4] = { 0, 0, 0, 0 };
        UINT clearValue[4] = { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff }; // initial value

        immediateContext->ClearUnorderedAccessViewUint( nodesPackCounter, clearCounter );
        immediateContext->ClearUnorderedAccessViewUint( octreeTex, clearValue );

        ID3D11UnorderedAccessView *uavs[] = { octreeTex, nodesPackCounter };
        // clear counters - effect11 lack
        immediateContext->OMSetRenderTargetsAndUnorderedAccessViews( 0, nullptr, nullptr, 0, 2, uavs, clearCounter );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
