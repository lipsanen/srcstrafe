#include "gtest/gtest.h"
#include "srcstrafe/gamemovement.h"
#include "srcstrafe/in_buttons.h"
#include "srcstrafe/surface.hpp"

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

static Strafe::TracePlayer_t GetZTraceFunc(float floorZ)
{
    
    
    Strafe::TracePlayer_t func = [=](const Ray_t & t, const CBasePlayer &player, unsigned int fMask) {
        trace_t result;
        float len = std::abs(t.end.z - t.start.z);
        if(t.end.z <= floorZ)
        {
            float traveled = std::abs(floorZ - t.start.z);
            if(len < 1e-3)
                result.fraction = 1.0f;
            else
                result.fraction = traveled / len;
            result.m_pEnt.m_bValid = true;
            Vector delta;
            Vector start = t.start;
            VecSubtract(t.end, start, delta);
            result.endpos = start + VectorMult(delta, result.fraction);
        }
        else
        {
            result.fraction = 1.0f;
        }

        return result;
    };

    return func;
}

TEST(GameMovement, FallDamage)
{
    const float maxspeed = 320.0f;
    Strafe::CGameMovement movement;
    Strafe::CBasePlayer player;
    Strafe::CMoveData data;
    player.m_vecVelocity[2] = -1000.0f;
    int damage = 0;
    int iteration = 0;

    Strafe::Surface surface = surface.GetZSurface(-10.0f);
    auto damagetaken = [&](int dmg) { damage += dmg; };
    movement.m_gameVars.tracePlayerFunc = [&] (const Ray_t & r, const CBasePlayer &player, unsigned int fMask) {
        return surface.Trace(r, player, fMask);
    };
    movement.m_gameVars.takeDamageFunc = damagetaken;

    movement.ProcessMovement(&player, &data);
    EXPECT_EQ(damage, 94);
    EXPECT_EQ(player.m_vecAbsOrigin[2], -10.0f);
}

TEST(GameMovement, BoundingBoxed)
{
    const float EPS = 1e-5;
    {
        for(size_t i=0; i < 3; ++i)
        {
            Strafe::CGameMovement movement;
            Strafe::CBasePlayer player;
            Strafe::CMoveData data;
            player.m_vecVelocity[i] = 1000.0f;

            Strafe::Surface surface;
            surface.pos[i] = GetPlayerMaxs(&player)[i] + 1.0f;
            surface.normal.Init();
            surface.normal[i] = -1.0f;
            movement.m_gameVars.tracePlayerFunc = [&] (const Ray_t & r, const CBasePlayer &player, unsigned int fMask) {
                return surface.Trace(r, player, fMask);
            };

            movement.ProcessMovement(&player, &data);
            EXPECT_NEAR(player.m_vecAbsOrigin[i], 1.0f, EPS);
        }
    }

    {
        for(size_t i=0; i < 3; ++i)
        {
            Strafe::CGameMovement movement;
            Strafe::CBasePlayer player;
            Strafe::CMoveData data;
            player.m_vecVelocity[i] = -1000.0f;

            Strafe::Surface surface;
            surface.pos[i] = GetPlayerMins(&player)[i] - 3.0f;
            surface.normal.Init();
            surface.normal[i] = 1.0f;
            movement.m_gameVars.tracePlayerFunc = [&] (const Ray_t & r, const CBasePlayer &player, unsigned int fMask) {
                return surface.Trace(r, player, fMask);
            };

            movement.ProcessMovement(&player, &data);
            EXPECT_NEAR(player.m_vecAbsOrigin[i], -3.0f, EPS);
        }
    }
}