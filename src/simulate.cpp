#include "strafe.hpp"
#include <algorithm>

using namespace Strafe;

static void Friction(PlayerData& player, const MovementVars& vars)
{
    if (!player.OnGround)
        return;

    float speed = player.Velocity.Length2D();
    if (speed < 0.1f)
        return;

    float friction = float{vars.Friction * vars.EntFriction};
    float control = (speed < vars.Stopspeed) ? vars.Stopspeed : speed;
    float drop = control * friction * vars.Frametime;
    float newspeed = std::max(speed - drop, 0.f);
    if(newspeed != speed)
    {
        player.Velocity.Scale(newspeed);
    }
}

void Simulate(PlayerData& player, const MovementVars& vars, const StrafeOutput& output)
{
    Friction(player, vars);
}

