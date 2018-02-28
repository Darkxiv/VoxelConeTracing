#ifndef __D3D_RENDERER_H
#define __D3D_RENDERER_H

#include <unordered_map>
#include <set>
#include <memory>
#include <windows.h>
#include <d3d11_1.h>
#include <DirectXMath.h>
#include <GlobalUtils.h>
#include <GBuffer.h>
#include <DefaultShader.h>
#include <ShadowMapper.h>
#include <VCT.h>
#include <UIDrawer.h>
#include <Blur.h>

class D3DTextureBuffer2D;
class D3DStructuredBuffer;
class D3DGeometryBuffer;
struct Material;
struct ID3D11Buffer;
struct ID3DX11Effect;
struct GGMeshData;
struct LightSource;
struct SceneGeometry;

// dx11 based renderer
class D3DRenderer
{
public:
    static D3DRenderer &Get();
    D3DRenderer( const D3DRenderer& ) = delete; // Prevent copy-construction
    D3DRenderer& operator=( const D3DRenderer& ) = delete; // Prevent assignment

    bool Init( HWND hWnd ); // predefined settings, hwnd instance, etc
    bool Resize( HWND hWnd, UINT width, UINT height );
    void Cleanup();
    void RenderTick();

    void SetDefaultViewport();
    void SetIndirectLayout( );
    void SetViewport( float w, float h, float minD, float maxD, float topLeftX, float topLeftY );
    void SetViewTransform( const DirectX::XMFLOAT4X4 &view );
    void PushSceneGeometryToRender( const SceneGeometry &geometry );
    void PushLigthToRender( const LightSource &light );

    void DrawGeometry( const std::shared_ptr<D3DGeometryBuffer> &geom );
    void CalcStaticSceneBB( const DirectX::XMFLOAT3 &vtx );

    HRESULT CreateEffect( const char *shaderName, ID3DX11Effect **fx );

    ID3D11Device* GetDevice();
    ID3D11DeviceContext* GetContext();

    void GetResolution( int &width, int &height );
    int GetWidth();
    int GetHeight();
    std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3> GetStaticSceneBB();
    std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3> GetStaticSceneBBRect();

    std::shared_ptr<D3DTextureBuffer2D> GetDefaultTexture();
    std::shared_ptr<D3DTextureBuffer2D> GetMainRT( );
    std::shared_ptr<D3DTextureBuffer2D> GetMainDepth( );
    ID3D11InputLayout* GetDefaultInputLayout();
    void GetViewProjMats( DirectX::XMFLOAT4X4 &view, DirectX::XMFLOAT4X4 &proj );
    void GetFullscreenQuadMats( DirectX::XMFLOAT4X4 &view, DirectX::XMFLOAT4X4 &proj );
    std::shared_ptr<Material> GetDefaultMaterial();
    std::shared_ptr<D3DGeometryBuffer> GetQuad();

    GBuffer&    GetGBuffer();
    VCT&        GetVCT();
    UIDrawer&   GetUIDrawer();
    Blur&       GetBlur();

    uint32_t GetValueFromCounter( std::shared_ptr<D3DStructuredBuffer> &buffer );
    bool IsGIEnabled();

private:
    D3DRenderer( );

    HRESULT CreateSwapChain( HWND hWnd, UINT width, UINT height );
    HRESULT CreateDevice( D3D_FEATURE_LEVEL *featureLevels, UINT numFeatureLevels );
    void ResetView();
    void ClearScene();

    void CreateDefaultTexture( );
    void CreateDefaultMaterial( );
    void CreateDefaultGeometry( );
    bool CreateCopyCounterBuffer( );
    bool CreateSyncQueries( );

    void SetFullscreenQuadMats();
    void SetDefaultMats();

    void ReportLiveObjects();
    void SendSyncQuery();
    void SyncFence();

    int mWidth;
    int mHeight;

    std::vector<SceneGeometry> mGeometryToRender;
    std::vector<LightSource> mLightToRender;

    // camera stuff
    DirectX::XMFLOAT4X4 mWorld;
    DirectX::XMFLOAT4X4 mView;
    DirectX::XMFLOAT4X4 mProj;

    DirectX::XMFLOAT4X4 mFullscreenQuadProj;
    DirectX::XMFLOAT4X4 mFullscreenQuadView;

    DirectX::XMFLOAT4X4 mDefaultProj;
    DirectX::XMFLOAT4X4 mDefaultView;

    D3D_DRIVER_TYPE mDriverType;
    D3D_FEATURE_LEVEL mFeatureLevel;
    ID3D11Device *md3dDevice;
    ID3D11Device1 *md3dDevice1;
    ID3D11DeviceContext *mImmediateContext;
    ID3D11DeviceContext1 *mImmediateContext1;
    IDXGISwapChain *mSwapChain;
    IDXGISwapChain1 *mSwapChain1;

    ID3D11RasterizerState *mNoCullRS;
    ID3D11RasterizerState *mCullRS;
    ID3D11DepthStencilState *mNoDepthNoStencilDS;
    ID3D11DepthStencilState *mDepthNoStencilDS;

    ID3D11Buffer *mCopyCounterBuffer;
    ID3D11Query *mSyncQueryA, *mSyncQueryB;
    bool mFirstFrame;
    HRESULT mLastPresentResult;

    D3D11_VIEWPORT curVP; // does it needs severals copies for MRT ?

    std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3> mStaticSceneBB; // <minBB, maxBB> calculated during mesh initializing

    // reserved textures
    std::shared_ptr<D3DTextureBuffer2D> mMainRT;
    std::shared_ptr<D3DTextureBuffer2D> mMainDepth;
    std::shared_ptr<D3DTextureBuffer2D> mDefaultTexture;

    // reserved material
    std::shared_ptr<Material> mDefaultMaterial;

    // reserved geometry
    std::shared_ptr<D3DGeometryBuffer> mQuad;

    // used render techniques
    DefaultShader mDefaultShader;
    GBuffer mGBuffer;
    ShadowMapper mShadowMapper;
    VCT mVCT;
    UIDrawer mUIDrawer;
    Blur mBlur;

    bool mGIEnabled;
};

#endif
