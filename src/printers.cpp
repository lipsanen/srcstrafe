#include <ostream>
#include "strafe_utils.hpp"
#include "strafe.hpp"

namespace Strafe
{
    std::ostream &operator<<(std::ostream &os, HullType HullType)
    {
        switch (HullType)
        {
        case Strafe::HullType::Standing:
            os << "Standing";
            break;
        case Strafe::HullType::Crouching:
            os << "Crouching";
            break;
        case Strafe::HullType::NumValues:
            os << "NumValues";
            break;
        }
        return os;
    }
    std::ostream &operator<<(std::ostream &os, const Vector &v)
    {
        os << "Vector(";
        os << "x=" << v.x;
        os << ", y=" << v.y;
        os << ", z=" << v.z;
        os << ")";
        return os;
    }
    std::ostream &operator<<(std::ostream &os, const PlayerData &v)
    {
        os << "PlayerData(";
        os << "m_vecAbsOrigin=" << v.m_vecAbsOrigin;
        os << ", m_vecVelocity=" << v.m_vecVelocity;
        os << ", Basevelocity=" << v.Basevelocity;
        os << ", Ducking=" << v.Ducking;
        os << ", DuckPressed=" << v.DuckPressed;
        os << ", OnGround=" << v.OnGround;
        os << ", CantJump=" << v.CantJump;
        os << ")";
        return os;
    }
    std::ostream &operator<<(std::ostream &os, const TraceResult &v)
    {
        os << "TraceResult(";
        os << "AllSolid=" << v.AllSolid;
        os << ", StartSolid=" << v.StartSolid;
        os << ", Fraction=" << v.Fraction;
        os << ", EndPos=" << v.EndPos;
        os << ")";
        return os;
    }
    std::ostream &operator<<(std::ostream &os, StrafeType StrafeType)
    {
        switch (StrafeType)
        {
        case Strafe::StrafeType::MaxAccel:
            os << "MaxAccel";
            break;
        case Strafe::StrafeType::MaxAngle:
            os << "MaxAngle";
            break;
        case Strafe::StrafeType::MaxAccelCapped:
            os << "MaxAccelCapped";
            break;
        case Strafe::StrafeType::Direction:
            os << "Direction";
            break;
        case Strafe::StrafeType::NumValues:
            os << "NumValues";
            break;
        }
        return os;
    }
    std::ostream &operator<<(std::ostream &os, JumpType JumpType)
    {
        switch (JumpType)
        {
        case Strafe::JumpType::None:
            os << "None";
            break;
        case Strafe::JumpType::ABH:
            os << "ABH";
            break;
        case Strafe::JumpType::Bhop:
            os << "Bhop";
            break;
        case Strafe::JumpType::Glitchless:
            os << "Glitchless";
            break;
        }
        return os;
    }
    std::ostream &operator<<(std::ostream &os, const StrafeInput &v)
    {
        os << "StrafeInput(";
        os << "Stype=" << v.Stype;
        os << ", Jtype=" << v.Jtype;
        os << ", CappedLimit=" << v.CappedLimit;
        os << ", TargetYaw=" << v.TargetYaw;
        os << ", VectorialOffset=" << v.VectorialOffset;
        os << ", AngleSpeed=" << v.AngleSpeed;
        os << ", Scale=" << v.Scale;
        os << ", AFH=" << v.AFH;
        os << ", Vectorial=" << v.Vectorial;
        os << ", JumpOverride=" << v.JumpOverride;
        os << ", Strafe=" << v.Strafe;
        os << ", Version=" << v.Version;
        os << ")";
        return os;
    }
    std::ostream &operator<<(std::ostream &os, const MovementVars &v)
    {
        os << "MovementVars(";
        os << "Accelerate=" << v.Accelerate;
        os << ", Airaccelerate=" << v.Airaccelerate;
        os << ", EntFriction=" << v.EntFriction;
        os << ", Frametime=" << v.Frametime;
        os << ", Friction=" << v.Friction;
        os << ", Maxspeed=" << v.Maxspeed;
        os << ", Stopspeed=" << v.Stopspeed;
        os << ", WishspeedCap=" << v.WishspeedCap;
        os << ", EntGravity=" << v.EntGravity;
        os << ", Maxvelocity=" << v.Maxvelocity;
        os << ", Gravity=" << v.Gravity;
        os << ", Stepsize=" << v.Stepsize;
        os << ", Bounce=" << v.Bounce;
        os << ", ReduceWishspeed=" << v.ReduceWishspeed;
        os << ")";
        return os;
    }
    std::ostream &operator<<(std::ostream &os, const StrafeOutput &v)
    {
        os << "StrafeOutput(";
        os << "Success=" << v.Success;
        os << ", Error=" << v.Error;
        os << ", Move=" << v.Move;
        os << ", Jump=" << v.Jump;
        os << ")";
        return os;
    }
}
