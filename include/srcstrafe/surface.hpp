#pragma once

#include "srcstrafe/entity.hpp"
#include "srcstrafe/vector.hpp"

namespace Strafe
{
    struct Surface
    {
        Surface();
        static Surface GetZSurface(float z);
        float Dot(const Vector& pos);
        trace_t Trace(const Ray_t& ray, const CBasePlayer &player, unsigned int fMask);
        Vector pos;
        Vector normal;
    };
}
