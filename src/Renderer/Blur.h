#ifndef __BLUR_H
#define __BLUR_H

#include <memory>
#include <FXBindings/FXBlur.h>

class D3DTextureBuffer2D;

class Blur
{
public:
    Blur( ) = default;

    bool Init( );
    bool IsReady( );

    // generate depth-based blur, use separate X Y passes and need tmp texture
    void UpscaleBlur( std::shared_ptr<D3DTextureBuffer2D> &depth, std::shared_ptr<D3DTextureBuffer2D> &smallTex,
        std::shared_ptr<D3DTextureBuffer2D> &bigTexTmp, std::shared_ptr<D3DTextureBuffer2D> &bigTexRT );

private:
    bool mIsReady = false;

    FXBlur mfx;
};

#endif