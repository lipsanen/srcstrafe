#include "gtest/gtest.h"
#include "srcstrafe/gamemovement.h"
#include "srcstrafe/in_buttons.h"
#include "srcstrafe/surface.hpp"
#include <filesystem>
#include <iostream>
#include <fstream>

using namespace Strafe;


struct StrafeState {
  float x, y, z;
  float vel_x, vel_y, vel_z;
  float pitch, yaw, roll;
  float fmove, smove, upmove;
  int buttons;
  int m_fFlags;
  float m_flDucktime;
  float m_flDuckJumpTime;
  float m_flJumptime;
  bool m_bDucked;
  bool m_bDucking;
  bool m_bInDuckJump;
  int m_hGroundEntity;
  bool has_entity_data;
};

std::ostream& operator<<(std::ostream& os, const StrafeState& state) {
  os
  << state.x << " "
  << state.y << " "
  << state.z << " "
  << state.vel_x << " "
  << state.vel_y << " "
  << state.vel_z << " "
  << state.pitch << " "
  << state.yaw << " "
  << state.roll << " "
  << state.fmove << " "
  << state.smove << " "
  << state.upmove << " "
  << state.buttons << " "
  << state.m_fFlags << " "
  << state.m_flDucktime << " "
  << state.m_flDuckJumpTime << " "
  << state.m_flJumptime << " "
  << state.m_bDucked << " "
  << state.m_bDucking << " "
  << state.m_bInDuckJump << " "
  << state.m_hGroundEntity;
  return os;
}

std::istream& operator>>(std::istream& is, StrafeState& state) {
  is
  >> state.x
  >> state.y
  >> state.z
  >> state.vel_x
  >> state.vel_y
  >> state.vel_z
  >> state.pitch
  >> state.yaw
  >> state.roll
  >> state.fmove
  >> state.smove
  >> state.upmove
  >> state.buttons
  >> state.m_fFlags
  >> state.m_flDucktime
  >> state.m_flDuckJumpTime
  >> state.m_flJumptime
  >> state.m_bDucked
  >> state.m_bDucking
  >> state.m_bInDuckJump
  >> state.m_hGroundEntity;
  return is;
}

StrafeState PredictMovement(const StrafeState& input) {
  const float maxspeed = 150.0f;
  StrafeState state = input;
  Strafe::CGameMovement movement;
  Strafe::CBasePlayer player;
  Strafe::CMoveData data;

  if(input.m_hGroundEntity != 2097151) {
    player.m_GroundEntity.m_bValid = true;
    player.m_GroundEntity.index = 0;
  }

  player.m_nButtons = input.buttons;
  player.m_vecAbsOrigin[0] = input.x;
  player.m_vecAbsOrigin[1] = input.y;
  player.m_vecAbsOrigin[2] = input.z;
  player.m_vecVelocity[0] = input.vel_x;
  player.m_vecVelocity[1] = input.vel_y;
  player.m_vecVelocity[2] = input.vel_z;
  player.m_nFlags = input.m_fFlags;
  player.m_flClientMaxSpeed = maxspeed;
  data.m_flForwardMove = input.fmove;
  data.m_flSideMove = input.smove;
  data.m_flUpMove = input.upmove;
  data.m_vecViewAngles[0] = input.pitch;
  data.m_vecViewAngles[1] = input.yaw;
  data.m_vecViewAngles[2] = input.roll;

  movement.ProcessMovement(&player, &data);

  state.x = player.m_vecAbsOrigin[0];
  state.y = player.m_vecAbsOrigin[1];
  state.z = player.m_vecAbsOrigin[2];
  state.vel_x = player.m_vecVelocity[0];
  state.vel_y = player.m_vecVelocity[1];
  state.vel_z = player.m_vecVelocity[2];
  state.m_fFlags = player.m_nFlags;

  return state;
}

bool Match(const StrafeState& lhs, const StrafeState& rhs) {
  const float EPS = 1e-1;
  EXPECT_NEAR(lhs.x, rhs.x, EPS);
  EXPECT_NEAR(lhs.y, rhs.y, EPS);
  EXPECT_NEAR(lhs.z, rhs.z, EPS);
  EXPECT_NEAR(lhs.vel_x, rhs.vel_x, EPS);
  EXPECT_NEAR(lhs.vel_y, rhs.vel_y, EPS);
  EXPECT_NEAR(lhs.vel_z, rhs.vel_z, EPS);
  return ::testing::Test::HasFailure();
}

TEST(TestCases, Run) {
  std::error_code ec;
  for(auto& path : std::filesystem::directory_iterator("./testcases", ec)) {
    std::cout << path << std::endl;
    bool has_predicted = false;
    StrafeState predicted;
    StrafeState state;
    std::ifstream strim;
    strim.open(path.path().c_str(), std::ios::in);

    while(strim.good() && !strim.eof()) {
      strim >> state;
      if(has_predicted) {
        if(!Match(predicted, state))
          break;
      }
      predicted = PredictMovement(state);
      has_predicted = true;
    }
  }
}

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

TEST(GameMovement, FallDamage)
{
    Strafe::CGameMovement movement;
    Strafe::CBasePlayer player;
    Strafe::CMoveData data;
    player.m_vecVelocity[2] = -1000.0f;
    int damage = 0;

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

TEST(GameMovement, JumpAndLand)
{
    const float z = 960.031250f;
    Strafe::CGameMovement movement;
    Strafe::CBasePlayer player;
    Strafe::CMoveData data;
    player.m_nButtons |= IN_JUMP;
    player.m_vecAbsOrigin[2] = z;

    Strafe::Surface surface = Surface::GetZSurface(z);
    movement.m_gameVars.tracePlayerFunc = [&] (const Ray_t & r, const CBasePlayer &player, unsigned int fMask) {
        return surface.Trace(r, player, fMask);
    };

    for(size_t i=0; i < 35; ++i)
    {
        movement.ProcessMovement(&player, &data);
    }
    EXPECT_NEAR(player.m_vecAbsOrigin[2], 961.306274f, 1e-5);
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