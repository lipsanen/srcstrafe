#include "gtest/gtest.h"
#include "srcstrafe/strafe_utils.h"
#include <cstddef>

using namespace Strafe;

TEST(DEG2RAD2DEG, Works)
{
    for(size_t i=0; i < 1000000; ++i)
    {
        double yaw = (rand() / (double)RAND_MAX) * 360;
        double rad = yaw * Strafe::M_DEG2RAD;
        double yaw2 = rad * Strafe::M_RAD2DEG;
        EXPECT_LE(std::fabs(yaw-yaw2), 1e-10);
    }
}

TEST(Vector, Scale)
{
    Vector v;
    v.x = 1.0f;
    v.Scale(100.0f);
    EXPECT_EQ(v[0], 100.0f);
}
