#ifndef __DEFAULT_SHADER_H
#define __DEFAULT_SHADER_H

#include <vector>
#include <memory>
#include <FXBindings/FXDefault.h>

struct SceneGeometry;
struct ID3D11InputLayout;
class D3DTextureBuffer2D;

class DefaultShader
{
public:
    DefaultShader() = default;

    bool Init();
    void Clear();

    bool IsReady();
    void Draw( std::shared_ptr<D3DTextureBuffer2D> &colorRT, std::shared_ptr<D3DTextureBuffer2D> &depthRT,
        const std::vector<SceneGeometry> &objs, bool showAlpha = false );
    void Draw( std::shared_ptr<D3DTextureBuffer2D> &colorRT, std::shared_ptr<D3DTextureBuffer2D> &depthRT,
        SceneGeometry &obj, bool showAlpha = false );

    ID3D11InputLayout* GetDefaultInputLayout();

private:
    bool BuildLayout();

    bool mIsReady = false;
    FXDefault mfx;
    ID3D11InputLayout *mInputLayout = nullptr;
};

#endif