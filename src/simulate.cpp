#include "strafe.hpp"
#include <algorithm>

using namespace Strafe;

static const float PLAYER_MAX_SAFE_FALL_SPEED = 580.0f;
static constexpr int MAX_CLIP_PLANES = 5;

static void Friction(PlayerData &player, const MovementVars &vars)
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
	if (newspeed != speed)
	{
		player.m_vecVelocity.Scale(newspeed);
	}
}

static void WalkMove(PlayerData &player, const MovementVars &vars, const StrafeOutput &output)
{
}

static void AirAccelerate(PlayerData &mv, const MovementVars &vars, const Vector &wishdir, float wishspeed)
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
	for (i = 0; i < 3; i++)
	{
		mv.m_vecVelocity[i] += accelspeed * wishdir[i];
		mv.m_outWishVel[i] += accelspeed * wishdir[i];
	}
}

static int TryPlayerMove(PlayerData &mv, const MovementVars &vars, Vector *pFirstDest = NULL, TraceResult *pFirstTrace = NULL)
{
	int bumpcount, numbumps;
	Vector dir;
	float d;
	int numplanes;
	Vector planes[MAX_CLIP_PLANES];
	Vector primal_velocity, original_velocity;
	Vector new_velocity;
	int i, j;
	TraceResult pm;
	Vector end;
	float time_left, allFraction;
	int blocked;

	numbumps = 4; // Bump up to four times

	blocked = 0;   // Assume not blocked
	numplanes = 0; //  and not sliding along any planes

	Vector original_velocity = mv.m_vecVelocity;
	Vector primal_velocity = mv.m_vecVelocity;

	allFraction = 0;
	time_left = vars.Frametime; // Total time for this movement operation.

	for (bumpcount = 0; bumpcount < numbumps; bumpcount++)
	{
		if (mv.m_vecVelocity.Length() == 0.0)
			break;

		// Assume we can move all the way from the current origin to the
		//  end point.
		VectorMA(mv.m_vecAbsOrigin, time_left, mv.m_vecVelocity, end);

		// See if we can make it from origin to end point.
		// If their velocity Z is 0, then we can avoid an extra trace here during WalkMove.
		if (pFirstDest && end == *pFirstDest)
			pm = *pFirstTrace;
		else
		{
			pm = vars.traceFunc(mv.m_vecVelocity, end, mv);
		}

		allFraction += pm.Fraction;

		// If we started in a solid object, or we were in solid space
		//  the whole way, zero out our velocity and return that we
		//  are blocked by floor and wall.
		if (pm.AllSolid)
		{
			// entity is trapped in another solid
			mv.m_vecVelocity.Zero();
			return 4;
		}

		// If we moved some portion of the total distance, then
		//  copy the end position into the pmove.origin and
		//  zero the plane counter.
		if (pm.Fraction > 0)
		{
			if (numbumps > 0 && pm.Fraction == 1)
			{
				// There's a precision issue with terrain tracing that can cause a swept box to successfully trace
				// when the end position is stuck in the triangle.  Re-run the test with an uswept box to catch that
				// case until the bug is fixed.
				// If we detect getting stuck, don't allow the movement
				TraceResult stuck = vars.traceFunc(pm.EndPos, pm.EndPos, mv);
				if (stuck.StartSolid || stuck.Fraction != 1.0f)
				{
					// Msg( "Player will become stuck!!!\n" );
					mv.m_vecVelocity.Zero();
					break;
				}
			}

			// actually covered some distance
			mv.m_vecAbsOrigin = pm.EndPos;
			mv.m_vecVelocity = original_velocity;
			numplanes = 0;
		}

		// If we covered the entire distance, we are done
		//  and can return.
		if (pm.Fraction == 1)
		{
			break; // moved the entire distance
		}

		// If the plane we hit has a high z component in the normal, then
		//  it's probably a floor
		if (pm.Plane[2] > 0.7)
		{
			blocked |= 1; // floor
		}
		// If the plane has a zero z component in the normal, then it's a
		//  step or wall
		if (!pm.Plane[2])
		{
			blocked |= 2; // step / wall
		}

		// Reduce amount of m_flFrameTime left by total time left * fraction
		//  that we covered.
		time_left -= time_left * pm.Fraction;

		// Did we run out of planes to clip against?
		if (numplanes >= MAX_CLIP_PLANES)
		{
			// this shouldn't really happen
			//  Stop our movement if so.
			mv.m_vecVelocity.Zero();
			// Con_DPrintf("Too many planes 4\n");

			break;
		}

		// Set up next clipping plane
		planes[numplanes] = pm.Plane;
		numplanes++;

		// modify original_velocity so it parallels all of the clip planes
		//

		// reflect player velocity
		// Only give this a try for first impact plane because you can get yourself stuck in an acute corner by jumping in place
		//  and pressing forward and nobody was really using this bounce/reflection feature anyway...
		if (numplanes == 1 &&
			mv.m_moveType == MoveType::Walk &&
			!mv.OnGround)
		{
			for (i = 0; i < numplanes; i++)
			{
				if (planes[i][2] > 0.7)
				{
					// floor or slope
					ClipVelocity(original_velocity, planes[i], new_velocity, 1);
					VectorCopy(new_velocity, original_velocity);
				}
				else
				{
					ClipVelocity(original_velocity, planes[i], new_velocity, 1.0 + vars.Bounce* (1 - mv.m_surfaceFriction));
				}
			}

			mv.m_vecVelocity = new_velocity;
			original_velocity = new_velocity;
		}
		else
		{
			for (i = 0; i < numplanes; i++)
			{
				ClipVelocity(
					original_velocity,
					planes[i],
					mv.m_vecVelocity,
					1);

				for (j = 0; j < numplanes; j++)
					if (j != i)
					{
						// Are we now moving against this plane?
						if (mv.m_vecVelocity.Dot(planes[j]) < 0)
							break; // not ok
					}
				if (j == numplanes) // Didn't have to clip, so we're ok
					break;
			}

			// Did we go all the way through plane set
			if (i != numplanes)
			{ // go along this plane
				// pmove.velocity is set in clipping call, no need to set again.
				;
			}
			else
			{ // go along the crease
				if (numplanes != 2)
				{
					mv.m_vecVelocity.Zero();
					break;
				}
				CrossProduct(planes[0], planes[1], dir);
				dir.VectorNormalize();
				d = dir.Dot(mv.m_vecVelocity);
				VectorScale(dir, d, mv.m_vecVelocity);
			}

			//
			// if original velocity is against the original velocity, stop dead
			// to avoid tiny occilations in sloping corners
			//
			d = mv.m_vecVelocity.Dot(primal_velocity);
			if (d <= 0)
			{
				// Con_DPrintf("Back\n");
				mv.m_vecVelocity.Zero();
				break;
			}
		}
	}

	if (allFraction == 0)
	{
		mv.m_vecVelocity.Zero();
	}

	// Check if they slammed into a wall
	float fSlamVol = 0.0f;

	float fLateralStoppingAmount = primal_velocity.Length2D() - mv.m_vecVelocity.Length2D();
	if (fLateralStoppingAmount > PLAYER_MAX_SAFE_FALL_SPEED * 2.0f)
	{
		fSlamVol = 1.0f;
	}
	else if (fLateralStoppingAmount > PLAYER_MAX_SAFE_FALL_SPEED)
	{
		fSlamVol = 0.85f;
	}

	return blocked;
}

static void AirMove(PlayerData &mv, const MovementVars &vars, const StrafeOutput &output)
{
	int i;
	Vector wishvel;
	float fmove, smove;

	float wishspeed;
	Vector forward, right, up;

	AngleVectors(mv.m_vecViewAngles, &forward, &right, &up); // Determine movement angles

	// Copy movement amounts
	fmove = output.Move[FMOVE_INDEX];
	smove = output.Move[SMOVE_INDEX];

	// Zero out z components of movement vectors
	forward[2] = right[2] = 0;
	forward.VectorNormalize();
	right.VectorNormalize();

	for (i = 0; i < 2; i++)
		wishvel[i] = forward[i] * fmove + right[i] * smove;
	wishvel[2] = 0;

	Vector wishdir = wishvel;
	float wishspeed = wishdir.VectorNormalize();

	// clamp to server defined max speed
	if (wishspeed != 0 && (wishspeed > vars.Maxspeed))
	{
		wishvel.Scale(vars.Maxspeed);
	}

	AirAccelerate(mv, vars, wishdir, wishspeed);

	mv.m_vecVelocity.Add(mv.Basevelocity);

	TryPlayerMove(mv, vars);

	mv.m_vecVelocity.Subtract(mv.Basevelocity);
}

static void Reset(PlayerData &player)
{
	player.m_outWishVel.Scale(0);
}

void Simulate(PlayerData &player, const MovementVars &vars, const StrafeInput &input)
{
	Friction(player, vars);
	auto output = Strafe::Strafe(player, vars, input);

	if (player.OnGround)
	{
		WalkMove(player, vars, output);
	}
	else
	{
		AirMove(player, vars, output);
	}
}
