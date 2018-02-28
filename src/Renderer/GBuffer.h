#ifndef __GBUFFER_H
#define __GBUFFER_H

#include <vector>
#include <memory>
#include <DirectXMath.h>
#include <FXBindings/FXGBuffer.h>

struct LightSource;
struct ShadowMap;
struct SceneGeometry;
class D3DTextureBuffer2D;

class GBuffer
{
public:
    GBuffer() = default;

    bool Init( );
    void Clear( );

    bool IsReady( );
    void DrawGBuffer( const std::vector<SceneGeometry> &objs );
    void DrawCombine( LightSource &lSource, ShadowMap &shadowMap );

    std::shared_ptr<D3DTextureBuffer2D> GetColor( );
    std::shared_ptr<D3DTextureBuffer2D> GetNormal( );
    std::shared_ptr<D3DTextureBuffer2D> GetDepth( );

    void GetSceneViewProj( DirectX::XMFLOAT4X4 &view, DirectX::XMFLOAT4X4 &proj );

private:
    bool mIsReady = false;

    FXGBuffer mfx;

    std::shared_ptr<D3DTextureBuffer2D> mColor; // TODO is it really should be weak? it should be shared
    std::shared_ptr<D3DTextureBuffer2D> mNormal; // it needs to rework storage system
    std::shared_ptr<D3DTextureBuffer2D> mDepth;

    DirectX::XMFLOAT4X4 mSceneView;
    DirectX::XMFLOAT4X4 mSceneProj;
};

#endif