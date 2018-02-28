#include <Scene.h>
#include <GlobalUtils.h>
#include <D3DRenderer.h>
#include <D3DGeometryBuffer.h>
#include <D3DTextureBuffer2D.h>
#include <Material.h>
#include <SceneGeometry.h>
#include <GeometryGenerator.h>
#include <Light.h>
#include <Settings.h>
#include <GameTimer.h>

#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <array>
#include <unordered_map>

void LoadSingleObj( const char *fn, GGMeshData &data );

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Scene::Scene():
    mIsLoaded( false ),
    mLastTime( 0.0f ),
    mCamMoveDir( 0.0f, 0.0f, 0.0f ),
    mSunOffset( 0.0f )
{
    // load scene
    Settings &settings = Settings::Get( );
    bool sceneLoaded = LoadSceneFromBin( settings.mSceneFn );
    ASSERT( sceneLoaded );

    // save if needed
    if ( sceneLoaded && settings.mSaveScene && settings.mSaveSceneFn )
        SaveSceneToBin( settings.mSaveSceneFn );

    // set camera
    mMainCamera.SetPosition( settings.mInitCamPos[0], settings.mInitCamPos[1], settings.mInitCamPos[2] );
    mMainCamera.SetDegrees( settings.mInitCamTheta, settings.mInitCamPhi );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Scene::~Scene() { }
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Scene::Update()
{
    D3DRenderer &renderer = D3DRenderer::Get( );
    // update camera
    // set render camera

    // update scene geometry if needed

    // select primitives - NOTE place for k-d tree and other optimizations
    // submit primitives to renderer
    for each ( auto &obj in mSceneGeometries )
    {
        renderer.PushSceneGeometryToRender( obj );
    }

    GameTimer &timer = GameTimer::GetAppTimer( );
    Settings &settings = Settings::Get( );
    float dt = timer.GetLiveTime( ) - mLastTime;
    float offset = settings.mCameraSpeed * dt;
    mLastTime = timer.GetLiveTime( );

    UpdateSun( dt );
    renderer.PushLigthToRender( mSun );

    // update camera
    mMainCamera.ChangePosition( mCamMoveDir.x * offset, mCamMoveDir.y * offset, mCamMoveDir.z * offset );

    // update cameraPos according to current velocity and direction
    renderer.SetViewTransform( mMainCamera.GetWorldToViewTransform( ) );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Scene::LoadScene( const char *objFileName )
{
    // TODO check extension (.obj / .bin)
    // TODO if objFileName == NULL - load simple scene
    ASSERT( objFileName != NULL );

    if ( objFileName != NULL )
    {
        mIsLoaded = ParseObjFile( objFileName );
        ASSERT( mIsLoaded );

        return mIsLoaded;
    }
    return false;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Scene::SaveSceneToBin( const char *fn )
{
    // open ofstream
    std::ofstream binFile;
    binFile.open( fn, std::ofstream::binary );
    if ( binFile.fail( ) )
    {
        ASSERT( false );
        LOG_ERROR( "Error during opening file: ", fn );
        return;
    }

    auto writeSize = [&]( size_t size )
    {
        binFile.write( ( char* )( &size ), sizeof( size_t ) );
    };

    auto writeString = [&]( const std::string &str )
    {
        size_t len = str.length( );
        writeSize( len );
        binFile.write( str.c_str( ), len );
    };

    // save mUsedMaterials
    size_t mSize = mUsedMatLibs.size( );
    writeSize( mSize );
    for ( size_t i = 0; i < mUsedMatLibs.size( ); i++ )
        writeString( mUsedMatLibs[i] );

    D3DRenderer &renderer = D3DRenderer::Get( );

    //  save mSceneGeometries
    size_t oSize = mSceneGeometries.size( );
    writeSize( oSize );
    for ( size_t i = 0; i < oSize; i++ )
    {
        SceneGeometry &sg = mSceneGeometries[i];

        // save name
        writeString( sg.mName );

        // save matName
        writeString( sg.mMaterial->mName );

        auto &gb = sg.mGeometryBuffer;

        // save vtxs
        auto &rawVB = gb->GetRawVB();
        size_t vCount = rawVB.size( );
        writeSize( vCount );
        binFile.write( ( char* )( &rawVB[0] ), vCount * sizeof( Vertex3F3F3F2F ) );

        // save inds
        auto &rawIB = gb->GetRawIB();
        size_t iCount = rawIB.size( );
        writeSize( iCount );
        binFile.write( ( char* )( &rawIB[0] ), iCount * sizeof( uint32_t ) );
    }

    binFile.close( );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Scene::CleanUp()
{
    // unload scene
    mGeometryBuffers.clear( );
    mMaterials.clear( );
    mTextures.clear( );
    mSceneGeometries.clear( );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Scene::LoadSceneFromBin( const char *fn )
{
    // open ifstream
    std::ifstream binFile;
    binFile.open( fn, std::ifstream::binary );
    if ( binFile.fail( ) )
    {
        ASSERT( false );
        LOG_ERROR( "Error during opening file: ", fn );
        return false;
    }

    auto readSize = [&]( size_t &size )
    {
        binFile.read( ( char* )( &size ), sizeof( size_t ) );
    };

    auto readString = [&]( std::string &str )
    {
        const size_t bufSize = 256;
        size_t len;
        readSize( len );
        ASSERT( len < bufSize );
        if ( len >= bufSize )
            len = bufSize - 1;

        if ( len )
        {
            static char tmpStr[bufSize];
            binFile.read( tmpStr, len );
            tmpStr[len] = '\0';
            str = tmpStr;
        }
    };

    // load mUsedMaterials
    size_t mSize = 0;
    readSize( mSize );
    for ( size_t i = 0; i < mSize; i++ )
    {
        std::string matLibPath;
        readString( matLibPath );
        mUsedMatLibs.push_back( matLibPath );

        LoadMTL( matLibPath.c_str( ) );
    }

    D3DRenderer &renderer = D3DRenderer::Get( );

    //  load mSceneGeometries
    size_t oSize = 0;
    binFile.read( ( char* )( &oSize ), sizeof( size_t ) );
    for ( size_t i = 0; i < oSize; i++ )
    {
        std::string name, matName;
        readString( name );
        readString( matName );

        // load vtxs
        size_t vCount = 0;
        readSize( vCount );
        std::vector<Vertex3F3F3F2F> vBuf( vCount );
        binFile.read( ( char* )( &vBuf[0] ), vCount * sizeof( Vertex3F3F3F2F ) );

        // load inds
        size_t iCount = 0;
        readSize( iCount );
        std::vector<uint32_t> iBuf( iCount );
        binFile.read( ( char* )( &iBuf[0] ), iCount * sizeof( uint32_t ) );

        ASSERT( iCount > 0 && vCount > 0 );
        if ( iCount > 0 && vCount > 0 ) // && i > 0x100 && i < 0x150 )
            CreateNewObject( name, matName, vBuf, iBuf );
    }

    binFile.close( );

    return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Scene::ChangeCamRot( float dTheta, float dPhi )
{
    mMainCamera.ChangeDegrees( dTheta, dPhi );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Scene::SetCamDirection( CamDirection &dir )
{
    mCamMoveDirSet.insert( dir );
    UpdateCamDirection();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Scene::ReleaseCamDirection( CamDirection &dir )
{
    mCamMoveDirSet.erase( dir );
    UpdateCamDirection();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Scene::LoadMTL( const char *fn )
{
    std::ifstream mtlFile;
    mtlFile.open( fn, std::ifstream::in );
    if ( mtlFile.fail( ) )
    {
        ASSERT( false );
        LOG_ERROR( "Error during opening file: ", fn );
        return false;
    }

    std::string sfn = fn;
    mUsedMatLibs.push_back( sfn );

    std::string path, tmpstr;
    SplitFilename( sfn, path, tmpstr );

    char tmp;
    std::string line;
    Material material;
    bool firstObj = true;
    D3DRenderer &renderer = D3DRenderer::Get( );
    while ( !mtlFile.eof( ) )
    {
        std::getline( mtlFile, line );
        std::istringstream iss( line.c_str( ) );

        const char *skipSpaces = line.c_str( );
        while ( *skipSpaces == ' ' || *skipSpaces == '\t' )
            skipSpaces++;
        line = line.substr( skipSpaces - line.c_str( ) );

        if ( !line.compare( 0, 6, "newmtl" ) )
        {
            if ( !firstObj )
            {
                AddMaterial( material );
                material.Clear( );
            }
            else
            {
                firstObj = false;
            }

            // start new material
            material.Clear( );

            ASSERT( line.length( ) > strlen( "newmtl " ) );
            if ( line.length( ) > strlen( "newmtl " ) )
                material.mName = line.substr( strlen( "newmtl " ) );
            else
                material.mName = "";
        }
        else if ( !line.compare( 0, 3, "Ka " ) )
        {
            iss >> tmp >> tmp;
            float ka[3];
            for ( int i = 0; i < 3; i++ )
                iss >> ka[i];

            material.mAmbient = DirectX::XMFLOAT3( ka[0], ka[1], ka[2] );
        }
        else if ( !line.compare( 0, 3, "Kd " ) )
        {
            iss >> tmp >> tmp;
            float kd[3];
            for ( int i = 0; i < 3; i++ )
                iss >> kd[i];

            material.mDiffuse = DirectX::XMFLOAT3( kd[0], kd[1], kd[2] );
        }
        else if ( !line.compare( 0, 3, "Ks " ) )
        {
            iss >> tmp >> tmp;
            float ks[3];
            for ( int i = 0; i < 3; i++ )
                iss >> ks[i];

            material.mSpecular = DirectX::XMFLOAT3( ks[0], ks[1], ks[2] );
        }
        else if ( !line.compare( 0, 7, "map_Kd " ) )
        {
            LoadMaterialTextureFromFile( line, path, "map_Kd ", material.tex0 );
        }
        else if ( !line.compare( 0, 9, "map_bump " ) )
        {
            LoadMaterialTextureFromFile( line, path, "map_bump ", material.tex1 );
        }
    }

    std::shared_ptr<Material> mat = std::make_shared<Material>( material );
    mMaterials.insert( std::make_pair( material.mName, mat ) );

    return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// simple obj reader
// load scene
// read obj file
//  read mtllib X - load X lib
//  read object X - X is a name of object (can be null)
//      read geometry data till the next object
//          usemtl - store material
//          
//      before parsing next object - store current data to scene object
//  if there is no objects - end
bool Scene::ParseObjFile( const char *fn )
{
    std::ifstream objFile;
    objFile.open( fn, std::ifstream::in );
    if ( objFile.fail( ) )
    {
        ASSERT( false );
        LOG_ERROR( "Error during opening file: ", fn );
        return false;
    }

    // load scene
    std::vector<DirectX::XMFLOAT3> positions;
    std::vector<DirectX::XMFLOAT3> normals;
    std::vector<DirectX::XMFLOAT3> uvw;
    std::vector<std::array<int, 9> > faceInfo;

    std::string path, tmpstr;
    SplitFilename( std::string( fn ), path, tmpstr );

    char tmp;
    std::string line;
    std::string objName = "";
    GGMeshData geometryData;
    bool firstObj = true;
    D3DRenderer &renderer = D3DRenderer::Get( );
    std::shared_ptr<Material> curMat = renderer.GetDefaultMaterial( );
    int subObjCount = 0;
    std::string suffix = "0";

    while ( !objFile.eof( ) ) // && mSceneGeometries.size( ) < 10 )
    {
        std::getline( objFile, line );
        std::istringstream iss( line.c_str( ) );

        // new name is an indicator of the next object (the lack of name is possible)
        size_t objfound = line.find( "object" );
        if ( objfound != std::string::npos )
        {
            if ( !firstObj && CreateGeometryFromObj( positions, normals, uvw, faceInfo, geometryData ) )
            {
                CreateNewObject( objName + suffix, geometryData, curMat );
                faceInfo.clear( );
                subObjCount = 0;
                suffix = "0";
            }
            else
            {
                firstObj = false;
            }

            // start new object
            size_t cutlen = objfound + strlen( "object " );
            int nameLength = line.length( ) - cutlen;
            if ( nameLength > 0 )
                objName = line.substr( cutlen ); // , nameLength 
            else
                objName = "";

            curMat = renderer.GetDefaultMaterial( );
        }
        else if ( !line.compare( 0, 6, "mtllib" ) )
        {
            size_t cutlen = strlen( "mtllib " );
            int nameLength = line.length( ) - cutlen;
            std::string mtlfn = path + line.substr( cutlen );
            bool success = LoadMTL( mtlfn.c_str( ) );
            ASSERT( success );
            // load material lib
            // concat obj path and current path
        }
        else if ( !line.compare( 0, 7, "usemtl " ) )
        {
            if ( faceInfo.size( ) != 0 )
            {
                if ( CreateGeometryFromObj( positions, normals, uvw, faceInfo, geometryData ) )
                {
                    CreateNewObject( objName + suffix, geometryData, curMat );
                    faceInfo.clear( );
                }
                suffix = std::to_string( subObjCount++ );
            }

            std::string matName = line.substr( strlen( "usemtl " ) );
            curMat = FindMaterial( matName );
        }
        else if ( !line.compare( 0, 2, "v " ) )
        {
            iss >> tmp;
            float v[3];
            for ( int i = 0; i < 3; i++ )
                iss >> v[i];

            positions.push_back( DirectX::XMFLOAT3( v[0], v[1], v[2] ) );
        }
        else if ( !line.compare( 0, 3, "vt " ) )
        {
            iss >> tmp >> tmp;
            float vt[3];
            for ( int i = 0; i < 3; i++ )
                iss >> vt[i];

            uvw.push_back( DirectX::XMFLOAT3( vt[0], 1.0f - vt[1], vt[2] ) ); // v coordinate is inverted
        }
        else if ( !line.compare( 0, 3, "vn " ) )
        {
            iss >> tmp >> tmp;
            float vn[3];
            for ( int i = 0; i < 3; i++ )
                iss >> vn[i];

            normals.push_back( DirectX::XMFLOAT3( vn[0], vn[1], vn[2] ) );
        }
        else if ( !line.compare( 0, 2, "f " ) )
        {
            std::vector<std::array<int, 3> > vertices; // position, normal, texture
            int idv, idn, idt;
            iss >> tmp;
            while ( iss >> idv >> tmp >> idt >> tmp >> idn )
            {
                std::array<int, 3> vertex = { idv - 1, idt - 1, idn - 1 };
                vertices.push_back( vertex );
            }
            ASSERT( vertices.size( ) >= 3 );
            if ( vertices.size( ) < 3 )
                break;

            for ( size_t i = 2; i < vertices.size( ); i++ )
            {
                // assume more than 3 verticies connected to first vertex
                std::array<int, 9> face;
                for ( size_t j = 0; j < 3; j++ )
                    face[j] = vertices[0][j];

                for ( size_t j = i - 1; j <= i; j++ )
                {
                    for ( int k = 0; k < 3; k++ )
                        face[( 2 - ( i - j ) ) * 3 + k] = vertices[j][k];
                }

                faceInfo.push_back( face );
            }
        }
    }

    // TODO clear comment - last element issue ????
    if ( CreateGeometryFromObj( positions, normals, uvw, faceInfo, geometryData ) )
        CreateNewObject( objName + suffix, geometryData, curMat );

    return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Scene::CreateNewObject( const std::string &name, const GGMeshData &data, const std::shared_ptr<Material> &mat )
{
    auto &geometryBuffer = D3DGeometryBuffer::Create( data );
    mGeometryBuffers.push_back( geometryBuffer );

    SceneGeometry sceneGeometry( name, geometryBuffer, mat );
    mSceneGeometries.push_back( sceneGeometry );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Scene::CreateNewObject( const std::string &name, const std::string &matName,
    const std::vector<Vertex3F3F3F2F> &vBuf, const std::vector<uint32_t> &iBuf )
{
    auto &geometryBuffer = D3DGeometryBuffer::Create( vBuf, iBuf );
    mGeometryBuffers.push_back( geometryBuffer );

    std::shared_ptr<Material> mat = FindMaterial( matName );

    SceneGeometry sceneGeometry( name, geometryBuffer, mat );
    mSceneGeometries.push_back( sceneGeometry );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
bool Scene::CreateGeometryFromObj( const std::vector<DirectX::XMFLOAT3> &positions,
    const std::vector<DirectX::XMFLOAT3> &normals,
    const std::vector<DirectX::XMFLOAT3> &uvw,
    const std::vector<std::array<int, 9> > &faceInfo,
    GGMeshData &data )
{
    if ( faceInfo.size( ) == 0 || positions.size() == 0 || normals.size() == 0 || uvw.size() == 0 )
        return false;

    data.verticies.clear( );
    data.indicies.clear( );

    const int lengthLimit = 1 << 21;
    ASSERT( positions.size( ) < lengthLimit && normals.size( ) < lengthLimit && uvw.size( ) < lengthLimit );

    // build layout, prevent duplicates
    std::vector<std::vector<std::pair<int, int> > > vertexCoherence;
    std::unordered_map<__int64, int> indiciesHash;
    int maxIndex = 0;
    for each ( auto f in faceInfo )
    {
        int iv[3];
        for ( int i = 0; i < 3; i++ )
        {
            int vp = f[0 + i * 3], vt = f[1 + i * 3], vn = f[2 + i * 3];
            __int64 key = ( __int64 )vp + ( ( ( __int64 )vt ) << 21 ) + ( ( ( __int64 )vn ) << 42 );
            auto keyValue = indiciesHash.find( key );
            if ( keyValue != indiciesHash.end( ) )
            {
                data.indicies.push_back( keyValue->second );
            }
            else
            {
                // push new vertex
                GGVertex v;
                v.position = positions[vp];
                v.UVW = uvw[vt];
                v.normal = normals[vn];
                data.verticies.push_back( v );
                indiciesHash.insert( std::make_pair( key, maxIndex ) );
                data.indicies.push_back( maxIndex++ );
                vertexCoherence.push_back( std::vector<std::pair<int, int> >( ) );
            }
            iv[i] = data.indicies.back();
        }
        vertexCoherence[iv[0]].push_back( std::make_pair( iv[1], iv[2] ) );
        vertexCoherence[iv[1]].push_back( std::make_pair( iv[0], iv[2] ) );
        vertexCoherence[iv[2]].push_back( std::make_pair( iv[0], iv[1] ) );
    }
    GeometryGenerator::CalculateTBN( data, vertexCoherence );

    return true;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Scene::AddMaterial( const Material &material )
{
    D3DRenderer &renderer = D3DRenderer::Get( );

    std::shared_ptr<Material> newMat = std::make_shared<Material>( material );
    WARNING( !newMat->tex0 || !newMat->tex1, "Material ", newMat->mName, " doesn't contain all textures" );
    if ( !newMat->tex0 )
        newMat->tex0 = renderer.GetDefaultTexture( );

    mMaterials.insert( std::make_pair( material.mName, newMat ) );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
std::shared_ptr<Material> Scene::FindMaterial( const std::string &matName )
{
    D3DRenderer &renderer = D3DRenderer::Get();

    std::shared_ptr<Material> mat;
    auto mati = mMaterials.find( matName );
    WARNING( mati == mMaterials.end( ), "Can't find material", matName );
    if ( mati != mMaterials.end( ) )
        mat = mati->second;
    else
        mat = renderer.GetDefaultMaterial( );

    return mat;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Scene::LoadMaterialTextureFromFile( const std::string &line, const std::string &fPath, const char *signature, 
    std::shared_ptr<D3DTextureBuffer2D> &textureSlot )
{
    std::string texturePath = fPath + line.substr( strlen( signature ) );
    auto tex = mTextures.find( texturePath );
    if ( tex != mTextures.end( ) )
    {
        textureSlot = tex->second;
    }
    else
    {
        auto &d3dTex = D3DTextureBuffer2D::CreateFromFile( texturePath );
        WARNING( !d3dTex->GetSRV( ), "Can't create texture from '", texturePath, "' use default instead" );

        if ( d3dTex->GetSRV() )
        {
            textureSlot = d3dTex;
        }
        else
        {
            D3DRenderer &renderer = D3DRenderer::Get( );
            textureSlot = renderer.GetDefaultTexture();
        }

        mTextures.insert( std::make_pair( texturePath, textureSlot ) );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Scene::UpdateSun( float dt )
{
    Settings &settings = Settings::Get( );
    mSun.color.x = settings.mSunColor[0] * settings.mSunPower;
    mSun.color.y = settings.mSunColor[1] * settings.mSunPower;
    mSun.color.z = settings.mSunColor[2] * settings.mSunPower;

    D3DRenderer &renderer = D3DRenderer::Get();
    auto sceneBB = renderer.GetStaticSceneBB();

    // calc center, radius
    // need center of floor, calc radius
    DirectX::XMVECTOR floorCenter = DirectX::XMVectorScale( 
                        DirectX::XMVectorAdd( DirectX::XMLoadFloat3( &sceneBB.first ),
                        DirectX::XMVectorSet( sceneBB.second.x, sceneBB.first.y, sceneBB.second.z, 0.0f ) ), 0.5f );

    float &lRadius = mSun.radius;
    DirectX::XMStoreFloat( &lRadius, DirectX::XMVector3Length(
        DirectX::XMVectorSubtract( DirectX::XMLoadFloat3( &sceneBB.second ), floorCenter ) ) );

    if ( settings.mLightAnimation )
    {
        float angle = settings.mLightAnimationSpeed * dt;
        mSunOffset += angle;
        if ( mSunOffset > PI * 2.0f )
            mSunOffset -= PI * 2.0f;
    }

    float lYaw = settings.mSunYaw + mSunOffset;
    float lPitch = settings.mSunPitch;
    float y = sinf( lPitch );
    float x = cosf( lYaw ) * fabs( cosf( lPitch ) );
    float z = sinf( lYaw ) * fabs( cosf( lPitch ) );

    lRadius *= settings.mLightDistance;

    // calc position and direction from staticSceneBB, yaw, pitch, distance
    mSun.position = DirectX::XMFLOAT3( x * lRadius, y * lRadius, z * lRadius );
    mSun.direction = DirectX::XMFLOAT3( -x, -y, -z );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Scene::UpdateCamDirection()
{
    float dx = 0.0f, dy = 0.0f, dz = 0.0f;
    for ( auto it : mCamMoveDirSet )
    {
        switch ( it )
        {
        case CD_UP:
            dy += 1.0f;
            break;
        case CD_DOWN:
            dy -= 1.0f;
            break;
        case CD_FORWARD:
            dx += 1.0f;
            break;
        case CD_BACK:
            dx -= 1.0f;
            break;
        case CD_LEFT:
            dz -= 1.0f;
            break;
        case CD_RIGHT:
            dz += 1.0f;
            break;
        default:
            break;
        }
    }

    DirectX::XMFLOAT3 dir( dx, dy, dz );
    DirectX::XMVECTOR dirV = DirectX::XMLoadFloat3( &dir );

    float dirLengh = 0.0f;
    DirectX::XMStoreFloat( &dirLengh, DirectX::XMVector3Length( dirV ) );
    if ( dirLengh < 0.001f )
    {
        mCamMoveDir.x = mCamMoveDir.y = mCamMoveDir.z = 0.0f;
    }
    else
    {
        DirectX::XMStoreFloat3( &mCamMoveDir, DirectX::XMVector3Normalize( dirV ) );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// TODO - remove, it works with perfect cases with v/vt/vn
void LoadSingleObj( const char *fn, GGMeshData &data )
{
    data.verticies.clear( );
    data.indicies.clear( );

    std::ifstream objFile;
    objFile.open( fn, std::ifstream::in );
    if ( objFile.fail( ) )
    {
        ASSERT( false );
        LOG_ERROR( "Error during opening file: ", fn );
        return;
    }

    std::vector<DirectX::XMFLOAT3> positions;
    std::vector<DirectX::XMFLOAT3> normals;
    std::vector<DirectX::XMFLOAT3> uvw;
    std::vector<std::array<int, 9> > faceInfo;

    std::string line;
    while ( !objFile.eof( ) ) {
        std::getline( objFile, line );
        std::istringstream iss( line.c_str( ) );

        char tmp;
        if ( !line.compare( 0, 2, "v " ) ) {
            iss >> tmp;
            float v[3];
            for ( int i = 0; i < 3; i++ )
                iss >> v[i];

            positions.push_back( DirectX::XMFLOAT3( v[0], v[1], v[2] ) );
        }
        else if ( !line.compare( 0, 3, "vt " ) ) {
            iss >> tmp >> tmp;
            float vt[3];
            for ( int i = 0; i < 3; i++ )
                iss >> vt[i];

            uvw.push_back( DirectX::XMFLOAT3( 1.0f - vt[0], vt[1], vt[2] ) );
            //uvw.push_back( DirectX::XMFLOAT3( vt[0], vt[1], vt[2] ) );
        }
        else if ( !line.compare( 0, 3, "vn " ) ) {
            iss >> tmp >> tmp;
            float vn[3];
            for ( int i = 0; i < 3; i++ )
                iss >> vn[i];

            normals.push_back( DirectX::XMFLOAT3( vn[0], vn[1], vn[2] ) );
        }
        else if ( !line.compare( 0, 2, "f " ) ) {
            std::array<int, 9> face; // position, normal, texture
            int idv, idn, idt, i = 0;
            iss >> tmp;
            while ( iss >> idv >> tmp >> idt >> tmp >> idn ) {
                idv--; // in wavefront obj all indices start at 1, not zero
                face[0 + i * 3] = idv;
                idt--;
                face[1 + i * 3] = idt;
                idn--;
                face[2 + i * 3] = idn;
                i++;
            }
            faceInfo.push_back( face );
        }
    }

    size_t lengthLimit = 1 << 21;
    ASSERT( positions.size( ) < lengthLimit && normals.size( ) < lengthLimit && uvw.size( ) < lengthLimit );

    // build layout, prevent duplicates
    std::vector<std::vector<std::pair<int, int> > > vertexCoherence;
    std::unordered_map<__int64, int> indiciesHash;
    int maxIndex = 0;
    for each ( auto f in faceInfo )
    {
        int iv[3];
        for ( int i = 0; i < 3; i++ )
        {
            int vp = f[0 + i * 3], vt = f[1 + i * 3], vn = f[2 + i * 3];
            __int64 key = ( __int64 )vp + ( ( ( __int64 )vt ) << 21 ) + ( ( ( __int64 )vn ) << 42 );
            auto keyValue = indiciesHash.find( key );
            if ( keyValue != indiciesHash.end( ) )
            {
                data.indicies.push_back( keyValue->second );
            }
            else
            {
                // push new vertex
                GGVertex v;
                v.position = positions[vp];
                v.UVW = DirectX::XMFLOAT3( 1.0f - uvw[vt].x, 1.0f - uvw[vt].y, uvw[vt].z );
                v.normal = normals[vn];
                data.verticies.push_back( v );
                indiciesHash.insert( std::make_pair( key, maxIndex ) );
                data.indicies.push_back( maxIndex++ );
                vertexCoherence.push_back( std::vector<std::pair<int, int> >( ) );
            }
            iv[i] = data.indicies.back( );
        }
        vertexCoherence[iv[0]].push_back( std::make_pair( iv[1], iv[2] ) );
        vertexCoherence[iv[1]].push_back( std::make_pair( iv[0], iv[2] ) );
        vertexCoherence[iv[2]].push_back( std::make_pair( iv[0], iv[1] ) );
    }
    GeometryGenerator::CalculateTBN( data, vertexCoherence );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
