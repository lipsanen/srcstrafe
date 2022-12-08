#include "srcstrafe/surface.hpp"
#include "srcstrafe/strafe_utils.h"
#include <algorithm>
#include <float.h>

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

        Vector start = ray.start + extents;
        Vector end = ray.end + extents;
        float startDot = Dot(start);
        float endDot = Dot(end);
        const float EPS = 1e-5;
        bool hitSurface = false;

        if(startDot <= EPS && startDot >= 0 && endDot <= EPS && endDot >= 0)
        {
            Vector delta = end - start;
            float dot = delta.Dot(this->normal);
            result.fraction = 1.0f;
            hitSurface = dot < -EPS;
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

    Vector vec3_origin;

    float& Vector::operator[](int i)
    {
        if(i == 0)
        {
            return x;
        }
        else if(i == 1)
        {
            return y;
        }
        else
        {
            return z;
        }
    }

    bool Vector::operator==(const Vector& rhs) const
    {
        return x == rhs.x && y == rhs.y && z == rhs.z;
    }

    const float& Vector::operator[](int i) const
    {
        if(i == 0)
        {
            return x;
        }
        else if(i == 1)
        {
            return y;
        }
        else
        {
            return z;
        }
    }

    Vector::Vector(float x, float y, float z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    float Vector::Length2D() const
    {
        return std::sqrt(x * x + y * y);
    }

    float Vector::Length() const
    {
        return std::sqrt(x * x + y * y + z * z);
    }

    float Vector::Dot2D(const Vector& rhs) const
    {
        return x * rhs.x + y * rhs.y;
    }


    float Vector::Dot(const Vector& rhs) const
    {
        return x * rhs.x + y * rhs.y + z * rhs.z;
    }

    float Vector::VectorNormalize()
    {
        float length = Length();
        if(length < FLT_EPSILON)
            length = FLT_EPSILON;

        x /= length;
        y /= length;
        z /= length;

        return length;
    }

    void Vector::Scale(float f)
    {
        float length = Length();
        if(length < FLT_EPSILON)
            length = FLT_EPSILON;

        x *= f / length;
        y *= f / length;
        z *= f / length;
    }


    void Vector::Add(const Vector& rhs)
    {
        x += rhs.x;
        y += rhs.y;
        z += rhs.z;
    }

    void Vector::Subtract(const Vector& rhs)
    {
        x -= rhs.x;
        y -= rhs.y;
        z -= rhs.z;
    }

    void Vector::Zero()
    {
        x = y = z = 0.0f;
    }

    void SinCos(float x, float* s, float *c)
    {
        *s = sin(x);
        *c = cos(x);
    }

    void AngleVectors(const Vector& angles, Vector* forward, Vector* right, Vector* up)
    {
        float sr, sp, sy, cr, cp, cy;

        SinCos(angles[YAW] * M_DEG2RAD, &sy, &cy );
        SinCos(angles[PITCH] * M_DEG2RAD, &sp, &cp );
        SinCos(angles[ROLL] * M_DEG2RAD, &sr, &cr );

        if (forward)
        {
            forward->x = cp*cy;
            forward->y = cp*sy;
            forward->z = -sp;
        }

        if (right)
        {
            right->x = (-1*sr*sp*cy+-1*cr*-sy);
            right->y = (-1*sr*sp*sy+-1*cr*cy);
            right->z = -1*sr*cp;
        }

        if (up)
        {
            up->x = (cr*sp*cy+-sr*-sy);
            up->y = (cr*sp*sy+-sr*cy);
            up->z = cr*cp;
        }
    }

    Vector VectorMult(const Vector& src, float scale)
    {
        Vector v = src;
        v.x *= scale;
        v.y *= scale;
        v.z *= scale;
        return v;
    }


    void VectorMA( const Vector& start, float scale, const Vector& direction, Vector& dest )
    {
        dest.x = start.x + scale * direction.x;
        dest.y = start.y + scale * direction.y;
        dest.z = start.z + scale * direction.z;
    }

    int ClipVelocity(Vector& in, Vector& normal, Vector& out, float overbounce)
    {
        float	backoff;
        float	change;
        float angle;
        int		i, blocked;
        
        angle = normal[ 2 ];

        blocked = 0x00;         // Assume unblocked.
        if (angle > 0)			// If the plane that is blocking us has a positive z component, then assume it's a floor.
            blocked |= 0x01;	// 
        if (!angle)				// If the plane has no Z, it is vertical (wall/step)
            blocked |= 0x02;	// 
        

        // Determine how far along plane to slide based on incoming direction.
        backoff = DotProduct (in, normal) * overbounce;

        for (i=0 ; i<3 ; i++)
        {
            change = normal[i]*backoff;
            out[i] = in[i] - change; 
        }
        
        // iterate once to make sure we aren't still moving through the plane
        float adjust = DotProduct( out, normal );
        if( adjust < 0.0f )
        {
            Vector sub = VectorMult(normal, adjust);
            out.Subtract(sub);
        }

        // Return blocking flags.
        return blocked;
    }


    float VectorNormalize(Vector& v)
    {
        return v.VectorNormalize();
    }

    float VectorLength(const Vector& v)
    {
        return v.Length();
    }

    void VectorCopy(const Vector& src, Vector& dest)
    {
        dest = src;   
    }

    void Vector::Init(float x, float y, float z)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    float Vector::LengthSqr() const
    {
        return this->Dot(*this);
    }

    Vector Vector::operator+(const Vector& rhs) const
    {
        return Vector(x + rhs.x, y + rhs.y, z + rhs.z);
    }

    Vector& Vector::operator+=(const Vector& rhs)
    {
        return *this = *this + rhs;
    }

    Vector Vector::operator-(const Vector& rhs) const
    {
        return Vector(x - rhs.x, y - rhs.y, z - rhs.z);
    }

    Vector& Vector::operator-=(const Vector& rhs)
    {
        return *this = *this - rhs;
    }

    Vector Vector::operator*(float rhs)
    {
        return VectorMult(*this, rhs);
    }

    Vector& Vector::operator*=(float rhs)
    {
        return *this = *this * rhs;
    }

    Vector Vector::operator-() const
    {
        return Vector(-x, -y, -z);
    }

    void Ray_t::Init(const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs)
    {
        this->start = start;
        this->end = end;
        this->mins = mins;
        this->maxs = maxs;
    }

    void VectorScale(Vector& src, float scale, Vector& dest)
    {
        Vector v = src;
        v.VectorNormalize();
        dest = VectorMult(v, scale);
    }

}
