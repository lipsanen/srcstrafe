#include "strafe.hpp"
#include <cmath>
#include <cstdio>

using namespace Strafe;

static double Accelerate(const PlayerData &player, const MovementVars &vars)
{
    return player.OnGround ? vars.Accelerate : vars.Airaccelerate;
}

static double Wishspeed(const PlayerData &player, const MovementVars &vars)
{
    if (player.OnGround)
    {
        return vars.Maxspeed;
    }
    else
    {
        return vars.WishspeedCap;
    }
}

double Strafe::TargetTheta(const PlayerData &player,
                           const MovementVars &vars,
                           double targetVel)
{
#if 0
double accel = player.OnGround ? vars.Accelerate : vars.Airaccelerate;
double L = vars.WishspeedCap;
double gamma1 = vars.EntFriction * vars.Frametime * vars.Maxspeed * accel;

PlayerData copy = player;
double lambdaVel = copy.m_vecVelocity.Length2D();

double cosTheta;

if (gamma1 <= 2 * L)
{
    cosTheta = ((targetVel * targetVel - lambdaVel * lambdaVel) / gamma1 - gamma1) / (2 * lambdaVel);
    return std::acos(cosTheta);
}
else
{
    cosTheta = std::sqrt((targetVel * targetVel - L * L) / lambdaVel * lambdaVel);
    return std::acos(cosTheta);
}
#else
    double A = Accelerate(player, vars) * vars.Maxspeed * vars.Frametime * player.m_surfaceFriction;
    double A2 = std::pow(A, 2);
    double v0 = player.m_vecVelocity.Length2D();
    double v02 = std::pow(v0, 2);
    double solution1 = (-A2 - v02 + std::pow(targetVel, 2)) / (2 * A * v0);
    double solution2 = (A2 + v02 - std::pow(targetVel, 2)) / (2 * A * v0);

    if(solution1 >= -1 && solution1 <= 1)
    {
        return std::acos(solution1);
    }
    else if(solution2 >= -1 && solution2 <= 1)
    {
        return std::acos(solution2);
    }
    else
    {
        abort(); // lol
    }
#endif
}

double MaxAccelTheta(const PlayerData &player, const MovementVars &vars)
{
    double accel = player.OnGround ? vars.Accelerate : vars.Airaccelerate;
    double wishspeed = Wishspeed(player, vars);

    double accelspeed = accel * vars.Maxspeed * vars.Frametime * player.m_surfaceFriction;
    if (accelspeed <= 0.0)
        return M_PI;

    if (player.m_vecVelocity.Length2D() == 0)
        return 0.0;

    double wishspeed_capped = player.OnGround ? wishspeed : vars.WishspeedCap;
    double tmp = wishspeed_capped - accelspeed;
    if (tmp <= 0.0)
        return M_PI / 2;

    double speed = player.m_vecVelocity.Length2D();
    if (tmp < speed)
        return std::acos(tmp / speed);

    return 0.0;
}

void VectorFME(PlayerData &player, const MovementVars &vars, double theta)
{
    Vector a(std::cos(theta), std::sin(theta), 0);
    double wishspeed_capped = Wishspeed(player, vars);
    double tmp = wishspeed_capped - player.m_vecVelocity.Dot2D(a);
    if (tmp <= 0.0)
        return;

    double accel = Accelerate(player, vars);
    double accelspeed = accel * vars.Maxspeed * vars.Frametime * player.m_surfaceFriction;
    if (accelspeed <= tmp)
        tmp = accelspeed;

    player.m_vecVelocity.x += static_cast<float>(a.x * accelspeed);
    player.m_vecVelocity.y += static_cast<float>(a.y * accelspeed);
}

bool Strafe::OvershotCap(const PlayerData &player, const MovementVars &vars, const StrafeInput &input)
{
    PlayerData temp = player;
    temp.m_vecVelocity.x = player.m_vecVelocity.Length2D();
    temp.m_vecVelocity.y = 0;
    VectorFME(temp, vars, M_PI);
    float vel2d = temp.m_vecVelocity.Length2D();

    return vel2d > input.CappedLimit;
}

static double StrafeCappedTheta(const PlayerData &player, const MovementVars &vars, const StrafeInput &input)
{
    double theta = MaxAccelTheta(player, vars);

    PlayerData temp = player;
    temp.m_vecVelocity.x = player.m_vecVelocity.Length2D();
    temp.m_vecVelocity.y = 0;

    VectorFME(temp, vars, theta);
    float vel2d = temp.m_vecVelocity.Length2D();

    if (vel2d > input.CappedLimit)
    {
        // If we are completely over the cap and can't set our velocity to it
        // just slow down as fast as possible
        if (OvershotCap(player, vars, input))
        {
            theta = M_PI;
        }
        else
        {
            theta = TargetTheta(player, vars, input.CappedLimit);
        }
    }

    return theta;
}

static double MaxAngleTheta(const PlayerData &player,
                            const MovementVars &vars)
{
    double speed = player.m_vecVelocity.Length2D();
    double accel = Accelerate(player, vars);
    double accelspeed = accel * vars.Maxspeed * vars.Frametime * player.m_surfaceFriction;

    if (accelspeed <= 0.0)
    {
        double wishspeed_capped = Wishspeed(player, vars);
        accelspeed *= -1;
        if (accelspeed >= speed)
        {
            if (wishspeed_capped >= speed)
                return 0.0;
            else
            {
                //safeguard_yaw = true;
                return std::acos(wishspeed_capped / speed); // The actual angle needs to be _less_ than this.
            }
        }
        else
        {
            if (wishspeed_capped >= speed)
                return std::acos(accelspeed / speed);
            else
            {
                //safeguard_yaw = (wishspeed_capped <= accelspeed);
                return std::acos(
                    std::min(accelspeed, wishspeed_capped) / speed); // The actual angle needs to be _less_ than this if wishspeed_capped <= accelspeed.
            }
        }
    }
    else
    {
        if (accelspeed >= speed)
            return M_PI;
        else
            return std::acos(-1 * accelspeed / speed);
    }
}

double Strafe::StrafeTheta(const PlayerData &player, const MovementVars &vars, const StrafeInput &input)
{
    if (input.Strafe)
    {
        if (input.Stype == StrafeType::MaxAccelCapped)
        {
            return StrafeCappedTheta(player, vars, input);
        }
        else if (input.Stype == StrafeType::MaxAccel)
        {
            return MaxAccelTheta(player, vars);
        }
        else if (input.Stype == StrafeType::Direction)
        {
            return 0.0;
        }
        else if (input.Stype == StrafeType::MaxAngle)
        {
            return MaxAngleTheta(player, vars);
        }
    }

    return 0;
}

TraceResult Strafe::TraceDefault(const Vector &start, const Vector &end, const PlayerData& data)
{
    TraceResult result;
    result.AllSolid = false;
    result.EndPos = end;
    result.Fraction = 1.0f;
    result.StartSolid = false;

    return result;
}

bool Strafe::TraceGroundDefault(PlayerData &data)
{
    return data.OnGround;
}
