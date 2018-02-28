#ifndef __WINDOW_HANDLER_H
#define __WINDOW_HANDLER_H

#include <windows.h>
#include <functional>
#include <InputHandler.h>

class WindowHandler
{
public:
    WindowHandler();

    // SetOnResizeCallback

    HRESULT InitWindow( HINSTANCE hInstance, int nCmdShow );
    void GetWindowMsg( _In_ const MSG *lpMsg );

    HWND GetWnd();
    InputHandler& GetInputHandler();

    static void RegisterResizeCallback( std::function< bool( HWND, UINT, UINT ) > &function );

    void CleanUp();

private:
    static InputHandler mInputHandler;
    static std::function< bool( HWND, UINT, UINT ) > mResizeCallback;
    static LRESULT CALLBACK WndProc( HWND, UINT, WPARAM, LPARAM );

    HINSTANCE hInst;
    HWND hWnd;
    bool initialized;
};


#endif