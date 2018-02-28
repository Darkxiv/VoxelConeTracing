
// FX specific header file

Texture3D<float4> opacityBrickBufferR;
Texture3D<float4> irradianceBrickBufferR; // also we can cast buffer for each (NX PX NY PY NZ PZ) light direction

uint brickBufferSize;

#define BRICK_SIZE 3
#define SAMPLING_OFFSET ( 0.5f / brickBufferSize )
#define SAMPLING_AREA_SIZE ( 2.0f / brickBufferSize )

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint3 NodeIDToTextureCoords( uint id )
{
    uint blocksPerAxis = brickBufferSize / BRICK_SIZE; // 3x3x3 values per brick
    uint blocksPerAxisSq = blocksPerAxis * blocksPerAxis;
    return uint3( id % blocksPerAxis, (id / blocksPerAxis) % blocksPerAxis, id / blocksPerAxisSq ) * BRICK_SIZE;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint3 IndexToBrickCoords( in uint index )
{
    return uint3( index % BRICK_SIZE, (index / BRICK_SIZE) % BRICK_SIZE, index / (BRICK_SIZE * BRICK_SIZE) );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
uint CoordsToBrickIndex( in uint3 coords )
{
    return coords.x + coords.y * BRICK_SIZE + coords.z * BRICK_SIZE * BRICK_SIZE;
}
