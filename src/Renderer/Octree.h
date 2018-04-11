#ifndef __OCTREE_H
#define __OCTREE_H

#include <memory>

class D3DTextureBuffer2D;
class D3DStructuredBuffer;
struct ID3D11Texture2D;

struct Octree
{
    Octree() = default;

    void Init();
    void Clear();

    void ClearOctree( );

    size_t mHeight = 0;
    size_t mResolution = 0;
    size_t mBufferSize = 0;
    std::shared_ptr<D3DTextureBuffer2D> mOctreeTex;
    std::shared_ptr<D3DStructuredBuffer> mNodesPackCounter;
    size_t mNodesCount = 0;
};

#endif