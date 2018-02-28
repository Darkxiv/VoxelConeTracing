#ifndef __UI_DRAWER_H
#define __UI_DRAWER_H

#include <FXBindings/FXImGui.h>
#include <memory>

class D3DTextureBuffer2D;
struct HWND__;
typedef HWND__* HWND;

class UIDrawer
{
public:
    UIDrawer() = default;

    bool Init( HWND hWnd );
    void Clear();

    bool IsReady( );

    void Draw(); // draw UI according to settings

private:
    bool mIsReady = false;

    std::shared_ptr<D3DTextureBuffer2D> mImGuiFontTex;

    FXImGui mfx;

    enum UIOutput
    {
        UIO_RENDERER,
        UIO_SCENE,

        UIO_COUNT
    } mCurrentOutput = UIO_RENDERER;

    void CreateImGuiFontTex();
    void BuildUIOutputUI();
    void BuildRendererUI();
    void BuildSceneUI();
    void BuildVCTUI();
};

#endif