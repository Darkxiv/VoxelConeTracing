#include <FXBindings/FXGenerateOctree.h>
#include <GlobalUtils.h>
#include <d3dx11effect.h>
#include <Camera.h>
#include <VCT.h>
#include <D3DRenderer.h>
#include <D3DStructuredBuffer.h>

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
FXGenerateOctree::~FXGenerateOctree( )
{
    COMSafeRelease( mFX );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FXGenerateOctree::Load( )
{
    D3DRenderer &renderer = D3DRenderer::Get( );
    HRESULT hr = renderer.CreateEffect( "voxelization.cso", &mFX );

    if ( hr == S_OK )
    {
        bool fxCheck = true;

        GET_FX_VAR( fxCheck, mGenOctree.mTech, mFX->GetTechniqueByName( "GenerateOctree" ) );
        if ( mGenOctree.mTech->IsValid() )
        {
            auto &mgo = mGenOctree;
            GET_FX_VAR( fxCheck, mgo.mCreateVoxelArray, mgo.mTech->GetPassByName( "CreateVoxelArray" ) );
            GET_FX_VAR( fxCheck, mgo.mFlagNodes, mgo.mTech->GetPassByName( "FlagNodes" ) );
            GET_FX_VAR( fxCheck, mgo.mSubdivideNodes, mgo.mTech->GetPassByName( "SubdivideNodes" ) );
            GET_FX_VAR( fxCheck, mgo.mConnectNeighbors, mgo.mTech->GetPassByName( "ConnectNeighbors" ) );
            GET_FX_VAR( fxCheck, mgo.mConnectNodesToVoxels, mgo.mTech->GetPassByName( "ConnectNodesToVoxels" ) );
        }

        GET_FX_VAR( fxCheck, mfxWorldView, mFX->GetVariableByName( "gWorldView" )->AsMatrix( ) );
        GET_FX_VAR( fxCheck, mfxOrthoProj, mFX->GetVariableByName( "gOrthoProj" )->AsMatrix( ) );
        GET_FX_VAR( fxCheck, mfxAlbedoTexture, mFX->GetVariableByName( "albedoTexture" )->AsShaderResource( ) );
        GET_FX_VAR( fxCheck, mfxNormalTexture, mFX->GetVariableByName( "normalTexture" )->AsShaderResource( ) );

        GET_FX_VAR( fxCheck, mfxUseNormalMap, mFX->GetVariableByName( "useNormalMap" )->AsScalar( ) );

        GET_FX_VAR( fxCheck, mfxVoxelArrayRW, mFX->GetVariableByName( "voxelArrayRW" )->AsUnorderedAccessView( ) );
        GET_FX_VAR( fxCheck, mfxVoxelArrayR, mFX->GetVariableByName( "voxelArrayR" )->AsShaderResource( ) );
        GET_FX_VAR( fxCheck, mfxIndirectDrawBuffer, mFX->GetVariableByName( "indirectDrawBuffer" )->AsUnorderedAccessView( ) );

        GET_FX_VAR( fxCheck, mfxFragmentBufferSize, mFX->GetVariableByName( "fragBufferSize" )->AsScalar( ) );
        GET_FX_VAR( fxCheck, mfxCurrentOctreeLevel, mFX->GetVariableByName( "currentOctreeLevel" )->AsScalar( ) );

        fxCheck &= mOctreeVariables.LoadFromFx( mFX );
        mIsLoaded = fxCheck;
    }

    ASSERT( mIsLoaded );

    return mIsLoaded;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FXGenerateOctree::SetupViewProj()
{
    ASSERT( mIsLoaded );
    if ( !mIsLoaded )
        return;

    D3DRenderer &renderer = D3DRenderer::Get( );
    std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3> sceneBB = renderer.GetStaticSceneBBRect( );
    DirectX::XMFLOAT3 &bba = sceneBB.first, &bbb = sceneBB.second;

    // set camera on the top of bounding box for the CreateVoxelArray pass
    Camera cam;
    cam.SetPosition( bba.x + ( bbb.x - bba.x ) * 0.5f,
                     bbb.y,
                     bba.z + ( bbb.z - bba.z ) * 0.5f );
    cam.SetDirection( 0.0f, -1.0f, 0.0f );
    DirectX::XMMATRIX tmpProj = DirectX::XMMatrixOrthographicRH(
        ( bbb.x - bba.x ), ( bbb.z - bba.z ), bba.y - EPS, bbb.y + EPS );

    DirectX::XMMATRIX view = DirectX::XMLoadFloat4x4( &cam.GetWorldToViewTransform( ) );
    mfxWorldView->SetMatrix( reinterpret_cast< float* >( &view ) );
    mfxOrthoProj->SetMatrix( reinterpret_cast< float* >( &tmpProj ) );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void FXGenerateOctree::BindVCTResources( VCT &vctResources )
{
    ASSERT( mIsLoaded );
    if ( !mIsLoaded )
        return;

    D3DRenderer &renderer = D3DRenderer::Get( );

    auto voxelArray = vctResources.GetVoxelArray();
    mfxVoxelArrayRW->SetUnorderedAccessView( voxelArray->GetUAV() );
    mfxVoxelArrayR->SetResource( voxelArray->GetSRV() );
    mfxIndirectDrawBuffer->SetUnorderedAccessView( vctResources.GetIndirectDrawBuffer( )->GetUAV() );

    mfxFragmentBufferSize->SetInt( vctResources.GetFragmentListSize( ) );

    std::pair<DirectX::XMFLOAT3, DirectX::XMFLOAT3> sceneBB = renderer.GetStaticSceneBBRect( );
    Octree &octree = vctResources.GetOctree( );
    mOctreeVariables.BindSceneBB( sceneBB );
    mOctreeVariables.BindOctree( octree );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
GeneralFX::FXType FXGenerateOctree::GetType()
{
    return mType;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool FXGenerateOctree::IsLoaded()
{
    return mIsLoaded;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

