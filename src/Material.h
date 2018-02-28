#ifndef __MATERIAL_H
#define __MATERIAL_H

#include <string.h>
#include <memory>
#include <DirectXMath.h>

class D3DTextureBuffer2D;

struct Material
{
    // render options
    bool mVisible;
    std::string mName;

    // visual settings
    DirectX::XMFLOAT3 mAmbient;
    DirectX::XMFLOAT3 mDiffuse;
    DirectX::XMFLOAT3 mSpecular;
    // shaders?

    std::shared_ptr<D3DTextureBuffer2D> tex0;
    std::shared_ptr<D3DTextureBuffer2D> tex1;

    Material():
        mVisible( true ),
        mAmbient( DirectX::XMFLOAT3( 0.0f, 0.0f, 0.0f ) ),
        mDiffuse( DirectX::XMFLOAT3( 0.0f, 0.0f, 0.0f ) ),
        mSpecular( DirectX::XMFLOAT3( 0.0f, 0.0f, 0.0f ) ),
        mName( "" )
    { }

    void Clear()
    {
        mVisible = true;
        mAmbient = DirectX::XMFLOAT3( 0.0f, 0.0f, 0.0f );
        mDiffuse = DirectX::XMFLOAT3( 0.0f, 0.0f, 0.0f );
        mSpecular = DirectX::XMFLOAT3( 0.0f, 0.0f, 0.0f );
        tex0.reset();
        tex1.reset();
    }
};

#endif