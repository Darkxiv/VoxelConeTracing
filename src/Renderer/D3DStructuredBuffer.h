#ifndef __D3D_BUFFER_H
#define __D3D_BUFFER_H

#include <DirectXTex.h>
#include <memory>
#include <set>

// structured buffer and atomic counters case
class D3DStructuredBuffer
{
public:
    static std::shared_ptr<D3DStructuredBuffer> CreateBuffer( bool isSRV, bool isUAV, const D3D11_BUFFER_DESC *bufDesc,
        D3D11_SUBRESOURCE_DATA *subresData, const D3D11_SHADER_RESOURCE_VIEW_DESC *srvDesc, const D3D11_UNORDERED_ACCESS_VIEW_DESC *uavDesc );

    static std::shared_ptr<D3DStructuredBuffer> CreateAtomicCounter( );

    ID3D11ShaderResourceView*   GetSRV( );
    ID3D11UnorderedAccessView*  GetUAV( );
    ID3D11Buffer*               GetBuffer( );

    static std::set<D3DStructuredBuffer*>& GetStorage( );

    // directx alias
    static D3D11_BUFFER_DESC GenBufferDesc( 
        D3D11_USAGE usage, UINT byteWidth, UINT bindFlags, UINT CPUAccessFlags, UINT miscFlags, UINT structureByteStride );

    static D3D11_SUBRESOURCE_DATA GenSubresourceData( 
        UINT sysMemPitch, UINT sysMemSlicePitch, const void *pSysMem );

    static D3D11_UNORDERED_ACCESS_VIEW_DESC GenUAVDesc( 
        UINT firstElement, UINT numElements, UINT flags, D3D11_UAV_DIMENSION viewDimension, DXGI_FORMAT format );

private:
    ID3D11ShaderResourceView    *mSRV;
    ID3D11UnorderedAccessView   *mUAV;
    ID3D11Buffer                *mBuffer;

    static std::set<D3DStructuredBuffer*> mInternalStorage;

    D3DStructuredBuffer( bool isSRV, bool isUAV, const D3D11_BUFFER_DESC *bufDesc, D3D11_SUBRESOURCE_DATA *subresData,
        const D3D11_SHADER_RESOURCE_VIEW_DESC *srvDesc, const D3D11_UNORDERED_ACCESS_VIEW_DESC *uavDesc );

    ~D3DStructuredBuffer( );

    struct _D3DStructuredBuffer; // shared_ptr proxy
};



#endif
