#include <FXBindings/FXImGui.h>
#include <GlobalUtils.h>
#include <D3DRenderer.h>
#include <d3dx11effect.h>
#include <imgui.h>
#include <D3DTextureBuffer2D.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FXImGui::~FXImGui()
{
    COMSafeRelease( mFX );
    COMSafeRelease( mInputLayoutP2T2C4 );
    COMSafeRelease( mVB );
    COMSafeRelease( mIB );
    ImGui::Shutdown( );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FXImGui::Load()
{
    D3DRenderer &renderer = D3DRenderer::Get( );
    HRESULT hr = renderer.CreateEffect( "ImGui.cso", &mFX );

    if ( hr == S_OK )
    {
        bool fxCheck = true;

        GET_FX_VAR( fxCheck, mTech, mFX->GetTechniqueByName( "ImGui" ) );
        if ( mTech->IsValid( ) )
            GET_FX_VAR( fxCheck, mfxDrawInterface, mTech->GetPassByName( "DrawInterface" ) );

        GET_FX_VAR( fxCheck, mfxProjectionMatrix, mFX->GetVariableByName( "ProjectionMatrix" )->AsMatrix( ) );
        GET_FX_VAR( fxCheck, mfxTexture, mFX->GetVariableByName( "texture0" )->AsShaderResource( ) );

        mIsLoaded = fxCheck;
    }
    ASSERT( mIsLoaded );

    if ( mIsLoaded )
    {
        D3D11_INPUT_ELEMENT_DESC local_layout[] = {
            { "POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, ( size_t )( &( ( ImDrawVert* )0 )->pos ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, ( size_t )( &( ( ImDrawVert* )0 )->uv ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
            { "COLOR", 0, DXGI_FORMAT_R8G8B8A8_UNORM, 0, ( size_t )( &( ( ImDrawVert* )0 )->col ), D3D11_INPUT_PER_VERTEX_DATA, 0 },
        };

        auto d3dDevice = renderer.GetDevice( );
        D3DX11_PASS_DESC passDesc;
        mTech->GetPassByIndex( 0 )->GetDesc( &passDesc );
        hr = d3dDevice->CreateInputLayout( local_layout, 3, passDesc.pIAInputSignature, passDesc.IAInputSignatureSize, &mInputLayoutP2T2C4 );
        ASSERT( hr == S_OK );

        if ( hr != S_OK )
            mIsLoaded = false;
    }

    return mIsLoaded;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FXImGui::Draw( ImDrawData* draw_data )
{
    // This is the main rendering function that you have to implement and provide to ImGui (via setting up 'RenderDrawListsFn' in the ImGuiIO structure)
    // If text or lines are blurry when integrating ImGui in your engine:
    // - in your Render function, try translating your projection matrix by (0.5f,0.5f) or (0.375f,0.375f)

    ASSERT( mIsLoaded );

    if ( !mIsLoaded )
        return;

    D3DRenderer &renderer = D3DRenderer::Get( );
    ID3D11Device *device = renderer.GetDevice( );
    ID3D11DeviceContext* ctx = renderer.GetContext( );

    // Create and grow vertex/index buffers if needed
    if ( !mVB || mVBufferSize < draw_data->TotalVtxCount )
    {
        if ( mVB ) {
            mVB->Release( ); mVB = NULL;
        }
        mVBufferSize = draw_data->TotalVtxCount + 5000;
        D3D11_BUFFER_DESC desc;
        memset( &desc, 0, sizeof( D3D11_BUFFER_DESC ) );
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = mVBufferSize * sizeof( ImDrawVert );
        desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        desc.MiscFlags = 0;
        if ( device->CreateBuffer( &desc, NULL, &mVB ) < 0 )
            return;
    }
    if ( !mIB || mIBufferSize < draw_data->TotalIdxCount )
    {
        if ( mIB ) {
            mIB->Release( ); mIB = NULL;
        }
        mIBufferSize = draw_data->TotalIdxCount + 10000;
        D3D11_BUFFER_DESC desc;
        memset( &desc, 0, sizeof( D3D11_BUFFER_DESC ) );
        desc.Usage = D3D11_USAGE_DYNAMIC;
        desc.ByteWidth = mIBufferSize * sizeof( ImDrawIdx );
        desc.BindFlags = D3D11_BIND_INDEX_BUFFER;
        desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        if ( device->CreateBuffer( &desc, NULL, &mIB ) < 0 )
            return;
    }

    // Copy and convert all vertices into a single contiguous buffer
    D3D11_MAPPED_SUBRESOURCE vtx_resource, idx_resource;
    if ( ctx->Map( mVB, 0, D3D11_MAP_WRITE_DISCARD, 0, &vtx_resource ) != S_OK )
        return;
    if ( ctx->Map( mIB, 0, D3D11_MAP_WRITE_DISCARD, 0, &idx_resource ) != S_OK )
        return;
    ImDrawVert* vtx_dst = ( ImDrawVert* )vtx_resource.pData;
    ImDrawIdx* idx_dst = ( ImDrawIdx* )idx_resource.pData;
    for ( int n = 0; n < draw_data->CmdListsCount; n++ )
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        memcpy( vtx_dst, cmd_list->VtxBuffer.Data, cmd_list->VtxBuffer.Size * sizeof( ImDrawVert ) );
        memcpy( idx_dst, cmd_list->IdxBuffer.Data, cmd_list->IdxBuffer.Size * sizeof( ImDrawIdx ) );
        vtx_dst += cmd_list->VtxBuffer.Size;
        idx_dst += cmd_list->IdxBuffer.Size;
    }
    ctx->Unmap( mVB, 0 );
    ctx->Unmap( mIB, 0 );

    // Setup orthographic projection matrix into our constant buffer
    {
        float L = 0.0f;
        float R = ImGui::GetIO( ).DisplaySize.x;
        float B = ImGui::GetIO( ).DisplaySize.y;
        float T = 0.0f;
        DirectX::XMFLOAT4X4 tmpMat = DirectX::XMFLOAT4X4(
            2.0f / ( R - L ), 0.0f, 0.0f, ( R + L ) / ( L - R ),
            0.0f, 2.0f / ( T - B ), 0.0f, ( T + B ) / ( B - T ),
            0.0f, 0.0f, 0.5f, 0.5f,
            0.0f, 0.0f, 0.0f, 1.0f );
        DirectX::XMMATRIX projMat = DirectX::XMLoadFloat4x4( &tmpMat );
        mfxProjectionMatrix->SetMatrix( reinterpret_cast<float*>( &projMat ) );
    }

    // Bind shader and vertex buffers
    unsigned int stride = sizeof( ImDrawVert );
    unsigned int offset = 0;

    ctx->IASetInputLayout( mInputLayoutP2T2C4 );
    ctx->IASetVertexBuffers( 0, 1, &mVB, &stride, &offset );
    ctx->IASetIndexBuffer( mIB, sizeof( ImDrawIdx ) == 2 ? DXGI_FORMAT_R16_UINT : DXGI_FORMAT_R32_UINT, 0 );
    ctx->IASetPrimitiveTopology( D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST );

    // Render command lists
    int vtx_offset = 0;
    int idx_offset = 0;
    for ( int n = 0; n < draw_data->CmdListsCount; n++ )
    {
        const ImDrawList* cmd_list = draw_data->CmdLists[n];
        for ( int cmd_i = 0; cmd_i < cmd_list->CmdBuffer.Size; cmd_i++ )
        {
            const ImDrawCmd* pcmd = &cmd_list->CmdBuffer[cmd_i];
            if ( pcmd->UserCallback )
            {
                pcmd->UserCallback( cmd_list, pcmd );
            }
            else
            {
                const D3D11_RECT r = { ( LONG )pcmd->ClipRect.x, ( LONG )pcmd->ClipRect.y, ( LONG )pcmd->ClipRect.z, ( LONG )pcmd->ClipRect.w };
                ID3D11ShaderResourceView *srv = (*( std::shared_ptr<D3DTextureBuffer2D>* )pcmd->TextureId)->GetSRV();
                mfxTexture->SetResource( srv );
                mfxDrawInterface->Apply( 0, ctx );

                ctx->RSSetScissorRects( 1, &r );
                ctx->DrawIndexed( pcmd->ElemCount, idx_offset, vtx_offset );
            }
            idx_offset += pcmd->ElemCount;
        }
        vtx_offset += cmd_list->VtxBuffer.Size;
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
GeneralFX::FXType FXImGui::GetType()
{
    return mType;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FXImGui::IsLoaded()
{
    return mIsLoaded;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
