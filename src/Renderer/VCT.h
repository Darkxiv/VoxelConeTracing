#ifndef __VCT_H
#define __VCT_H

#include <memory>
#include <vector>
#include <Octree.h>
#include <DirectXMath.h>
#include <Light.h>
#include <FXBindings/FXGenerateOctree.h>
#include <FXBindings/FXGenerateBrickBuffer.h>
#include <FXBindings/FXConeTracing.h>

class D3DTextureBuffer2D;
class D3DTextureBuffer3D;
class D3DStructuredBuffer;
struct ID3D11Texture2D;
struct ID3D11Buffer;
struct SceneGeometry;
struct ShadowMap;

// voxel cone tracing class
class VCT
{
public:
    VCT( ) = default;

    bool Init( );
    void Clear( );

    bool IsReady( );
    bool NeedsVoxelization( );
    bool NeedsProcessLight( const LightSource &lsource );

    void VoxelizeStaticScene( const std::vector<SceneGeometry> &objs );
    //void VoxelizeDynamicScene( const std::vector<SceneGeometry> &objs );
    void ClearIrradianceBrickBuffer();
    void ProcessShadowMap( LightSource &lsource, ShadowMap &shadowMap );

    void VoxelConeTracing( );

    void DrawBuffers( bool showVoxels );

    Octree& GetOctree( );
    size_t GetFragmentListSize( );

    std::shared_ptr<D3DStructuredBuffer> GetIndirectDrawBuffer( );
    std::shared_ptr<D3DStructuredBuffer> GetVoxelArray( );
    std::shared_ptr<D3DTextureBuffer3D> GetOpacityBrickBuffer( );
    std::shared_ptr<D3DTextureBuffer3D> GetIrradianceBrickBuffer( );
    std::shared_ptr<D3DTextureBuffer2D> GetIndirectIrradianceSmall( );
    std::shared_ptr<D3DTextureBuffer2D> GetIndirectIrradiance( );

    size_t GetBrickBufferSize( );
    float GetLambdaFalloff( );
    float GetLocalConeOffset( );
    float GetWorldConeOffset( );
    float GetIndirectInfluence( );
    float GetStepCorrection( );
    bool GetUseOpacityBuffer( );

    int GetDebugOctreeLevel( );
    bool GetDebugView( );
    int GetDebugConeDir( );
    int GetDebugOctreeFirstLevel( );
    int GetDebugOctreeLastLevel( );

    size_t GetIndirectBufferOffset( size_t level ) const;

private:
    bool mIsReady = false;
    bool mNeedsVoxelization = true;

    size_t mFragmentListSize;
    std::weak_ptr<D3DStructuredBuffer> mFragmentCounter;
    std::shared_ptr<D3DStructuredBuffer> mIndirectDrawBuffer; // contains metadata for directx indirect draw (voxels count, nodes count per tree level)
    std::shared_ptr<D3DStructuredBuffer> mVoxelArray;
    LightSource mProcessedLight;

    size_t mBrickBufferSize;
    std::shared_ptr<D3DTextureBuffer3D> mOpacityBrickBuffer;
    std::shared_ptr<D3DTextureBuffer3D> mIrradianceBrickBuffer;

    std::shared_ptr<D3DTextureBuffer2D> mProcessShadowRT;
    std::shared_ptr<D3DTextureBuffer2D> mIndirectIrradianceSmall; // render indirect irradiance via VCT here
    std::shared_ptr<D3DTextureBuffer2D> mIndirectIrradianceBig; // final indirect irradiance texture
    std::shared_ptr<D3DTextureBuffer2D> mIndirectIrradianceBigTmp; // used for blur; can be replaced by mIndirectIrradianceBig and merged to gbuffer

    Octree mOctree;
    FXGenerateOctree mfxGenOctree;
    FXGenerateBrickBuffer mfxGenBrickBuffer;
    FXConeTracing mfxConeTracing;

    void GenOpacityBrickBuffer();
    void GenRadianceBrickBuffer( std::shared_ptr<D3DTextureBuffer3D> &texbuffer );

    void AverageBrickAlias( FXGenerateBrickBuffer *fx, ID3D11Buffer *indirectBuffer, size_t indirectBufferOffset );
    void AverageLitBrickAlias( FXGenerateBrickBuffer *fx, ID3D11Buffer *indirectBuffer, size_t indirectBufferOffset );
};

#endif