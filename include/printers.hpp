#pragma once

#include <iostream>
#include "strafe.hpp"
#include "strafe_utils.hpp"

namespace Strafe
{
    std::ostream &operator<<(std::ostream &os, HullType HullType);
    std::ostream &operator<<(std::ostream &os, const Vector &v);
    std::ostream &operator<<(std::ostream &os, const PlayerData &v);
    std::ostream &operator<<(std::ostream &os, const TraceResult &v);
    std::ostream &operator<<(std::ostream &os, StrafeType StrafeType);
    std::ostream &operator<<(std::ostream &os, JumpType JumpType);
    std::ostream &operator<<(std::ostream &os, const StrafeInput &v);
    std::ostream &operator<<(std::ostream &os, const MovementVars &v);
    std::ostream &operator<<(std::ostream &os, const StrafeOutput &v);
}
