#ifndef __SCENE_H
#define __SCENE_H

#include <vector>
#include <set>
#include <array>
#include <unordered_map>
#include <string>
#include <memory>

#include <Camera.h>
#include <Light.h>

namespace DirectX
{
    struct XMFLOAT3;
}

class D3DTextureBuffer2D;
class D3DGeometryBuffer;
struct Material;
struct GGMeshData;
struct SceneGeometry;
struct Vertex3F3F3F2F;

// this should belong to engine class but we don't have that yet
enum CamDirection
{
    CD_UP,
    CD_DOWN,
    CD_FORWARD,
    CD_BACK,
    CD_LEFT,
    CD_RIGHT,
    CD_NONE
};

class Scene
{
public:
    Scene();
    ~Scene();

    void Update();
    bool LoadScene( const char *objFileName );
    bool LoadSceneFromBin( const char *binFileName );
    void SaveSceneToBin( const char *binFileName );
    void CleanUp();

    // this should belong to engine or camera class
    void ChangeCamRot( float dTheta, float dPhi );
    void SetCamDirection( CamDirection &dir );
    void ReleaseCamDirection( CamDirection &dir );
private:

    bool LoadMTL( const char *fn );
    bool ParseObjFile( const char *fn );
    void CreateNewObject( const std::string &name, const GGMeshData &data, const std::shared_ptr<Material> &mat );
    void CreateNewObject( const std::string &name, const std::string &matName,
        const std::vector<Vertex3F3F3F2F> &vBuf, const std::vector<uint32_t> &iBuf );
    bool CreateGeometryFromObj( const std::vector<DirectX::XMFLOAT3> &positions, const std::vector<DirectX::XMFLOAT3> &normals,
        const std::vector<DirectX::XMFLOAT3> &uvw, const std::vector<std::array<int, 9> > &faceInfo, GGMeshData &data );

    void AddMaterial( const Material &material );
    std::shared_ptr<Material> FindMaterial( const std::string &matName );
    void LoadMaterialTextureFromFile( const std::string &line, const std::string &fPath, const char *signature,
        std::shared_ptr<D3DTextureBuffer2D> &textureSlot );

    void UpdateSun( float dt );
    void UpdateCamDirection();

    bool mIsLoaded;

    Camera mMainCamera;
    float mLastTime;
    DirectX::XMFLOAT3 mCamMoveDir;
    std::set<CamDirection> mCamMoveDirSet;

    // render resources that belongs scene
    std::vector<std::shared_ptr<D3DGeometryBuffer>> mGeometryBuffers;
    std::unordered_map<std::string, std::shared_ptr<D3DTextureBuffer2D>> mTextures;
    std::unordered_map<std::string, std::shared_ptr<Material>> mMaterials;

    // scene objects compiles from scene resources
    std::vector<SceneGeometry> mSceneGeometries; // possibly better to store smart pointers?
    std::vector<std::string> mUsedMatLibs;

    LightSource mSun;
    float mSunOffset;
};

#endif