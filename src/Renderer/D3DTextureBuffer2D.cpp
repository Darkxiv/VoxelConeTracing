#include <D3DTextureBuffer2D.h>
#include <D3DRenderer.h>
#include <GlobalUtils.h>
#include <DirectXTex.h>
#include <string>
#include <Settings.h>

std::set<D3DTextureBuffer2D*> D3DTextureBuffer2D::mInternalStorage;

struct D3DTextureBuffer2D::_D3DTextureBuffer2D: public D3DTextureBuffer2D
{
    _D3DTextureBuffer2D( const std::string &fn ) : D3DTextureBuffer2D( fn ) {}
    _D3DTextureBuffer2D( ID3D11ShaderResourceView *srv, ID3D11RenderTargetView *rtv, ID3D11DepthStencilView *dsv, ID3D11UnorderedAccessView *uav ) :
        D3DTextureBuffer2D( srv, rtv, dsv, uav ) {}

    _D3DTextureBuffer2D( bool isRTV, bool isDSV, bool isSRV, bool isUAV, const D3D11_TEXTURE2D_DESC *texDesc,
        const D3D11_RENDER_TARGET_VIEW_DESC *rtvDesc, const D3D11_SHADER_RESOURCE_VIEW_DESC *srvDesc,
        const D3D11_UNORDERED_ACCESS_VIEW_DESC *uavDesc, float resolutionScale, int width, int height ) :
        D3DTextureBuffer2D( isRTV, isDSV, isSRV, isUAV, texDesc, rtvDesc, srvDesc, uavDesc, resolutionScale, width, height ) {}

    ~_D3DTextureBuffer2D() {}
};
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DTextureBuffer2D> D3DTextureBuffer2D::CreateDefault( )
{
    Settings &settings = Settings::Get( );
    std::string fn = std::string( settings.mMediaDir ) + std::string( "checker.tga" );
    return std::make_shared<_D3DTextureBuffer2D>( fn );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DTextureBuffer2D> D3DTextureBuffer2D::CreateFromFile( const std::string &fn )
{
    return std::make_shared<_D3DTextureBuffer2D>( fn );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DTextureBuffer2D> D3DTextureBuffer2D::CreateFromRTV( ID3D11RenderTargetView *rtv )
{
    return std::make_shared<_D3DTextureBuffer2D>( nullptr, rtv, nullptr, nullptr );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DTextureBuffer2D> D3DTextureBuffer2D::Create( bool isRTV, bool isDSV, bool isSRV, bool isUAV, 
    const D3D11_TEXTURE2D_DESC *texDesc, const D3D11_RENDER_TARGET_VIEW_DESC *rtvDesc, 
    const D3D11_SHADER_RESOURCE_VIEW_DESC *srvDesc, const D3D11_UNORDERED_ACCESS_VIEW_DESC *uavDesc, 
    float resolutionScale, int width, int height )
{
    return std::make_shared<_D3DTextureBuffer2D>( 
        isRTV, isDSV, isSRV, isUAV, texDesc, rtvDesc, srvDesc, uavDesc, resolutionScale, width, height );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DTextureBuffer2D> D3DTextureBuffer2D::CreateTextureSRV( D3D11_TEXTURE2D_DESC *desc,
    D3D11_SUBRESOURCE_DATA *subRes, D3D11_SHADER_RESOURCE_VIEW_DESC *srvDesc )
{
    D3DRenderer &renderer = D3DRenderer::Get( );
    ID3D11Device *device = renderer.GetDevice( );

    ID3D11Texture2D *tex = nullptr;
    HRESULT hr = device->CreateTexture2D( desc, subRes, &tex );
    ASSERT( hr == S_OK );

    ID3D11ShaderResourceView *tmpSrv = nullptr;
    if ( hr == S_OK )
    {
        hr = device->CreateShaderResourceView( tex, srvDesc, &tmpSrv );
        ASSERT( hr == S_OK );

        tex->Release( );
    }

    return std::make_shared<_D3DTextureBuffer2D>( tmpSrv, nullptr, nullptr, nullptr );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DTextureBuffer2D::Resize( int width, int height )
{
    if ( !mRTV && !mDSV && !mSRV && !mUAV )
        return;

    D3DRenderer &renderer = D3DRenderer::Get( );
    auto device = renderer.GetDevice( );

    if ( mMainRTResolutionScale > 0.0f )
    {
        // change resolution according to viewport
        width = ( int )( renderer.GetWidth() * mMainRTResolutionScale );
        height = ( int )( renderer.GetHeight() * mMainRTResolutionScale );
    }

    D3D11_TEXTURE2D_DESC texDesc;
    ID3D11Resource *res = nullptr;

    // backup old params
    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc;

    if ( mRTV )
    {
        mRTV->GetDesc( &rtvDesc );
        mRTV->GetResource( &res );
    }
    if ( mDSV )
    {
        mDSV->GetDesc( &dsvDesc );
        if ( !res )
            mDSV->GetResource( &res );
    }
    if ( mSRV )
    {
        mSRV->GetDesc( &srvDesc );
        if ( !res )
            mSRV->GetResource( &res );
    }
    if ( mUAV )
    {
        mUAV->GetDesc( &uavDesc );
        if ( !res )
            mUAV->GetResource( &res );
    }

    // change texture resolution
    ID3D11Texture2D *tex = ( ID3D11Texture2D * )res;
    tex->GetDesc( &texDesc );
    texDesc.Height = height;
    texDesc.Width = width;
    tex->Release();

    ID3D11Texture2D *tmpTexBuffer;
    HRESULT hr = device->CreateTexture2D( &texDesc, 0, &tmpTexBuffer );
    if ( !FAILED( hr ) )
    {
        bool rtvCheck = true, dsvCheck = true, srvCheck = true, uavCheck = true;
        ID3D11RenderTargetView *newRTV = nullptr;
        ID3D11DepthStencilView *newDSV = nullptr;
        ID3D11ShaderResourceView *newSRV = nullptr;
        ID3D11UnorderedAccessView *newUAV = nullptr;

        if ( mRTV )
        {
            hr = device->CreateRenderTargetView( tmpTexBuffer, &rtvDesc, &newRTV );
            if ( FAILED( hr ) || newRTV == nullptr )
            {
                rtvCheck = false;
            }
            ASSERT( !FAILED( hr ), "Error during texture resize: RTV" );
        }
        if ( mDSV )
        {
            hr = device->CreateDepthStencilView( tmpTexBuffer, &dsvDesc, &newDSV );
            if ( FAILED( hr ) || newDSV == nullptr )
            {
                dsvCheck = false;
            }
            ASSERT( !FAILED( hr ), "Error during texture resize: DSV" );
        }
        if ( mSRV )
        {
            hr = device->CreateShaderResourceView( tmpTexBuffer, &srvDesc, &newSRV );
            if ( FAILED( hr ) || newSRV == nullptr )
            {
                srvCheck = false;
            }
            ASSERT( !FAILED( hr ), "Error during texture resize: SRV" );
        }
        if ( mUAV )
        {
            hr = device->CreateUnorderedAccessView( tmpTexBuffer, &uavDesc, &newUAV );
            if ( FAILED( hr ) || newUAV == nullptr )
            {
                uavCheck = false;
            }
            ASSERT( !FAILED( hr ), "Error during texture resize: UAV" );
        }

        if ( rtvCheck && dsvCheck && srvCheck && uavCheck )
        {
            // success
            COMSafeRelease( mRTV );
            COMSafeRelease( mDSV );
            COMSafeRelease( mSRV );
            COMSafeRelease( mUAV );

            mRTV = newRTV;
            mDSV = newDSV;
            mSRV = newSRV;
            mUAV = newUAV;

            mWidth = width;
            mHeight = height;
        }
        else
        {
            COMSafeRelease( newRTV );
            COMSafeRelease( newDSV );
            COMSafeRelease( newSRV );
            COMSafeRelease( newUAV );
        }
    }
    ASSERT( !FAILED( hr ), "Error during texture resize" );

    COMSafeRelease( tmpTexBuffer );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int D3DTextureBuffer2D::GetWidth( )
{
    ASSERT( mWidth );
    return mWidth;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int D3DTextureBuffer2D::GetHeight( )
{
    ASSERT( mHeight );
    return mHeight;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
float D3DTextureBuffer2D::GetMainRTResolutionScale( )
{
    return mMainRTResolutionScale;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DTextureBuffer2D::SetMainRTResolutionScale( float resScale )
{
    mMainRTResolutionScale = resScale;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11RenderTargetView* D3DTextureBuffer2D::GetRTV( )
{
    ASSERT( mRTV );
    return mRTV;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11ShaderResourceView* D3DTextureBuffer2D::GetSRV( )
{
    ASSERT( mSRV );
    return mSRV;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11DepthStencilView* D3DTextureBuffer2D::GetDSV( )
{
    ASSERT( mDSV );
    return mDSV;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11UnorderedAccessView* D3DTextureBuffer2D::GetUAV( )
{
    ASSERT( mUAV );
    return mUAV;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11Texture2D* D3DTextureBuffer2D::GetTexBuffer( )
{
    ASSERT( mTexBuffer );
    return mTexBuffer;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::set<D3DTextureBuffer2D*>& D3DTextureBuffer2D::GetStorage()
{
    return mInternalStorage;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3D11_UNORDERED_ACCESS_VIEW_DESC D3DTextureBuffer2D::GenUAVDesc( UINT firstElement, UINT numElements, UINT flags,
    D3D11_UAV_DIMENSION viewDimension, DXGI_FORMAT format )
{
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavd;
    ZeroMemory( &uavd, sizeof( uavd ) );
    uavd.Buffer.FirstElement = firstElement;
    uavd.Buffer.NumElements = numElements;
    uavd.Buffer.Flags = flags;
    uavd.ViewDimension = viewDimension;
    uavd.Format = format;

    return uavd;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3D11_SHADER_RESOURCE_VIEW_DESC D3DTextureBuffer2D::GenSRVDescTex2D( DXGI_FORMAT format, UINT mipLevels, UINT mostDetailedMip )
{
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
    ZeroMemory( &srvDesc, sizeof( srvDesc ) );
    srvDesc.Format = format;
    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srvDesc.Texture2D.MipLevels = mipLevels;
    srvDesc.Texture2D.MostDetailedMip = mostDetailedMip;

    return srvDesc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3D11_TEXTURE2D_DESC D3DTextureBuffer2D::GenTexture2DDesc( UINT width, UINT height, DXGI_FORMAT format, UINT arraySize,
    UINT mipLevels, D3D11_USAGE usage, UINT CPUaccessFlags, UINT miscFlags, UINT bindFlags, UINT sampleDescCount,
    UINT sampleDescQuality )
{
    D3D11_TEXTURE2D_DESC texDesc;

    ASSERT( width != 0 && height != 0 );

    ZeroMemory( &texDesc, sizeof( texDesc ) );
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.Format = format;
    texDesc.ArraySize = arraySize;
    texDesc.MipLevels = mipLevels;
    texDesc.Usage = usage;
    texDesc.CPUAccessFlags = CPUaccessFlags;
    texDesc.MiscFlags = miscFlags;
    texDesc.BindFlags = bindFlags;
    texDesc.SampleDesc.Count = sampleDescCount;
    texDesc.SampleDesc.Quality = sampleDescQuality;

    return texDesc;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3D11_SUBRESOURCE_DATA D3DTextureBuffer2D::GenSubresourceData( void *sysMem, UINT sysMemPitch, UINT sysMemSlicePitch )
{
    D3D11_SUBRESOURCE_DATA subResource;
    subResource.pSysMem = sysMem;
    subResource.SysMemPitch = sysMemPitch;
    subResource.SysMemSlicePitch = sysMemSlicePitch;

    return subResource;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DTextureBuffer2D::Init( )
{
    mInternalStorage.insert( this );

    mSRV = nullptr;
    mRTV = nullptr;
    mDSV = nullptr;
    mUAV = nullptr;
    mTexBuffer = nullptr;

    mWidth = 0;
    mHeight = 0;
    mMainRTResolutionScale = 0.0f;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DTextureBuffer2D::InitFromFile( const std::string &fn )
{
    Init( );
    std::string ext;
    std::size_t found = fn.find_last_of( "." );
    ASSERT( found != std::string::npos );
    if ( found != std::string::npos )
    {
        ext = fn.substr( found + 1 );
        // TODO check ext
    }
    else
    {
        return;
    }

    // create default texture
    D3DRenderer &renderer = D3DRenderer::Get( );
    auto device = renderer.GetDevice( );
    if ( device && found != std::string::npos )
    {
        HRESULT hr = S_FALSE;
        DirectX::ScratchImage img;

        if ( ext.compare( "tga" ) == 0 || ext.compare( "dds" ) == 0 )
        {
            std::wstring wfn = std::wstring( fn.begin( ), fn.end( ) );

            if ( ext.compare( "tga" ) == 0 )
                hr = DirectX::LoadFromTGAFile( wfn.c_str( ), nullptr, img );
            else
                hr = DirectX::LoadFromDDSFile( wfn.c_str( ), 0, nullptr, img );

            WARNING( hr != S_OK, "Error during openning file ", fn, ", hr = ", hr );
            if ( hr == S_OK )
            {
                auto meta = img.GetMetadata( );
                hr = DirectX::CreateShaderResourceView( device, img.GetImages( ), img.GetImageCount( ), img.GetMetadata( ), &mSRV );

                if ( hr == S_OK )
                {
                    mWidth = meta.width;
                    mHeight = meta.height;
                    mMainRTResolutionScale = 0.0f;
                }

                WARNING( hr != S_OK, "Error during openning file ", fn, ", hr = ", hr );
            }
        }
        else
        {
            WARNING( false, "Error during openning file, unknown format ", fn );
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3DTextureBuffer2D::D3DTextureBuffer2D( const std::string &fn )
{
    InitFromFile( fn );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3DTextureBuffer2D::D3DTextureBuffer2D( bool isRTV, bool isDSV, bool isSRV, bool isUAV, const D3D11_TEXTURE2D_DESC *texDesc,
    const D3D11_RENDER_TARGET_VIEW_DESC *rtvDesc, const D3D11_SHADER_RESOURCE_VIEW_DESC *srvDesc,
    const D3D11_UNORDERED_ACCESS_VIEW_DESC *uavDesc, float resolutionScale, int width, int height )
{
    Init( );
    ASSERT( !( isRTV && isDSV ) && ( !!width == !!height ), "unexpected behavior: width = ", width, "height = ", height );
    if ( ( isRTV && isDSV ) || !!width != !!height )
        return;

    D3DRenderer &renderer = D3DRenderer::Get( );
    if ( !width && !height )
    {
        if ( resolutionScale > 0.0f )
        {
            width = ( int )( renderer.GetWidth( ) * resolutionScale );
            height = ( int )( renderer.GetHeight( ) * resolutionScale );
        }
        else
        {
            width = renderer.GetWidth( );
            height = renderer.GetHeight( );
        }
    }
    else
    {
        resolutionScale = 0.0f;
    }
    mWidth = width;
    mHeight = height;
    mMainRTResolutionScale = resolutionScale;

    auto device = renderer.GetDevice( );
    // todo refactor this
    if ( isDSV )
    {
        if ( !texDesc && !rtvDesc && !srvDesc )
        {
            D3D11_TEXTURE2D_DESC depthStencilDesc;
            ZeroMemory( &depthStencilDesc, sizeof( depthStencilDesc ) );
            depthStencilDesc.Width = width;
            depthStencilDesc.Height = height;
            depthStencilDesc.Format = isSRV ? DXGI_FORMAT_R24G8_TYPELESS : DXGI_FORMAT_D24_UNORM_S8_UINT;
            depthStencilDesc.ArraySize = 1;
            depthStencilDesc.MipLevels = 1;
            depthStencilDesc.Usage = D3D11_USAGE_DEFAULT;
            depthStencilDesc.CPUAccessFlags = 0;
            depthStencilDesc.MiscFlags = 0;
            depthStencilDesc.BindFlags = isSRV ? D3D11_BIND_DEPTH_STENCIL | D3D11_BIND_SHADER_RESOURCE : D3D11_BIND_DEPTH_STENCIL;
            depthStencilDesc.SampleDesc.Count = 1;
            depthStencilDesc.SampleDesc.Quality = 0;

            ID3D11Texture2D *depthStencilBuffer;
            HRESULT hr = device->CreateTexture2D( &depthStencilDesc, 0, &depthStencilBuffer );
            ASSERT( !FAILED( hr ) );
            if ( !FAILED( hr ) )
            {
                if ( isSRV )
                {
                    D3D11_DEPTH_STENCIL_VIEW_DESC dsvDesc;
                    dsvDesc.Flags = 0;
                    dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
                    dsvDesc.ViewDimension = D3D11_DSV_DIMENSION_TEXTURE2D;
                    dsvDesc.Texture2D.MipSlice = 0;
                    hr = device->CreateDepthStencilView( depthStencilBuffer, &dsvDesc, &mDSV );
                    ASSERT( !FAILED( hr ) );

                    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
                    srvDesc.Format = DXGI_FORMAT_R24_UNORM_X8_TYPELESS;
                    srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
                    srvDesc.Texture2D.MipLevels = depthStencilDesc.MipLevels;
                    srvDesc.Texture2D.MostDetailedMip = 0;
                    hr = device->CreateShaderResourceView( depthStencilBuffer, &srvDesc, &mSRV );
                    ASSERT( !FAILED( hr ) );
                }
                else
                {
                    hr = device->CreateDepthStencilView( depthStencilBuffer, 0, &mDSV );
                    ASSERT( !FAILED( hr ) );
                }

                if ( !FAILED( hr ) )
                    mTexBuffer = depthStencilBuffer;
            }
        }
    }
    else if ( isSRV || isRTV || isUAV )
    {
        const D3D11_TEXTURE2D_DESC *pTmpTexDesc;
        D3D11_TEXTURE2D_DESC tmpTexDesc;

        if ( !texDesc )
        {
            ZeroMemory( &tmpTexDesc, sizeof( tmpTexDesc ) );
            tmpTexDesc.Width = width;
            tmpTexDesc.Height = height;
            tmpTexDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
            tmpTexDesc.ArraySize = 1;
            tmpTexDesc.MipLevels = 1;
            tmpTexDesc.Usage = D3D11_USAGE_DEFAULT;
            tmpTexDesc.CPUAccessFlags = 0;
            tmpTexDesc.MiscFlags = 0;
            tmpTexDesc.BindFlags = 0;
            if ( isSRV )
                tmpTexDesc.BindFlags |= D3D11_BIND_SHADER_RESOURCE;
            if ( isRTV )
                tmpTexDesc.BindFlags |= D3D11_BIND_RENDER_TARGET;
            if ( isUAV )
                tmpTexDesc.BindFlags |= D3D11_BIND_UNORDERED_ACCESS;
            tmpTexDesc.SampleDesc.Count = 1;
            tmpTexDesc.SampleDesc.Quality = 0;

            pTmpTexDesc = &tmpTexDesc;
        }
        else
        {
            pTmpTexDesc = texDesc;
        }

        ID3D11Texture2D *tmpTexBuffer;
        HRESULT hr = device->CreateTexture2D( pTmpTexDesc, 0, &tmpTexBuffer );
        ASSERT( !FAILED( hr ) );
        if ( !FAILED( hr ) )
        {
            if ( isSRV )
            {
                if ( srvDesc )
                    hr = device->CreateShaderResourceView( tmpTexBuffer, srvDesc, &mSRV );
                else
                    hr = device->CreateShaderResourceView( tmpTexBuffer, 0, &mSRV );
                ASSERT( !FAILED( hr ) );
            }
            if ( isRTV )
            {
                if ( rtvDesc )
                    hr = device->CreateRenderTargetView( tmpTexBuffer, rtvDesc, &mRTV );
                else
                    hr = device->CreateRenderTargetView( tmpTexBuffer, 0, &mRTV );
                ASSERT( !FAILED( hr ) );
            }
            if ( isUAV )
            {
                if ( uavDesc )
                    hr = device->CreateUnorderedAccessView( tmpTexBuffer, uavDesc, &mUAV );
                else
                    hr = device->CreateUnorderedAccessView( tmpTexBuffer, 0, &mUAV );
                ASSERT( !FAILED( hr ) );
            }

            if ( !FAILED( hr ) )
                mTexBuffer = tmpTexBuffer;
        }
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3DTextureBuffer2D::D3DTextureBuffer2D( ID3D11ShaderResourceView *srv, ID3D11RenderTargetView *rtv,
    ID3D11DepthStencilView *dsv, ID3D11UnorderedAccessView *uav )
{
    Init( );
    mSRV = srv;
    mRTV = rtv;
    mDSV = dsv;
    mUAV = uav;

    // warning: we doesn't setup mWidth and mHeight here
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3DTextureBuffer2D::~D3DTextureBuffer2D( )
{
    mInternalStorage.erase( this );

    COMSafeRelease( mSRV );
    COMSafeRelease( mRTV );
    COMSafeRelease( mDSV );
    COMSafeRelease( mUAV );
    COMSafeRelease( mTexBuffer );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
