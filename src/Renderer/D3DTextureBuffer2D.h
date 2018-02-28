#ifndef __D3D_TEXTUREBUFFER_2D_H
#define __D3D_TEXTUREBUFFER_2D_H

#include <DirectXTex.h>
#include <memory>
#include <set>

class D3DTextureBuffer2D
{
public:
    static std::shared_ptr<D3DTextureBuffer2D> CreateDefault();
    static std::shared_ptr<D3DTextureBuffer2D> CreateFromFile( const std::string &fn );
    static std::shared_ptr<D3DTextureBuffer2D> CreateFromRTV( ID3D11RenderTargetView *rtv );
    // TODO overloaded interface, should be broken somehow (possibly remove bool flags or move to struct)
    static std::shared_ptr<D3DTextureBuffer2D> Create( bool isRTV, bool isDSV = false, bool isSRV = true, bool isUAV = false,
        const D3D11_TEXTURE2D_DESC *texDesc = nullptr, const D3D11_RENDER_TARGET_VIEW_DESC *rtvDesc = nullptr,
        const D3D11_SHADER_RESOURCE_VIEW_DESC *srvDesc = nullptr, const D3D11_UNORDERED_ACCESS_VIEW_DESC *uavDesc = nullptr,
        float resolutionScale = 0.0f, int width = 0, int height = 0 );

    // can be prototype for another system of texture management
    static std::shared_ptr<D3DTextureBuffer2D> CreateTextureSRV( D3D11_TEXTURE2D_DESC *desc,
        D3D11_SUBRESOURCE_DATA *subRes, D3D11_SHADER_RESOURCE_VIEW_DESC *srvDesc );

    void Resize( int width = 0, int height = 0 );
    int GetWidth( );
    int GetHeight( );
    float GetMainRTResolutionScale( );
    void SetMainRTResolutionScale( float resScale );

    ID3D11ShaderResourceView*   GetSRV();
    ID3D11RenderTargetView*     GetRTV();
    ID3D11DepthStencilView*     GetDSV();
    ID3D11UnorderedAccessView*  GetUAV();
    ID3D11Texture2D*            GetTexBuffer();

    static std::set<D3DTextureBuffer2D*>& GetStorage();

    // directx alias
    static D3D11_UNORDERED_ACCESS_VIEW_DESC GenUAVDesc(
        UINT firstElement, UINT numElements, UINT flags, D3D11_UAV_DIMENSION viewDimension, DXGI_FORMAT format);

    static D3D11_SHADER_RESOURCE_VIEW_DESC GenSRVDescTex2D( DXGI_FORMAT format, UINT mipLevels, UINT mostDetailedMip );
    static D3D11_TEXTURE2D_DESC GenTexture2DDesc( UINT width = 0, UINT height = 0, DXGI_FORMAT format = DXGI_FORMAT_R8G8B8A8_UNORM, UINT arraySize = 1, UINT mipLevels = 1,
        D3D11_USAGE usage = D3D11_USAGE_DEFAULT, UINT CPUaccessFlags = 0, UINT miscFlags = 0, UINT bindFlags = 0, UINT sampleDescCount = 1, UINT sampleDescQuality = 0);

    static D3D11_SUBRESOURCE_DATA GenSubresourceData( void *sysMem, UINT sysMemPitch, UINT sysMemSlicePitch );

private:
    int mWidth, mHeight;
    float mMainRTResolutionScale; // for textures which size bounds to viewport

    ID3D11ShaderResourceView    *mSRV;
    ID3D11RenderTargetView      *mRTV;
    ID3D11DepthStencilView      *mDSV;
    ID3D11UnorderedAccessView   *mUAV;
    ID3D11Texture2D             *mTexBuffer;

    void Init( );
    void InitFromFile( const std::string &fn );

    static std::set<D3DTextureBuffer2D*> mInternalStorage;

    D3DTextureBuffer2D( const std::string &fn );
    D3DTextureBuffer2D( bool isRTV, bool isDSV = false, bool isSRV = true, bool isUAV = false, const D3D11_TEXTURE2D_DESC *texDesc = nullptr,
        const D3D11_RENDER_TARGET_VIEW_DESC *rtvDesc = nullptr, const D3D11_SHADER_RESOURCE_VIEW_DESC *srvDesc = nullptr,
        const D3D11_UNORDERED_ACCESS_VIEW_DESC *uavDesc = nullptr, float resolutionScale = 0.0f, int width = 0, int height = 0 );

    D3DTextureBuffer2D( ID3D11ShaderResourceView *srv, ID3D11RenderTargetView *rtv, ID3D11DepthStencilView *dsv, ID3D11UnorderedAccessView *uav );
    ~D3DTextureBuffer2D( );

    struct _D3DTextureBuffer2D; // shared_ptr proxy
};



#endif
