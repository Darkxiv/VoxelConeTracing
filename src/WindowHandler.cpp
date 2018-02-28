#include <WindowHandler.h>
#include <GlobalUtils.h>
#include <Settings.h>

InputHandler WindowHandler::mInputHandler;
std::function< bool( HWND, UINT, UINT ) > WindowHandler::mResizeCallback = nullptr;

WindowHandler::WindowHandler( ) :
    initialized( false ),
    hInst( nullptr ),
    hWnd( nullptr )
{}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HRESULT WindowHandler::InitWindow( HINSTANCE hInstance, int nCmdShow )
{
    ASSERT( initialized == false );
    if ( initialized )
        return E_FAIL;

    // Register class
    WNDCLASSEX wcex;
    wcex.cbSize = sizeof( WNDCLASSEX );
    wcex.style = CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc = WndProc;
    wcex.cbClsExtra = 0;
    wcex.cbWndExtra = 0;
    wcex.hInstance = hInstance;
    wcex.hIcon = LoadIcon( hInstance, IDI_WINLOGO );
    wcex.hCursor = LoadCursor( nullptr, IDC_ARROW );
    wcex.hbrBackground = ( HBRUSH )( COLOR_WINDOW + 1 );
    wcex.lpszMenuName = nullptr;
    wcex.lpszClassName = L"WindowClass";
    wcex.hIconSm = LoadIcon( wcex.hInstance, IDI_WINLOGO );
    if ( !RegisterClassEx( &wcex ) )
        return E_FAIL;

    // Create window
    Settings &settings = Settings::Get();
    hInst = hInstance;
    RECT rc = { 0, 0, settings.mWndWidth, settings.mWndHeight };
    AdjustWindowRect( &rc, WS_OVERLAPPEDWINDOW, FALSE );
    hWnd = CreateWindow( L"WindowClass", L"Global illumination via Voxel Cone Tracing",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        rc.right - rc.left, rc.bottom - rc.top, nullptr, nullptr, hInstance, nullptr );
    if ( !hWnd )
        return E_FAIL;

    ShowWindow( hWnd, nCmdShow );

    initialized = true;
    return S_OK;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowHandler::GetWindowMsg( _In_ const MSG *lpMsg )
{
    TranslateMessage( lpMsg );
    DispatchMessage( lpMsg );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
HWND WindowHandler::GetWnd( )
{
    return hWnd;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
InputHandler& WindowHandler::GetInputHandler( )
{
    return mInputHandler;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowHandler::RegisterResizeCallback( std::function< bool( HWND, UINT, UINT ) > &function )
{
    mResizeCallback = function;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void WindowHandler::CleanUp( )
{
    mInputHandler.CleanUp();
    mResizeCallback = nullptr;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
LRESULT CALLBACK WindowHandler::WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    switch ( message )
    {
        // WM_ACTIVATE callback
        // WM_ENTERSIZEMOVE callback
    case WM_SIZE:
        {
            if ( mResizeCallback )
            {
                if ( !mResizeCallback( hWnd, ( UINT )LOWORD( lParam ), ( UINT )HIWORD( lParam ) ) )
                    DestroyWindow( hWnd );
            }
        }
        break;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_MOUSEWHEEL:
    case WM_MOUSEMOVE:
    case WM_KEYUP:
    case WM_KEYDOWN:
        mInputHandler.ProcessedInput( hWnd, message, wParam, lParam );
        break;
    case WM_PAINT:
        {
            PAINTSTRUCT ps;
            HDC hdc;
            hdc = BeginPaint( hWnd, &ps );
            EndPaint( hWnd, &ps );
        }
        break;
    case WM_DESTROY:
        PostQuitMessage( 0 );
        break;
    default:
        return DefWindowProc( hWnd, message, wParam, lParam );
    }

    return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
