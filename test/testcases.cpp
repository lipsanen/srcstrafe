#include "gtest/gtest.h"
#include "srcstrafe/gamemovement.h"

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
    EXPECT_EQ(player.m_vecAbsOrigin[0], 30.0f * 0.015f);
}

TEST(GameMovement, BasicGroundCase)
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
    EXPECT_EQ(player.m_vecAbsOrigin[0], 30.0f * 0.015f);
}

