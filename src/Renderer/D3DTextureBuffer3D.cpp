#include <D3DTextureBuffer3D.h>
#include <GlobalUtils.h>
#include <D3DRenderer.h>

std::set<D3DTextureBuffer3D*> D3DTextureBuffer3D::mInternalStorage;

struct D3DTextureBuffer3D::_D3DTextureBuffer3D: public D3DTextureBuffer3D
{
    _D3DTextureBuffer3D( bool isSRV, bool isUAV, const D3D11_TEXTURE3D_DESC *texDesc,
        const D3D11_SHADER_RESOURCE_VIEW_DESC *srvDesc, const D3D11_UNORDERED_ACCESS_VIEW_DESC *uavDesc ):
        D3DTextureBuffer3D( isSRV, isUAV, texDesc, srvDesc, uavDesc ) {}
    ~_D3DTextureBuffer3D() {}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DTextureBuffer3D> D3DTextureBuffer3D::Create( bool isSRV, bool isUAV, const D3D11_TEXTURE3D_DESC *texDesc,
    const D3D11_SHADER_RESOURCE_VIEW_DESC *srvDesc, const D3D11_UNORDERED_ACCESS_VIEW_DESC *uavDesc )
{
    return std::make_shared<_D3DTextureBuffer3D>( isSRV, isUAV, texDesc, srvDesc, uavDesc );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11ShaderResourceView* D3DTextureBuffer3D::GetSRV( )
{
    ASSERT( mSRV );
    return mSRV;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11UnorderedAccessView* D3DTextureBuffer3D::GetUAV( )
{
    ASSERT( mUAV );
    return mUAV;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11Texture3D* D3DTextureBuffer3D::GetTextureBuffer( )
{
    ASSERT( mTexBuffer );
    return mTexBuffer;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::set<D3DTextureBuffer3D*>& D3DTextureBuffer3D::GetStorage( )
{
    return mInternalStorage;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3DTextureBuffer3D::D3DTextureBuffer3D( bool isSRV, bool isUAV, const D3D11_TEXTURE3D_DESC *texDesc,
    const D3D11_SHADER_RESOURCE_VIEW_DESC *srvDesc, const D3D11_UNORDERED_ACCESS_VIEW_DESC *uavDesc ) :
    mSRV( nullptr ),
    mUAV( nullptr ),
    mTexBuffer( nullptr )
{
    mInternalStorage.insert( this );

    ASSERT( ( isSRV || isUAV ) && texDesc );
    if ( !( isSRV || isUAV ) && texDesc )
        return;

    D3DRenderer &renderer = D3DRenderer::Get( );
    auto device = renderer.GetDevice( );

    HRESULT hr = device->CreateTexture3D( texDesc, 0, &mTexBuffer );
    ASSERT( hr == S_OK );
    if ( hr == S_OK )
    {
        if ( isSRV )
        {
            if ( srvDesc )
                hr = device->CreateShaderResourceView( mTexBuffer, srvDesc, &mSRV );
            else
                hr = device->CreateShaderResourceView( mTexBuffer, 0, &mSRV );
            ASSERT( hr == S_OK );
        }
        if ( isUAV )
        {
            if ( uavDesc )
                hr = device->CreateUnorderedAccessView( mTexBuffer, uavDesc, &mUAV );
            else
                hr = device->CreateUnorderedAccessView( mTexBuffer, 0, &mUAV );
            ASSERT( hr == S_OK );
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3DTextureBuffer3D::~D3DTextureBuffer3D( )
{
    mInternalStorage.erase( this );

    COMSafeRelease( mSRV );
    COMSafeRelease( mUAV );
    COMSafeRelease( mTexBuffer );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
