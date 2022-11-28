#include "strafe.hpp"
#include <cstdio>

// Outputs some data for gnuplot, maybe this will also draw things in the future

using namespace Strafe;

struct Point
{
    float x;
    float y;
};

Point GetSpeedGain(PlayerData data, const MovementVars& vars, double theta)
{
    float startVel = data.m_vecVelocity.Length2D();
    Strafe::VectorFME(data, vars, theta);
    float diff = data.m_vecVelocity.Length2D() - startVel;

    Point out;
    out.x = M_RAD2DEG * theta;
    out.y = diff;
    return out;
}

Point GraphPoint(const PlayerData& data, const MovementVars& vars, StrafeType type)
{
    Strafe::StrafeInput input;
    input.CappedLimit = data.m_vecVelocity.Length2D();
    input.Stype = type;
    double theta = Strafe::StrafeTheta(data, vars, input);

    return GetSpeedGain(data, vars, theta);
}

int main() {
    const int points = 1800;
    const float startVel = 230.0f;

    MovementVars vars;
    vars.Airaccelerate = 15.0f;
    vars.WishspeedCap = 60.0f;

    PlayerData data;
    data.m_vecVelocity.x = startVel;

    auto maxaccel = GraphPoint(data, vars, Strafe::StrafeType::MaxAccel);
    auto capped = GraphPoint(data, vars, Strafe::StrafeType::MaxAccelCapped);
    auto maxangle = GraphPoint(data, vars, Strafe::StrafeType::MaxAngle);

    std::fprintf(stderr, "set label at %f, %f \"maxaccel\" point pointtype 7 pointsize 1\n", maxaccel.x, maxaccel.y);
    std::fprintf(stderr, "set label at %f, %f \"maxangle\" point pointtype 7 pointsize 1\n", maxangle.x, maxangle.y);
    std::fprintf(stderr, "set label at %f, %f \"capped\" point pointtype 7 pointsize 1\n", capped.x, capped.y);

    for(size_t i=0; i < points; ++i)
    {
        double theta = (double)i / points * M_PI;
        Point out = GetSpeedGain(data, vars, theta);

        std::printf("%f %f\n", out.x, out.y);
    }
    

    return 0;
}
