#ifndef __LIGHT_H
#define __LIGHT_H

#include <DirectXMath.h>

struct LightSource
{
    LightSource():
        type( DIRECTIONAL ),
        position( 0.0, 0.0, 0.0 ),
        direction( 0.0, -1.0, 0.0 ),
        color( 0.0, 0.0, 0.0 ),
        radius( 0.0 )
    {

    }

    bool operator==( const LightSource &r ) const
    {
        auto compareXMFLOAT3 = []( const DirectX::XMFLOAT3 &l, const DirectX::XMFLOAT3 &r )
        {
            return l.x == r.x && l.y == r.y && l.z == r.z;
        };

        return type == r.type &&
            compareXMFLOAT3( color, r.color ) &&
            compareXMFLOAT3( position, r.position ) &&
            compareXMFLOAT3( direction, r.direction ) &&
            radius == r.radius;
    }

    bool operator!=( const LightSource &r ) const
    {
        return !( *this == r );
    }

    enum LightType
    {
        DIRECTIONAL
        // POINT
    } type;

    DirectX::XMFLOAT3 position;
    DirectX::XMFLOAT3 direction;
    DirectX::XMFLOAT3 color;

    float radius;
    // bool castShadow
};


#endif