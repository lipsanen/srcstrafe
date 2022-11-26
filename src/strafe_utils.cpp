#include "strafe_utils.hpp"
#include <float.h>

using namespace Strafe;

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

void Strafe::SinCos(float x, float* s, float *c)
{
    *s = sin(x);
    *c = cos(x);
}

void Strafe::AngleVectors(const Vector& angles, Vector* forward, Vector* right, Vector* up)
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
