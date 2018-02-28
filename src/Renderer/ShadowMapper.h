#ifndef __SHADOW_MAPPER_H
#define __SHADOW_MAPPER_H

#include <vector>
#include <memory>
#include <DirectXMath.h>
#include <FXBindings/FXShadowMap.h>

struct SceneGeometry;
struct LightSource;
class D3DTextureBuffer2D;

struct ShadowMap
{
    DirectX::XMFLOAT4X4 mView;
    DirectX::XMFLOAT4X4 mProj;
    std::shared_ptr<D3DTextureBuffer2D> mShadowTexture;
};

class ShadowMapper
{
public:
    ShadowMapper( ) = default;

    bool Init( );
    void Clear( );

    bool IsReady( );
    void Draw( const std::vector<SceneGeometry> &objs, const LightSource &light, const std::pair<float, float> &spaceSize );

    ShadowMap& GetShadowMap( );

private:
    bool mIsReady = false;
    FXShadowMap mfx;

    ShadowMap mShadowMap;
};

#endif