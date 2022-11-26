#include "strafe.hpp"
#include <stdint.h>
#include <cstring>
#include <cstdio>

int readFloat(float& output, const uint8_t*& data, size_t& size, float min, float max)
{
  if(size < sizeof(unsigned))
    return 1;
  
  unsigned value = 0;
  memcpy(&value, data, sizeof(unsigned));

  output = min + (max - min) * ((double)value / UINT32_MAX);

  data += sizeof(unsigned);
  size -= sizeof(unsigned);

  if(!std::isfinite(output))
    abort();

  return 0;
}

#define TRY_READ_FLOAT(output, min, max) if(readFloat(output, data, size, min, max) != 0) return 0;

int readBool(bool& output, const uint8_t*& data, size_t& size)
{
  if(size < 1)
    return 1;
  output = *data > 128;

  return 0;
}

#define TRY_READ_BOOL(output, data, size) if(readBool(output, data, size) != 0) return 0;

template<typename T>
int readUInt(T& output, const uint8_t*& data, size_t& size, const T max)
{
  if(size < sizeof(T))
    return 1;
  std::memcpy(&output, data, sizeof(T));
  output = (T)((int)output % (int)max);
  return 0;
}

#define TRY_READ_ENUM(output, max) if(readUInt(output, data, size, max) != 0) return 0;

void printAndAbort(const Strafe::PlayerData& data, double theta)
{
  std::fprintf(stderr, "vel (%f, %f, %f), 2d %f, ground %d, theta %f\n", data.Velocity.x, data.Velocity.y, data.Velocity.z, data.Velocity.Length2D(), data.OnGround, theta);
  abort();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  const float maxVel = 3500;
  Strafe::PlayerData pl;
  TRY_READ_FLOAT(pl.Velocity.x, -maxVel, maxVel);
  TRY_READ_FLOAT(pl.Velocity.y, -maxVel, maxVel);
  TRY_READ_FLOAT(pl.Velocity.z, -maxVel, maxVel);
  TRY_READ_BOOL(pl.OnGround, data, size);

  Strafe::MovementVars vars;
  TRY_READ_FLOAT(vars.Accelerate, 0.01, 100);
  TRY_READ_FLOAT(vars.Airaccelerate, 0.01, 100);
  vars.WishspeedCap = 60;
  vars.Maxspeed = 150;
  
  Strafe::StrafeInput input;
  input.CappedLimit = 299.99;
  input.Strafe = true;
  TRY_READ_ENUM(input.Stype, Strafe::StrafeType::NumValues);

  double value = Strafe::StrafeTheta(pl, vars, input);

  if(std::isnan(value) || !std::isfinite(value) || value > M_PI || value < 0)
  {
    printAndAbort(pl, value);
  }

  return 0;
}