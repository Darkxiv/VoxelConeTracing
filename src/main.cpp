#include <stdlib.h>
#include <windows.h>
#include <d3d11_1.h>
#include <D3DRenderer.h>
#include <GameTimer.h>
#include <WindowHandler.h>
#include <Camera.h>
#include <Scene.h>
#include <Settings.h>
#include <direct.h>

#include <string>
#include <sstream>
#include <iomanip>

#include <functional>
#include <chrono>
#include <thread>

//
// This is a simple DX11.1 render engine created to study modern graphics technologies. Particularly Voxel Cone Tracing.
// It's easy to use. It has a basic obj/mtl loader, supports static scene, uses effect11 framework and precompiled fx-shaders.
// I have added abstracting comments and draw several schemes to simplify codes reading.
// If you'd like to program any technique you can start from DefaultShader and RenderTick.
//
// Author: Dontsov Valentin
//

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool SetupWorkingDirectory()
{
    // set working directory one level above from *.exe
    char szFileName[MAX_PATH + 1];
    GetModuleFileNameA( NULL, szFileName, MAX_PATH + 1 );

    // get executable directory
    std::string fn = szFileName;
    std::string path, file;
    SplitFilename( fn, path, file );

    path = path.substr( 0, path.length() - 1 );
    std::size_t found = path.find_last_of( "/\\" );
    if ( found != std::string::npos )
        path = path.substr( 0, found + 1 );

    // set new working directory
    return _chdir( path.c_str() ) == 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
int WINAPI wWinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPWSTR lpCmdLine, _In_ int nCmdShow )
{
    UNREFERENCED_PARAMETER( hPrevInstance );
    UNREFERENCED_PARAMETER( lpCmdLine );

#ifdef _DEBUG
    _CrtSetDbgFlag( _CRTDBG_ALLOC_MEM_DF | _CRTDBG_LEAK_CHECK_DF );
#endif

    // change working directory
    SetupWorkingDirectory( );

    WindowHandler wHandler;
    if ( FAILED( wHandler.InitWindow( hInstance, nCmdShow ) ) )
        return 0;

    // init renderer
    D3DRenderer &renderer = D3DRenderer::Get( );
    HWND hwnd = wHandler.GetWnd();
    if ( !renderer.Init( hwnd ) )
    {
        MessageBoxA( hwnd, "Can't initialize renderer", "Error", MB_OK );
        renderer.Cleanup();
        return 0;
    }

    Scene scene; // load scene

    // setup camera and renderer callbacks
    std::function< void( float, float ) >        CamRotCallback = std::bind( &Scene::ChangeCamRot, &scene, std::placeholders::_1, std::placeholders::_2 );
    std::function< void( CamDirection ) >     SetCamDirCallback = std::bind( &Scene::SetCamDirection, &scene, std::placeholders::_1 );
    std::function< void( CamDirection ) > ReleaseCamDirCallback = std::bind( &Scene::ReleaseCamDirection, &scene, std::placeholders::_1 );
    std::function< bool( HWND, UINT, UINT ) >    ResizeCallback = std::bind( &D3DRenderer::Resize, &renderer, std::placeholders::_1, std::placeholders::_2, std::placeholders::_3 );

    InputHandler &iHandler = wHandler.GetInputHandler();
    iHandler.RegisterCamRotCallback( CamRotCallback );
    iHandler.RegisterSetCamDirCallback( SetCamDirCallback );
    iHandler.RegisterReleaseCamDirCallback( ReleaseCamDirCallback );
    wHandler.RegisterResizeCallback( ResizeCallback );

    // start timer
    Settings &settings = Settings::Get( );
    GameTimer &appTimer = GameTimer::GetAppTimer();
    appTimer.Start();
    float timeElapsed = 0;

    // main message loop
    MSG msg = { 0 };
    while ( WM_QUIT != msg.message )
    {
        if ( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) )
        {
            wHandler.GetWindowMsg( &msg );
        }
        else
        {
            // push scene to render
            scene.Update();

            // draw scene
            renderer.RenderTick();

            // update timer
            appTimer.Tick( );
            if ( appTimer.GetDeltaTime( ) < settings.mFrameDeltaTime )
            {
                int waitFor = static_cast< int >( ( settings.mFrameDeltaTime - appTimer.GetDeltaTime( ) ) * 1000 );
                std::this_thread::sleep_for( std::chrono::milliseconds( waitFor ) );
            }

            if ( appTimer.GetLiveTime( ) - timeElapsed > 1.0 )
            {
                int FPS = static_cast< int >( 1.0f / appTimer.GetDeltaTime( ) );
                std::ostringstream title;
                title.precision( 5 );
                title << std::fixed << "Voxel Cone Tracing FPS: " << std::setw( 6 ) << FPS; // << " Frame Time: " << appTimer.GetDeltaTime();
                SetWindowTextA( hwnd, title.str( ).c_str( ) );
                timeElapsed = appTimer.GetLiveTime( );
            }
        }
    }

    scene.CleanUp();
    wHandler.CleanUp();
    renderer.Cleanup();

    return ( int )msg.wParam;
}
