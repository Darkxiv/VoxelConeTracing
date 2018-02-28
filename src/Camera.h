#ifndef __CAMERA_H
#define __CAMERA_H

#include <DirectXMath.h>

class Camera
{
public:
    Camera();

    void ChangePosition( float dx, float dy, float dz );
    void ChangeDegrees( float dTheta, float dPhi );

    void SetDegrees( float theta, float phi );
    void SetPosition( float x, float y, float z );
    void SetPosition( const DirectX::XMFLOAT3 &vec );
    void SetPosition( const DirectX::XMFLOAT4 &vec );
    void SetDirection( float x, float y, float z );
    void SetDirection( const DirectX::XMFLOAT3 &vec );
    void SetDirection( const DirectX::XMFLOAT4 &vec );

    const DirectX::XMFLOAT4X4& GetWorldToViewTransform( );

private:
    float mPhi, mTheta;
    DirectX::XMFLOAT4 mPos;
    DirectX::XMFLOAT4 mViewVec;
    DirectX::XMFLOAT4 mRight;
    DirectX::XMFLOAT4 mUp;

    DirectX::XMFLOAT4X4 mViewTransform;

    void UpdateRightUp();
    void UpdateViewTransform();

    // is parallel projection (projectionType)
    // fov
};

#endif