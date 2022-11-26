#include "printers.hpp"
#include "strafe.hpp"
#include <stdint.h>
#include <cstring>
#include <cstdio>
#include <iostream>

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

void printAndAbort(const Strafe::PlayerData& data, const Strafe::MovementVars& vars, const Strafe::StrafeInput& input, double theta)
{
  std::cout << data << std::endl;
  std::cout << input << std::endl;
  std::cout << vars << std::endl;
  std::cout << "theta: " << theta << std::endl;
  std::fprintf(stderr, "vel (%f, %f, %f), 2d %f, ground %d, theta %f\n", data.m_vecVelocity.x, data.m_vecVelocity.y, data.m_vecVelocity.z, data.m_vecVelocity.Length2D(), data.OnGround, theta);
  abort();
}

extern "C" int LLVMFuzzerTestOneInput(const uint8_t *data, size_t size) {
  const float maxVel = 3500;
  Strafe::PlayerData pl;
  TRY_READ_FLOAT(pl.m_vecVelocity.x, -maxVel, maxVel);
  TRY_READ_FLOAT(pl.m_vecVelocity.y, -maxVel, maxVel);
  TRY_READ_FLOAT(pl.m_vecVelocity.z, -maxVel, maxVel);
  TRY_READ_BOOL(pl.OnGround, data, size);

  Strafe::MovementVars vars;
  TRY_READ_FLOAT(vars.Accelerate, 0.01, 100);
  TRY_READ_FLOAT(vars.Airaccelerate, 0.01, 100);
  TRY_READ_FLOAT(vars.WishspeedCap, 0.01, 320);
  TRY_READ_FLOAT(vars.Maxspeed, 100, 1000);
  
  Strafe::StrafeInput input;
  input.CappedLimit = 299.99;
  input.Strafe = true;
  TRY_READ_ENUM(input.Stype, Strafe::StrafeType::NumValues);

  double value = Strafe::StrafeTheta(pl, vars, input);

  if(std::isnan(value) || !std::isfinite(value) || value > M_PI || value < 0)
  {
    printAndAbort(pl, vars, input, value);
  }

  return 0;
}