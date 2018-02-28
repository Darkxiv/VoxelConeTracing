#include <Camera.h>
#include <GlobalUtils.h>

using namespace DirectX; // a lot of directx math

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
XMFLOAT3 Normalize( const XMFLOAT3 &v )
{
    float length = sqrt( v.x*v.x + v.y*v.y + v.z*v.z );
    ASSERT( length < EPS );
    return XMFLOAT3( v.x / length, v.y / length, v.z / length );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
Camera::Camera():
    mPhi( -0.115f ),
    mTheta( -3.136f ),
    mPos( 0.01511f, 0.3311f, 3.3732f, 1.0f ),
    mUp( 0.0f, 1.0f, 0.0f, 1.0f )
{
    XMMATRIX I = XMMatrixIdentity( );
    XMStoreFloat4x4( &mViewTransform, I );

    XMVECTOR tmp = XMVectorSet( sinf( mTheta ) * cosf( mPhi ), sinf( mPhi ), cosf( mTheta ) * cosf( mPhi ), 0.0f );
    XMVECTOR viewVec = XMVector3Normalize( tmp );
    XMStoreFloat4( &mViewVec, viewVec );
    XMStoreFloat4( &mRight, XMVector3Normalize( XMVector3Cross( viewVec, XMLoadFloat4( &mUp ) ) ) );

    UpdateViewTransform();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Camera::ChangePosition( float dx, float dy, float dz )
{
    XMStoreFloat4( &mPos, XMVectorAdd( XMLoadFloat4( &mPos ), XMVectorScale( XMLoadFloat4( &mViewVec ), dx ) ) );
    XMStoreFloat4( &mPos, XMVectorAdd( XMLoadFloat4( &mPos ), XMVectorScale( XMLoadFloat4( &mRight ), dz ) ) );
    XMStoreFloat4( &mPos, XMVectorAdd( XMLoadFloat4( &mPos ), XMVectorSet( 0.0f, dy, 0.0f, 0.0f ) ) );

    UpdateViewTransform( );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Camera::ChangeDegrees( float dTheta, float dPhi )
{
    mPhi = Clamp( mPhi + dPhi, -PI_2, PI_2 );
    mTheta += dTheta;
    if ( mTheta + dTheta > PI )
        mTheta -= PI * 2.0f;
    else if ( mTheta < -PI )
        mTheta += PI * 2.0f;

    SetDegrees( mTheta, mPhi );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Camera::SetDegrees( float theta, float phi )
{
    mPhi = Clamp( phi, -PI_2, PI_2 );
    mTheta = Clamp( theta, -PI, PI );

    XMVECTOR tmp = XMVectorSet( sinf( mTheta ) * cosf( mPhi ), sinf( mPhi ), cosf( mTheta ) * cosf( mPhi ), 0.0f );
    XMStoreFloat4( &mViewVec, XMVector3Normalize( tmp ) );

    UpdateRightUp( );
    UpdateViewTransform( );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Camera::SetPosition( float x, float y, float z )
{
    mPos = XMFLOAT4( x, y, z, 1.0f );
    UpdateViewTransform();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Camera::SetPosition( const XMFLOAT3 &vec )
{
    mPos = XMFLOAT4( vec.x, vec.y, vec.z, 1.0f );
    UpdateViewTransform();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Camera::SetPosition( const XMFLOAT4 &vec )
{
    mPos = vec;
    UpdateViewTransform();
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Camera::SetDirection( float x, float y, float z )
{
    SetDirection( XMFLOAT4( x, y, z, 1.0f ) );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Camera::SetDirection( const XMFLOAT3 &vec )
{
    SetDirection( XMFLOAT4( vec.x, vec.y, vec.z, 1.0f ) );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Camera::SetDirection( const XMFLOAT4 &vec )
{
    XMStoreFloat4( &mViewVec, XMVector3Normalize( XMLoadFloat4( &vec ) ) );

    mTheta = asin( mViewVec.y );
    if ( abs( mViewVec.y ) > 0.99f )
        mPhi = 0.0f;
    else
        mPhi = acos( 1.0f - mViewVec.y * mViewVec.y );

    UpdateRightUp();
    UpdateViewTransform( );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
const DirectX::XMFLOAT4X4& Camera::GetWorldToViewTransform( )
{
    return mViewTransform;
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Camera::UpdateRightUp()
{
    if ( abs( mViewVec.y ) > 0.99f )
    {
        mRight = XMFLOAT4( sinf( mTheta - XM_PIDIV2 ), 0.0f, cosf( mTheta - XM_PIDIV2 ), 0.0f );
        XMStoreFloat4( &mUp, XMVector3Normalize( XMVector3Cross( XMLoadFloat4( &mRight ), XMLoadFloat4( &mViewVec ) ) ) );
    }
    else
    {
        mUp = XMFLOAT4( 0.0f, 1.0f, 0.0f, 0.0f );
        XMStoreFloat4( &mRight, XMVector3Normalize( XMVector3Cross( XMLoadFloat4( &mViewVec ), XMLoadFloat4( &mUp ) ) ) );
    }
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
void Camera::UpdateViewTransform( )
{
    XMVECTOR target = XMVectorAdd( XMLoadFloat4( &mPos ), XMLoadFloat4( &mViewVec ) );
    XMVECTOR pos = XMLoadFloat4( &mPos );
    XMVECTOR up = XMLoadFloat4( &mUp );

    XMMATRIX viewTransform = XMMatrixLookAtRH( pos, target, up );

    XMStoreFloat4x4( &mViewTransform, viewTransform );
}
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
