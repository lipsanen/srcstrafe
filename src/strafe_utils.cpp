#include "strafe_utils.hpp"

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

void Vector::Scale(float f)
{
    float length = Length();

    x *= f / length;
    y *= f / length;
    z *= f / length;
}
