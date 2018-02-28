#include <InputHandler.h>
#include <imgui.h>
#include <Settings.h>
#include <Scene.h>

void ImGui_InitKeys( );
bool ImGui_ImplDX11_WndProcHandler( UINT msg, WPARAM wParam, LPARAM lParam );
CamDirection VirtKeyToCamDirection( int virtKey );

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
InputHandler::InputHandler():
    mMouseCaptured( false ),
    mLastWheelPos( 0 ),
    mCamRotCallback( nullptr ),
    mSetCamDirCallback( nullptr ),
    mReleaseCamDirCallback( nullptr )
{
    mLastMousePos.x = 0;
    mLastMousePos.y = 0;

    ImGui_InitKeys();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void InputHandler::RegisterCamRotCallback( std::function< void( float, float ) > &function )
{
    mCamRotCallback = function;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void InputHandler::RegisterSetCamDirCallback( std::function< void( CamDirection ) > &function )
{
    mSetCamDirCallback = function;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void InputHandler::RegisterReleaseCamDirCallback( std::function< void( CamDirection ) > &function )
{
    mReleaseCamDirCallback = function;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void InputHandler::ProcessedInput( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    if ( message != WM_KEYUP && message != WM_KEYDOWN )
    {
        // UI can block input for viewport rotation
        if ( ImGui_ImplDX11_WndProcHandler( message, wParam, lParam ) )
            return;
    }

    switch ( message )
    {
    case WM_LBUTTONDOWN:
        {
            GetCursorPos( &mLastMousePos );
            SetCapture( hWnd );
            mMouseCaptured = true;
        }
        break;
    case WM_LBUTTONUP:
        {
            ReleaseCapture();
            mMouseCaptured = false;
        }
        break;
    case WM_MOUSEMOVE:
        {
            if ( mMouseCaptured )
            {
                POINT curPoint;
                GetCursorPos( &curPoint );

                // note: there is a question about camera orientation...
                Settings &settings = Settings::Get();
                float dTheta = -settings.mMouseSens * ( curPoint.x - mLastMousePos.x );
                float dPhi = -settings.mMouseSens * ( curPoint.y - mLastMousePos.y );
                mLastMousePos = curPoint;

                if ( mCamRotCallback )
                    mCamRotCallback( dTheta, dPhi );
            }
        }
        break;
    case WM_KEYUP:
        {
            int nVirtKey = wParam;

            // release cam directions
            CamDirection dir = VirtKeyToCamDirection( nVirtKey );
            if ( mReleaseCamDirCallback && dir != CD_NONE )
                mReleaseCamDirCallback( dir );
        }
        break;
    case WM_KEYDOWN:
        {
            int nVirtKey = wParam;
            int lKeyData = lParam;

            // https://msdn.microsoft.com/ru-ru/library/windows/desktop/ms646280(v=vs.85).aspx
            switch ( nVirtKey )
            {
            case 27:
                PostQuitMessage( 0 );
                break;
            }

            // set cam directions
            CamDirection dir = VirtKeyToCamDirection( nVirtKey );
            if ( mSetCamDirCallback && dir != CD_NONE )
                mSetCamDirCallback( dir );
        }
        break;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void InputHandler::CleanUp( )
{
    mCamRotCallback = nullptr;
    mSetCamDirCallback = nullptr;
    mReleaseCamDirCallback = nullptr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void ImGui_InitKeys( )
{
    ImGuiIO& io = ImGui::GetIO( );
    io.KeyMap[ImGuiKey_Tab] = VK_TAB;           // Keyboard mapping. ImGui will use those indices to peek
    io.KeyMap[ImGuiKey_LeftArrow] = VK_LEFT;    // into the io.KeyDown[] array that we will update during
    io.KeyMap[ImGuiKey_RightArrow] = VK_RIGHT;  // the application lifetime.
    io.KeyMap[ImGuiKey_UpArrow] = VK_UP;
    io.KeyMap[ImGuiKey_DownArrow] = VK_DOWN;
    io.KeyMap[ImGuiKey_PageUp] = VK_PRIOR;
    io.KeyMap[ImGuiKey_PageDown] = VK_NEXT;
    io.KeyMap[ImGuiKey_Home] = VK_HOME;
    io.KeyMap[ImGuiKey_End] = VK_END;
    io.KeyMap[ImGuiKey_Delete] = VK_DELETE;
    io.KeyMap[ImGuiKey_Backspace] = VK_BACK;
    io.KeyMap[ImGuiKey_Enter] = VK_RETURN;
    io.KeyMap[ImGuiKey_Escape] = VK_ESCAPE;
    io.KeyMap[ImGuiKey_A] = 'A';
    io.KeyMap[ImGuiKey_C] = 'C';
    io.KeyMap[ImGuiKey_V] = 'V';
    io.KeyMap[ImGuiKey_X] = 'X';
    io.KeyMap[ImGuiKey_Y] = 'Y';
    io.KeyMap[ImGuiKey_Z] = 'Z';
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool ImGui_ImplDX11_WndProcHandler( UINT msg, WPARAM wParam, LPARAM lParam )
{
    ImGuiIO& io = ImGui::GetIO( );

    switch ( msg )
    {
    case WM_LBUTTONDOWN:
        io.MouseDown[0] = true;
        break;
    case WM_LBUTTONUP:
        io.MouseDown[0] = false;
        break;
    case WM_RBUTTONDOWN:
        io.MouseDown[1] = true;
        break;
    case WM_RBUTTONUP:
        io.MouseDown[1] = false;
        break;
    case WM_MBUTTONDOWN:
        io.MouseDown[2] = true;
        break;
    case WM_MBUTTONUP:
        io.MouseDown[2] = false;
        break;
    case WM_MOUSEWHEEL:
        io.MouseWheel += GET_WHEEL_DELTA_WPARAM( wParam ) > 0 ? +1.0f : -1.0f;
        break;
    case WM_MOUSEMOVE:
        io.MousePos.x = static_cast<signed short>( lParam );
        io.MousePos.y = static_cast<signed short>( lParam >> 16 );
        break;
    case WM_CHAR:
        // You can also use ToAscii()+GetKeyboardState() to retrieve characters.
        if ( wParam > 0 && wParam < 0x10000 )
            io.AddInputCharacter( static_cast<signed short>(wParam) );
        break;
    }

    bool skipCapture = ( msg == WM_KEYUP ) || ( msg == WM_KEYDOWN );
    if ( ( io.WantCaptureMouse || io.WantCaptureKeyboard ) && !skipCapture )
    {
        return true;
    }

    return false;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
CamDirection VirtKeyToCamDirection( int virtKey )
{
    CamDirection dir = CD_NONE;

    switch ( virtKey )
    {
    case 32: // space
        dir = CD_UP;
        break;
    case 67: // c
        dir = CD_DOWN;
        break;
    case 87: // w
        dir = CD_FORWARD;
        break;
    case 83: // s
        dir = CD_BACK;
        break;
    case 65: // a
        dir = CD_LEFT;
        break;
    case 68: // d
        dir = CD_RIGHT;
        break;
    }

    return dir;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////