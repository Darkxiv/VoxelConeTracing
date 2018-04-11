#include <UIDrawer.h>
#include <Settings.h>
#include <imgui.h>
#include <GlobalUtils.h>
#include <D3DRenderer.h>
#include <D3DTextureBuffer2D.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool UIDrawer::Init( HWND hWnd )
{
    mIsReady = mfx.Load( );
    ASSERT( mIsReady );

    // init imgui
    ImGuiIO& io = ImGui::GetIO( );
    io.RenderDrawListsFn = nullptr;
    io.IniFilename = nullptr;
    io.ImeWindowHandle = hWnd;

    CreateImGuiFontTex( );

    return mIsReady;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void UIDrawer::Clear()
{
    mImGuiFontTex.reset();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool UIDrawer::IsReady()
{
    return mIsReady;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void UIDrawer::Draw()
{
    if ( !mIsReady )
        return;

    D3DRenderer &renderer = D3DRenderer::Get( );
    Settings &settings = Settings::Get( );

    ImGuiIO& io = ImGui::GetIO( );
    io.DisplaySize = ImVec2( static_cast< float >( renderer.GetWidth() ), static_cast< float >( renderer.GetHeight() ) );
    io.DeltaTime = settings.mFrameDeltaTime;

    // Read keyboard modifiers inputs
    io.KeyCtrl = ( GetKeyState( VK_CONTROL ) & 0x8000 ) != 0;
    io.KeyShift = ( GetKeyState( VK_SHIFT ) & 0x8000 ) != 0;
    io.KeyAlt = ( GetKeyState( VK_MENU ) & 0x8000 ) != 0;
    io.KeySuper = false;

    // Hide OS mouse cursor if ImGui is drawing it
    if ( io.MouseDrawCursor )
        SetCursor( NULL );

    // Start the frame
    ImGui::NewFrame( ); // should be in the begin of frame

    // frame
    static bool show_test_window = false;
    static bool showVCTDebug = true;
    static ImVec4 clear_col = ImColor( 114, 144, 154 );

    ImGui::SetNextWindowSize( ImVec2( 300, 450 ), ImGuiCond_FirstUseEver );
    ImGui::SetNextWindowPos( ImVec2( 20, 20 ), ImGuiCond_FirstUseEver );
    ImGui::Begin( "Settings", &showVCTDebug, ImGuiWindowFlags_NoSavedSettings );
    BuildUIOutputUI();

    switch ( mCurrentOutput )
    {
    case UIO_RENDERER:
        if ( renderer.IsGIEnabled() )
            BuildRendererUI( );
        BuildVCTUI( );
        break;
    case UIO_SCENE:
        BuildSceneUI();
        break;
    default:
        break;
    }

    ImGui::End( );
    ImGui::Render( );
    ImDrawData *drawData = ImGui::GetDrawData( );
    mfx.Draw( drawData );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void UIDrawer::CreateImGuiFontTex( )
{
    // Build texture atlas
    ImGuiIO& io = ImGui::GetIO( );
    unsigned char* pixels;
    int width, height;
    io.Fonts->GetTexDataAsRGBA32( &pixels, &width, &height );

    // Upload texture to graphics system
    D3D11_TEXTURE2D_DESC texDesc = D3DTextureBuffer2D::GenTexture2DDesc( width, height,
        DXGI_FORMAT_R8G8B8A8_UNORM, 1, 1, D3D11_USAGE_DEFAULT, 0, 0, D3D11_BIND_SHADER_RESOURCE, 1, 0 );

    D3D11_SUBRESOURCE_DATA subRes = D3DTextureBuffer2D::GenSubresourceData( pixels, width * 4, 0 );
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = D3DTextureBuffer2D::GenSRVDescTex2D( DXGI_FORMAT_R8G8B8A8_UNORM, 1, 0 );

    mImGuiFontTex = D3DTextureBuffer2D::CreateTextureSRV( &texDesc, &subRes, &srvDesc );
    ASSERT( mImGuiFontTex->GetSRV() );

    // Store identifier
    io.Fonts->TexID = reinterpret_cast< void* >( &mImGuiFontTex );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void UIDrawer::BuildUIOutputUI()
{
    const char* items[] = {
        "Renderer",
        "Scene",
    };

    static_assert( ARRAYSIZE( items ) == UIO_COUNT, "Items size doesn't match UIO_COUNT" );
    ImGui::Combo( "Configure:", reinterpret_cast< int* >( &mCurrentOutput ), items, UIO_COUNT );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void UIDrawer::BuildRendererUI()
{
    Settings &settings = Settings::Get();

    const char* items[] = {
        "Final",
        "Indirect",
        "Show brick buffer",
        "Show voxels",
    };

    static_assert( ARRAYSIZE( items ) == RO_COUNT, "Items size doesn't match RO_COUNT" );
    ImGui::Combo( "Output", reinterpret_cast< int* >( &settings.mRenderOutput ), items, RO_COUNT );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void UIDrawer::BuildSceneUI( )
{
    Settings &settings = Settings::Get( );

    ImGui::ColorEdit3( "Sun color", settings.mSunColor );
    ImGui::SliderFloat( "Sun power", &settings.mSunPower, 0.0f, 10.0f );
    ImGui::SliderFloat( "Sun yaw", &settings.mSunYaw, 0.0f, 6.2830f );
    ImGui::SliderFloat( "Sun pitch", &settings.mSunPitch, 0.0f, 1.570f );
    ImGui::SliderFloat( "Camera speed", &settings.mCameraSpeed, 0.0f, 1000.0f );
    ImGui::SliderFloat( "Mouse sens", &settings.mMouseSens, 0.0f, 0.02f );

    // hide unnecessary settings
    // ImGui::SliderFloat( "Light distance", &settings.mLightDistance, 0.0f, 2.0f );
    // ImGui::SliderFloat( "Shadow map z near", &settings.mShadowMapZNear, 0.0001f, 100.0f );
    // ImGui::SliderFloat( "Shadow map z far", &settings.mShadowMapZFar, settings.mShadowMapZNear + 3.0f, 20000.0f );
    // ImGui::SliderFloat( "Shadow bias", &settings.mShadowBias, 0, 5000.0f );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void UIDrawer::BuildVCTUI( )
{
    D3DRenderer &renderer = D3DRenderer::Get( );
    Settings &settings = Settings::Get( );

    switch ( settings.mRenderOutput )
    {
    case RenderOutput::RO_COLOR:
        {
            ImGui::SliderFloat( "Direct influence", &settings.mDirectInfluence, 0.0f, 1.0f );
            if ( renderer.IsGIEnabled() )
            {
                ImGui::Checkbox( "Enable GI", &settings.mVCTEnable );
                ImGui::SliderFloat( "AO influence", &settings.mAOInfluence, 0.0f, 1.0f );
                ImGui::SliderFloat( "GI influence", &settings.mIndirectInfluence, 0.0f, 1.0f );
                ImGui::SliderFloat( "GI amplification", &settings.mVCTIndirectAmplification, 0.0f, 10.0f );
            }
            else
            {
                ImGui::Text( "Global illumination is unsupported\nfor current device (DX11.1 required)" );
            }

            if ( settings.mLightAnimation )
            {
                if ( ImGui::Button( "Stop light animation" ) )
                    settings.mLightAnimation = false;
            }
            else
            {
                if ( ImGui::Button( "Run light animation" ) )
                    settings.mLightAnimation = true;
            }
        }
        break;
    case RenderOutput::RO_INDIRECT:
        {
            ImGui::Checkbox( "Enable GI", &settings.mVCTEnable );
            ImGui::Checkbox( "Show AO", &settings.mShowAO );
            ImGui::SliderFloat( "AO influence", &settings.mAOInfluence, 0.0f, 1.0f );
            ImGui::SliderFloat( "Lambda falloff", &settings.mVCTLambdaFalloff, 0.001f, 0.3f );

            ImGui::Text( "VCT" );
            // ImGui::SliderFloat( "Cone local offset", &settings.mVCTLocalConeOffset, 0.001f, 10.0f ); // TODO should depends from last level size
            ImGui::SliderFloat( "Cone world offset", &settings.mVCTWorldConeOffset, 0.000f, 20.0f ); // kick

            ImGui::SliderFloat( "GI amplification", &settings.mVCTIndirectAmplification, 0.0f, 10.0f );
            ImGui::SliderFloat( "Step correction", &settings.mVCTStepCorrection, 0.001f, 2.0f );
            ImGui::Checkbox( "Use opacity from buffer", &settings.mVCTUseOpacityBuffer );

            ImGui::SliderInt( "Octree first", &settings.mVCTDebugOctreeFirstLevel, 1, settings.mOctreeHeight - 1 ); // kick
            if ( settings.mVCTDebugOctreeLastLevel >= settings.mVCTDebugOctreeFirstLevel )
                settings.mVCTDebugOctreeLastLevel = settings.mVCTDebugOctreeFirstLevel - 1;
            ImGui::SliderInt( "Octree last", &settings.mVCTDebugOctreeLastLevel, 0, settings.mVCTDebugOctreeFirstLevel - 1 );

            ImGui::Text( "Debug cone" );
            ImGui::Checkbox( "Show debug", &settings.mVCTDebugView );
            ImGui::SliderInt( "Cone dir", &settings.mVCTDebugConeDir, 0, 4 );

            ImGui::SliderFloat( "Blur radius", &settings.mBlurRadius, 0.5f, 30.0f );
        }
        break;
    case RenderOutput::RO_BRICKS:
        {
            ImGui::Text( "Octree level" );
            ImGui::SliderInt( "Octree level", &settings.mVCTDebugOctreeLevel, 0, settings.mOctreeHeight - 1 );

            const char* debugBrickItems[] = { "Opacity", "Irradiance" };
            static_assert( ARRAYSIZE( debugBrickItems ) == DBUF_COUNT, "Items size doesn't match DBUF_COUNT" );
            ImGui::Combo( "DebugBrickBuffer", reinterpret_cast< int* >( &settings.mVCTDebugBuffer ), debugBrickItems,
                sizeof( debugBrickItems ) / sizeof( *debugBrickItems ) );
        }
        break;
    default:
        break;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
