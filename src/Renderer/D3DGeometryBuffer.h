#ifndef __GEOMETRY_BUFFER_AGREGATOR_H
#define __GEOMETRY_BUFFER_AGREGATOR_H

#include <DirectXMath.h>
#include <vector>
#include <memory>
#include <set>

struct ID3D11Buffer;
struct GGMeshData;

struct Vertex3F3F3F2F
{
    DirectX::XMFLOAT3 mPosition;
    DirectX::XMFLOAT3 mNormal;
    DirectX::XMFLOAT3 mBinormal;
    DirectX::XMFLOAT2 mUV;
};

class D3DGeometryBuffer
{
public:
    enum VertexFormat
    {
        V_3F3F3F2F
    } mFormat;

    static std::shared_ptr<D3DGeometryBuffer> Create( const GGMeshData &data );
    static std::shared_ptr<D3DGeometryBuffer> Create( const std::vector<Vertex3F3F3F2F> &vBuf, const std::vector<uint32_t> &iBuf );

    ID3D11Buffer* GetVB() const;
    ID3D11Buffer* GetIB() const;
    int GetIndexCount() const;

    std::vector<Vertex3F3F3F2F>& GetRawVB();
    std::vector<uint32_t>& GetRawIB();

    static std::set<D3DGeometryBuffer*>& GetStorage();

private:
    ID3D11Buffer *mVB;
    ID3D11Buffer *mIB;
    int mIndexCount;

    // provided by Settings.mSaveScene
    std::vector<Vertex3F3F3F2F> mRawVB;
    std::vector<uint32_t> mRawIB;

    static std::set<D3DGeometryBuffer*> mInternalStorage;

    static void FillGeometryBufferAgregator( 
        std::shared_ptr<D3DGeometryBuffer> &agregator, const std::vector<Vertex3F3F3F2F> &vBuf, const std::vector<uint32_t> &iBuf );

    D3DGeometryBuffer( );
    ~D3DGeometryBuffer( );

    struct _GeometryBufferAgregator; // shared_ptr proxy
};

#endif