#pragma once
#include "strafe_utils.h"
#include <functional>

namespace Strafe
{
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
