#include "gtest/gtest.h"
#include "srcstrafe/gamemovement.h"
#include "srcstrafe/in_buttons.h"

TEST(GameMovement, BasicAirCase)
{
    const float maxspeed = 320.0f;
    Strafe::CGameMovement movement;
    Strafe::CBasePlayer player;
    Strafe::CMoveData data;

    player.m_flClientMaxSpeed = maxspeed;
    data.m_flForwardMove = maxspeed;

    EXPECT_EQ(player.m_vecVelocity[0], 0.0f);
    EXPECT_EQ(player.m_vecAbsOrigin[0], 0.0f);
    movement.ProcessMovement(&player, &data);
    EXPECT_EQ(player.m_vecVelocity[0], 30.0f);
    EXPECT_EQ(player.m_vecVelocity[2], -movement.m_gameVars.sv_gravity * movement.m_gameVars.frametime);
    EXPECT_EQ(player.m_vecAbsOrigin[0], 30.0f * 0.015f);
}

TEST(GameMovement, BasicGroundCase)
{
    const float maxspeed = 320.0f;
    Strafe::CGameMovement movement;
    Strafe::CBasePlayer player;
    player.m_GroundEntity.m_bValid = true;
    Strafe::CMoveData data;

    player.m_flClientMaxSpeed = maxspeed;
    data.m_flForwardMove = maxspeed;

    EXPECT_EQ(player.m_vecVelocity[0], 0.0f);
    EXPECT_EQ(player.m_vecAbsOrigin[0], 0.0f);
    movement.ProcessMovement(&player, &data);
    EXPECT_NEAR(player.m_vecVelocity[0], 48.0f, 1e-3);
    EXPECT_EQ(player.m_vecVelocity[2], 0.0f);
    EXPECT_NEAR(player.m_vecAbsOrigin[0], 48.0f * 0.015f, 1e-3);
}

TEST(GameMovement, BasicJumpCase)
{
    const float maxspeed = 320.0f;
    Strafe::CGameMovement movement;
    Strafe::CBasePlayer player;
    player.m_GroundEntity.m_bValid = true;
    Strafe::CMoveData data;

    player.m_nFlags |= FL_ONGROUND;
    player.m_nButtons |= IN_JUMP;
    player.m_flClientMaxSpeed = maxspeed;

    EXPECT_EQ(player.m_vecVelocity[2], 0.0f);
    movement.ProcessMovement(&player, &data);
    EXPECT_EQ(player.m_vecVelocity[2], 146.5f);
}

using namespace Strafe;

TEST(GameMovement, FallDamage)
{
    const float maxspeed = 320.0f;
    Strafe::CGameMovement movement;
    Strafe::CBasePlayer player;
    Strafe::CMoveData data;
    player.m_vecVelocity[2] = -1000.0f;
    int damage = 0;
    int iteration = 0;

    auto trace = [&](const Ray_t & t, const CBasePlayer &player, unsigned int fMask) {
        trace_t rval = TracePlayerDefault(t, player, fMask);
        // Report the first 5 trace checks as not ground
        if(iteration > 5)
        {
            rval.m_pEnt.m_bValid = true;
        }
        ++iteration;
        return rval;
    };
    auto damagetaken = [&](int dmg) { damage += dmg; };
    movement.m_gameVars.tracePlayerFunc = trace;
    movement.m_gameVars.takeDamageFunc = damagetaken;

    movement.ProcessMovement(&player, &data);
    EXPECT_EQ(damage, 94);
}
