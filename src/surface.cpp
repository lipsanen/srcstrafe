#include "srcstrafe/surface.hpp"
#include <algorithm>

namespace Strafe
{
    Surface::Surface() = default;

    Surface Surface::GetZSurface(float z)
    {
        Surface surf;
        surf.pos.z = z;
        surf.normal.z = 1.0f;

        return surf;
    }

    float Surface::Dot(const Vector& rhs)
    {
        Vector diffy = rhs - this->pos;
        return diffy.Dot(this->normal);
    }

    trace_t Surface::Trace(const Ray_t& ray, const CBasePlayer &player, unsigned int fMask)
    {
        trace_t result;
        
        Vector extents;
        for(int i=0; i < 3; ++i)
        {
            if(normal[i] <= 0.0f)
            {
                extents[i] = ray.maxs[i];
            }
            else
            {
                extents[i] = ray.mins[i];
            }
        }

        float startDot = Dot(ray.start + extents);
        float endDot = Dot(ray.end + extents);
        const float EPS = 1e-5;
        bool hitSurface = false;

        if(startDot <= EPS && startDot >= 0 && endDot <= EPS && endDot >= 0)
        {
            result.fraction = 1.0f;
            hitSurface = true;
        }
        else if(startDot >= 0 && endDot < 0)
        {
            float diffy = std::max(startDot - endDot, EPS);
            result.fraction = startDot / diffy;
            hitSurface = true;
        }
        else
        {
            result.fraction = 1.0f;
        }

        result.endpos = (ray.end - ray.start) * result.fraction + ray.start;

        if(hitSurface)
        {
            result.m_pEnt.m_bValid = true;
            result.plane.normal = this->normal;
        }

        return result;
    }
}
