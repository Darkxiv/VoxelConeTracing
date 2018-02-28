#ifndef __D3D_TEXTUREBUFFER_3D_H
#define __D3D_TEXTUREBUFFER_3D_H

#include <DirectXTex.h>
#include <memory>
#include <set>

class D3DTextureBuffer3D
{
public:
    static std::shared_ptr<D3DTextureBuffer3D> Create( bool isSRV, bool isUAV, const D3D11_TEXTURE3D_DESC *texDesc,
        const D3D11_SHADER_RESOURCE_VIEW_DESC *srvDesc, const D3D11_UNORDERED_ACCESS_VIEW_DESC *uavDesc );

    ID3D11ShaderResourceView*   GetSRV( );
    ID3D11UnorderedAccessView*  GetUAV( );
    ID3D11Texture3D*            GetTextureBuffer( );

    // gen descs

    static std::set<D3DTextureBuffer3D*>& GetStorage();

private:
    ID3D11ShaderResourceView    *mSRV;
    ID3D11UnorderedAccessView   *mUAV;
    ID3D11Texture3D             *mTexBuffer;

    static std::set<D3DTextureBuffer3D*> mInternalStorage;

    D3DTextureBuffer3D( bool isSRV, bool isUAV, const D3D11_TEXTURE3D_DESC *texDesc,
        const D3D11_SHADER_RESOURCE_VIEW_DESC *srvDesc, const D3D11_UNORDERED_ACCESS_VIEW_DESC *uavDesc );
    ~D3DTextureBuffer3D( );

    struct _D3DTextureBuffer3D; // shared_ptr proxy
};

#endif