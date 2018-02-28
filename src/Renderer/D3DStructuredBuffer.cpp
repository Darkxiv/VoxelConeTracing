#include <D3DStructuredBuffer.h>
#include <GlobalUtils.h>
#include <D3DTextureBuffer2D.h>
#include <D3DRenderer.h>
#include <DirectXTex.h>

std::set<D3DStructuredBuffer*> D3DStructuredBuffer::mInternalStorage;

struct D3DStructuredBuffer::_D3DStructuredBuffer: public D3DStructuredBuffer
{
    _D3DStructuredBuffer( bool isSRV, bool isUAV, const D3D11_BUFFER_DESC *bufDesc, D3D11_SUBRESOURCE_DATA *subresData,
        const D3D11_SHADER_RESOURCE_VIEW_DESC *srvDesc, const D3D11_UNORDERED_ACCESS_VIEW_DESC *uavDesc ):
        D3DStructuredBuffer( isSRV, isUAV, bufDesc, subresData, srvDesc, uavDesc ) {}
    ~_D3DStructuredBuffer() {}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DStructuredBuffer> D3DStructuredBuffer::CreateBuffer( bool isSRV, bool isUAV, const D3D11_BUFFER_DESC *bufDesc,
    D3D11_SUBRESOURCE_DATA *subresData, const D3D11_SHADER_RESOURCE_VIEW_DESC *srvDesc, const D3D11_UNORDERED_ACCESS_VIEW_DESC *uavDesc )
{
    return std::make_shared<_D3DStructuredBuffer>( isSRV, isUAV, bufDesc, subresData, srvDesc, uavDesc );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DStructuredBuffer> D3DStructuredBuffer::CreateAtomicCounter( )
{
    D3D11_BUFFER_DESC counterBufDesc = GenBufferDesc( D3D11_USAGE_DEFAULT, sizeof( int ), 
        D3D11_BIND_UNORDERED_ACCESS | D3D11_BIND_SHADER_RESOURCE,0, D3D11_RESOURCE_MISC_BUFFER_STRUCTURED, sizeof( int ) );

    D3D11_UNORDERED_ACCESS_VIEW_DESC counterUAVDesc = D3DTextureBuffer2D::GenUAVDesc(
        0, 1, D3D11_BUFFER_UAV_FLAG_COUNTER, D3D11_UAV_DIMENSION_BUFFER, DXGI_FORMAT_UNKNOWN );

    return std::make_shared<_D3DStructuredBuffer>( false, true, &counterBufDesc, nullptr, nullptr, &counterUAVDesc );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11ShaderResourceView* D3DStructuredBuffer::GetSRV( )
{
    ASSERT( mSRV );
    return mSRV;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11UnorderedAccessView* D3DStructuredBuffer::GetUAV( )
{
    ASSERT( mUAV );
    return mUAV;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11Buffer* D3DStructuredBuffer::GetBuffer( )
{
    ASSERT( mBuffer );
    return mBuffer;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::set<D3DStructuredBuffer*>& D3DStructuredBuffer::GetStorage( )
{
    return mInternalStorage;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3D11_BUFFER_DESC D3DStructuredBuffer::GenBufferDesc( D3D11_USAGE usage, UINT byteWidth, UINT bindFlags, UINT CPUAccessFlags, UINT miscFlags, UINT structureByteStride )
{
    D3D11_BUFFER_DESC bd;
    ZeroMemory( &bd, sizeof( bd ) );
    bd.Usage = usage;
    bd.ByteWidth = byteWidth;
    bd.BindFlags = bindFlags;
    bd.CPUAccessFlags = CPUAccessFlags;
    bd.MiscFlags = miscFlags;
    bd.StructureByteStride = structureByteStride;
    return bd;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3D11_SUBRESOURCE_DATA D3DStructuredBuffer::GenSubresourceData( UINT sysMemPitch, UINT sysMemSlicePitch, const void *pSysMem )
{
    D3D11_SUBRESOURCE_DATA sd;
    sd.SysMemPitch = sysMemPitch;
    sd.SysMemSlicePitch = sysMemSlicePitch;
    sd.pSysMem = pSysMem;
    return sd;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3D11_UNORDERED_ACCESS_VIEW_DESC D3DStructuredBuffer::GenUAVDesc( UINT firstElement, UINT numElements, UINT flags, D3D11_UAV_DIMENSION viewDimension, DXGI_FORMAT format )
{
    return D3DTextureBuffer2D::GenUAVDesc(firstElement, numElements, flags, viewDimension, format );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3DStructuredBuffer::D3DStructuredBuffer( bool isSRV, bool isUAV, const D3D11_BUFFER_DESC *bufDesc, D3D11_SUBRESOURCE_DATA *subresData,
    const D3D11_SHADER_RESOURCE_VIEW_DESC *srvDesc, const D3D11_UNORDERED_ACCESS_VIEW_DESC *uavDesc ) :
    mSRV( nullptr ),
    mUAV( nullptr ),
    mBuffer( nullptr )
{
    mInternalStorage.insert( this );

    ASSERT( bufDesc && ( isSRV && srvDesc || isUAV && uavDesc ) );
    if ( bufDesc )
    {
        D3DRenderer &renderer = D3DRenderer::Get( );
        auto device = renderer.GetDevice( );

        mBuffer = nullptr;
        HRESULT hr = device->CreateBuffer( bufDesc, subresData, &mBuffer );
        ASSERT( hr == S_OK );
        if ( hr == S_OK )
        {
            if ( isSRV )
            {
                hr = device->CreateShaderResourceView( mBuffer, srvDesc, &mSRV );
                ASSERT( hr == S_OK );
            }
            if ( isUAV )
            {
                hr = device->CreateUnorderedAccessView( mBuffer, uavDesc, &mUAV );
                ASSERT( hr == S_OK );
            }
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3DStructuredBuffer::~D3DStructuredBuffer( )
{
    mInternalStorage.erase( this );

    COMSafeRelease( mBuffer );
    COMSafeRelease( mSRV );
    COMSafeRelease( mUAV );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
