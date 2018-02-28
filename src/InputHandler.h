#ifndef __INPUT_HANDLER_H
#define __INPUT_HANDLER_H

#include <windows.h>
#include <functional>

enum CamDirection;

class InputHandler
{
public:
    InputHandler();

    void RegisterCamRotCallback( std::function< void( float, float ) > &function );
    void RegisterSetCamDirCallback( std::function< void( CamDirection ) > &function );
    void RegisterReleaseCamDirCallback( std::function< void( CamDirection ) > &function );

    void ProcessedInput( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );

    void CleanUp();

private:
    bool mMouseCaptured;
    POINT mLastMousePos;
    short mLastWheelPos;

    std::function< void( float, float ) > mCamRotCallback;
    std::function< void( CamDirection ) > mSetCamDirCallback;
    std::function< void( CamDirection ) > mReleaseCamDirCallback;
};

#endif