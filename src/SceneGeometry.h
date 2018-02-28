#ifndef __SCENE_GEOMETRY_H
#define __SCENE_GEOMETRY_H

#include <string>
#include <memory>

struct Material;
class D3DGeometryBuffer;

struct SceneGeometry
{
    SceneGeometry( const std::string &objName, const std::shared_ptr<D3DGeometryBuffer> &geometryBuffer, std::shared_ptr<Material> mat );

    std::shared_ptr<D3DGeometryBuffer> mGeometryBuffer;
    std::shared_ptr<Material> mMaterial;

    bool isDynamic;
    std::string mName;
};


#endif