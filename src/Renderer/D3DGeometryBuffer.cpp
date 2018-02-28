#include <D3DGeometryBuffer.h>
#include <GlobalUtils.h>
#include <GeometryGenerator.h>
#include <D3DRenderer.h>
#include <Settings.h>

std::set<D3DGeometryBuffer*> D3DGeometryBuffer::mInternalStorage;

struct D3DGeometryBuffer::_GeometryBufferAgregator : public D3DGeometryBuffer
{
    _GeometryBufferAgregator(): D3DGeometryBuffer() {}
    ~_GeometryBufferAgregator() {}
};

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DGeometryBuffer> D3DGeometryBuffer::Create( const GGMeshData &data )
{
    D3DRenderer &renderer = D3DRenderer::Get( );
    std::shared_ptr<D3DGeometryBuffer> agregator = std::make_shared<_GeometryBufferAgregator>( );
    std::vector<Vertex3F3F3F2F> verticies;

    for ( size_t i = 0; i < data.verticies.size( ); i++ )
    {
        Vertex3F3F3F2F v;
        const GGVertex &ggv = data.verticies[i];
        v.mPosition = ggv.position;
        v.mNormal = ggv.normal;
        v.mBinormal = ggv.binormal;
        v.mUV = DirectX::XMFLOAT2( ggv.UVW.x, ggv.UVW.y );

        verticies.push_back( v );
        renderer.CalcStaticSceneBB( v.mPosition );
    }

    FillGeometryBufferAgregator( agregator, verticies, data.indicies );

    agregator->mIndexCount = data.indicies.size( );
    agregator->mFormat = D3DGeometryBuffer::V_3F3F3F2F;

    if ( Settings::Get( ).mSaveScene )
    {
        agregator->mRawVB = verticies;
        agregator->mRawIB = data.indicies;
    }

    return agregator;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<D3DGeometryBuffer> D3DGeometryBuffer::Create(
    const std::vector<Vertex3F3F3F2F> &vBuf, const std::vector<uint32_t> &iBuf )
{
    D3DRenderer &renderer = D3DRenderer::Get( );
    std::shared_ptr<D3DGeometryBuffer> agregator = std::make_shared<_GeometryBufferAgregator>( );

    for ( size_t i = 0; i < vBuf.size(); i++ )
        renderer.CalcStaticSceneBB( vBuf[i].mPosition );

    FillGeometryBufferAgregator( agregator, vBuf, iBuf );

    agregator->mIndexCount = iBuf.size( );
    agregator->mFormat = D3DGeometryBuffer::V_3F3F3F2F;

    if ( Settings::Get( ).mSaveScene )
    {
        agregator->mRawVB = vBuf;
        agregator->mRawIB = iBuf;
    }

    return agregator;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11Buffer* D3DGeometryBuffer::GetVB( ) const
{
    ASSERT( mVB );
    return mVB;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
ID3D11Buffer* D3DGeometryBuffer::GetIB( ) const
{
    ASSERT( mIB );
    return mIB;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int D3DGeometryBuffer::GetIndexCount( ) const
{
    ASSERT( mIndexCount );
    return mIndexCount;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<Vertex3F3F3F2F>& D3DGeometryBuffer::GetRawVB( )
{
    return mRawVB;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::vector<uint32_t>& D3DGeometryBuffer::GetRawIB( )
{
    return mRawIB;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::set<D3DGeometryBuffer*>& D3DGeometryBuffer::GetStorage()
{
    return mInternalStorage;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void D3DGeometryBuffer::FillGeometryBufferAgregator( std::shared_ptr<D3DGeometryBuffer> &agregator,
    const std::vector<Vertex3F3F3F2F> &vBuf, const std::vector<uint32_t> &iBuf )
{
    D3DRenderer &renderer = D3DRenderer::Get( );
    auto device = renderer.GetDevice();

    D3D11_BUFFER_DESC vbd;
    vbd.Usage = D3D11_USAGE_IMMUTABLE;
    vbd.ByteWidth = vBuf.size( ) * sizeof( Vertex3F3F3F2F );
    vbd.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    vbd.CPUAccessFlags = 0;
    vbd.MiscFlags = 0;
    vbd.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA vinitData;
    vinitData.SysMemPitch = 0;
    vinitData.SysMemSlicePitch = 0;
    vinitData.pSysMem = &vBuf[0];
    HRESULT hr = device->CreateBuffer( &vbd, &vinitData, &agregator->mVB );
    ASSERT( hr == S_OK );

    D3D11_BUFFER_DESC ibd;
    ibd.Usage = D3D11_USAGE_IMMUTABLE;
    ibd.ByteWidth = iBuf.size( ) * sizeof( uint32_t );
    ibd.BindFlags = D3D11_BIND_INDEX_BUFFER;
    ibd.CPUAccessFlags = 0;
    ibd.MiscFlags = 0;
    ibd.StructureByteStride = 0;

    D3D11_SUBRESOURCE_DATA iinitData;
    iinitData.SysMemPitch = 0;
    iinitData.SysMemSlicePitch = 0;
    iinitData.pSysMem = &iBuf[0];
    hr = device->CreateBuffer( &ibd, &iinitData, &agregator->mIB );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3DGeometryBuffer::D3DGeometryBuffer()
{
    mInternalStorage.insert( this );

    ID3D11Buffer *mVB = nullptr;
    ID3D11Buffer *mIB = nullptr;
    int mIndexCount = 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
D3DGeometryBuffer::~D3DGeometryBuffer( )
{
    mInternalStorage.erase( this );

    COMSafeRelease( mVB );
    COMSafeRelease( mIB );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
