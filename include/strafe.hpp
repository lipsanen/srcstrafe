#pragma once
#include "strafe_utils.hpp"
#include <functional>

namespace Strafe
{
	enum class StrafeType
	{
		MaxAccel,
		MaxAngle,
		MaxAccelCapped,
		Direction,
		NumValues
	};
	enum class JumpType
	{
		None,
		ABH,
		Bhop,
		Glitchless
	};

	TraceResult TraceDefault(const Vector &start, const Vector &end, const PlayerData &data);
	bool TraceGroundDefault(PlayerData &data);

	typedef std::function<TraceResult(const Vector &start, const Vector &end, const PlayerData &data)> TraceFunc;
	typedef std::function<bool(PlayerData &)> TraceGround;

	struct StrafeInput
	{
		float Sidemove = 0.0f;
		float Forwardmove = 0.0f;
		StrafeType Stype = StrafeType::MaxAccel;
		JumpType Jtype = JumpType::ABH;
		float CappedLimit = 0.0f;
		double TargetYaw = 0.0;
		float VectorialOffset = 0.0f;
		float AngleSpeed = 0.0f;
		float Scale = 1.0f;
		float MaxAngleEps = 1e-4f;
		bool AFH = true;
		bool Vectorial = true;
		bool JumpOverride = true;
		bool Strafe = true;
		int Version = 6;
	};

	struct MovementVars
	{
		float Accelerate = 10;
		float Airaccelerate = 10;
		float EntFriction = 1;
		float Frametime = 0.015f;
		float Friction = 4;
		float Maxspeed = 320;
		float Stopspeed = 10;
		float WishspeedCap = 30;

		float EntGravity = 1;
		float Maxvelocity = 3500;
		float Gravity = 600;
		float Stepsize = 20;
		float Bounce = 0.0f;
		bool ReduceWishspeed = false;
		TraceFunc traceFunc = TraceDefault;
		TraceGround groundFunc = TraceGroundDefault;
	};

	struct StrafeOutput
	{
		bool Success = false;
		const char *Error = nullptr;
		Vector Move;
		bool Jump = false;
	};

	double TargetTheta(const PlayerData &player,
					   const MovementVars &vars,
					   double target, bool forceIterative = false);
	StrafeOutput Strafe(const PlayerData &player, const MovementVars &vars, const StrafeInput &input);
	double StrafeTheta(const PlayerData &player, const MovementVars &vars, const StrafeInput &input);
	void Simulate(PlayerData &player, const MovementVars &vars, const StrafeInput &input);
	bool OvershotCap(const PlayerData &player, const MovementVars &vars, const StrafeInput &input);
	void VectorFME(PlayerData &player, const MovementVars &vars, double theta);
	double GetNewSpeed(const PlayerData &player, const MovementVars &vars, double theta);
	double GetDotWithOld(const PlayerData &player, const MovementVars &vars, double theta);
}
