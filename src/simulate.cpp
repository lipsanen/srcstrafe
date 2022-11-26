#include "strafe.hpp"
#include <algorithm>

using namespace Strafe;

static void Friction(PlayerData& player, const MovementVars& vars)
{
    if (!player.OnGround)
        return;

    float speed = player.m_vecVelocity.Length2D();
    if (speed < 0.1f)
        return;

    float friction = float{vars.Friction * vars.EntFriction};
    float control = (speed < vars.Stopspeed) ? vars.Stopspeed : speed;
    float drop = control * friction * vars.Frametime;
    float newspeed = std::max(speed - drop, 0.f);
    if(newspeed != speed)
    {
        player.m_vecVelocity.Scale(newspeed);
    }
}

static void WalkMove(PlayerData& player, const MovementVars& vars, const StrafeOutput& output)
{

}

static void AirAccelerate(PlayerData& mv, const MovementVars& vars, const Vector& wishdir, float wishspeed)
{
	int i;
	float addspeed, accelspeed, currentspeed;
	float wishspd;

	wishspd = wishspeed;

	if (mv.m_flWaterJumpTime > 0.0f)
		return;

	// Cap speed
	if (wishspd > 30)
		wishspd = 30;

	// Determine veer amount
	currentspeed = mv.m_vecVelocity.Dot(wishdir);

	// See how much to add
	addspeed = wishspd - currentspeed;

	// If not adding any, done.
	if (addspeed <= 0)
		return;

	// Determine acceleration speed after acceleration
	accelspeed = vars.Airaccelerate * wishspeed * vars.Frametime * mv.m_surfaceFriction;

	// Cap it
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	// Adjust pmove vel.
	for (i=0 ; i<3 ; i++)
	{
		mv.m_vecVelocity[i] += accelspeed * wishdir[i];
		mv.m_outWishVel[i] += accelspeed * wishdir[i];
	}
}

static void TryPlayerMove(PlayerData& mv, const MovementVars& vars)
{

}

static void AirMove(PlayerData& mv, const MovementVars& vars, const StrafeOutput& output)
{
	int			i;
	Vector		wishvel;
	float		fmove, smove;

	float		wishspeed;
	Vector forward, right, up;

	AngleVectors (mv.m_vecViewAngles, &forward, &right, &up);  // Determine movement angles
	
	// Copy movement amounts
	fmove = output.Move[FMOVE_INDEX];
	smove = output.Move[SMOVE_INDEX];
	
	// Zero out z components of movement vectors
	forward[2] = right[2] = 0;
    forward.VectorNormalize();
    right.VectorNormalize();

	for (i=0 ; i<2 ; i++)
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	wishvel[2] = 0;

    Vector wishdir = wishvel;
    float wishspeed = wishdir.VectorNormalize();

	// clamp to server defined max speed
	if ( wishspeed != 0 && (wishspeed > vars.Maxspeed))
	{
        wishvel.Scale(vars.Maxspeed);
	}
	
	AirAccelerate(mv, vars, wishdir, wishspeed);

    mv.m_vecVelocity.Add(mv.Basevelocity);

	TryPlayerMove(mv, vars);
	
    mv.m_vecVelocity.Subtract(mv.Basevelocity);
}

static void Reset(PlayerData& player)
{
    player.m_outWishVel.Scale(0);
}

void Simulate(PlayerData& player, const MovementVars& vars, const StrafeInput& input)
{
    Friction(player, vars);
    auto output = Strafe::Strafe(player, vars, input);

    if(player.OnGround)
    {
        WalkMove(player, vars, output);
    }
    else
    {
        AirMove(player, vars, output);
    }
}
