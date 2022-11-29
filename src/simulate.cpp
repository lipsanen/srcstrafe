#include "strafe.hpp"
#include <algorithm>

using namespace Strafe;

static const float PLAYER_MAX_SAFE_FALL_SPEED = 580.0f;
static constexpr int MAX_CLIP_PLANES = 5;

static int TryPlayerMove(PlayerData &mv, const MovementVars &vars, Vector *pFirstDest = NULL, TraceResult *pFirstTrace = NULL);

static void Friction(PlayerData& mv, const MovementVars &vars)
{
	if (!mv.OnGround)
		return;

	float speed = mv.m_vecVelocity.Length2D();
	if (speed < 0.1f)
		return;

	float friction = float{vars.Friction * vars.EntFriction};
	float control = (speed < vars.Stopspeed) ? vars.Stopspeed : speed;
	float drop = control * friction * vars.Frametime;
	float newspeed = std::max(speed - drop, 0.f);
	if (newspeed != speed)
	{
		mv.m_vecVelocity.Scale(newspeed);
	}
}

static void StartGravity(PlayerData& mv, const MovementVars& vars)
{
	float ent_gravity = 1.0;

	// Add gravity so they'll be in the correct position during movement
	// yes, this 0.5 looks wrong, but it's not.  
	mv.m_vecVelocity[2] -= (ent_gravity * vars.Gravity * 0.5 * vars.Frametime );
	mv.m_vecVelocity[2] += mv.Basevelocity[2] * vars.Frametime;

	Vector temp = mv.Basevelocity;
	temp[ 2 ] = 0;
	mv.Basevelocity = temp;

	CheckVelocity(mv, vars);
}

static void WaterJump(PlayerData& mv, const MovementVars &vars)
{
	if (mv.m_flWaterJumpTime > 10000)
		mv.m_flWaterJumpTime = 10000;

	if (!mv.m_flWaterJumpTime)
		return;

	mv.m_flWaterJumpTime -= 1000.0f * vars.Frametime;

	if (mv.m_flWaterJumpTime <= 0 || mv.m_waterLevel == WaterLevel::WL_NotInWater)
	{
		mv.m_flWaterJumpTime = 0;
		mv.m_nFlags &= ~FL_WATERJUMP;
	}
	
	mv.m_vecVelocity[0] = mv.m_vecWaterJumpVel[0];
	mv.m_vecVelocity[1] = mv.m_vecWaterJumpVel[1];
}

static bool CheckWater(PlayerData& mv, const MovementVars &vars)
{
	return false; // TODO: Fix
}

static void CheckWaterJump(PlayerData& mv, const MovementVars &vars)
{
}

static void CheckJumpButton(PlayerData& mv, const MovementVars& vars)
{

}

static void WaterMove(PlayerData& mv, const MovementVars& vars)
{

}

static void CategorizePosition(PlayerData& mv, const MovementVars& vars)
{

}

static void CheckVelocity(PlayerData& mv, const MovementVars& vars)
{

}

static void FinishGravity(PlayerData& mv, const MovementVars& vars)
{

}

static void CheckFalling(PlayerData& mv, const MovementVars& vars)
{

	if(mv.m_flWaterJumpTime)
		return;

	float ent_gravity = 1.0f;

	mv.m_vecVelocity[2] -= (ent_gravity * vars.Gravity * vars.Frametime * 0.5);
	CheckVelocity(mv, vars);
}

static void StepMove(PlayerData &mv, const MovementVars &vars, const Vector& dest, TraceResult& trace)
{
	Vector vecEndPos;
	VectorCopy( dest, vecEndPos );

	// Try sliding forward both on ground and up 16 pixels
	//  take the move that goes farthest
	Vector vecPos, vecVel;
	VectorCopy( mv.m_vecAbsOrigin, vecPos );
	VectorCopy( mv.m_vecVelocity, vecVel );

	// Slide move down.
	TryPlayerMove( mv, vars, &vecEndPos, &trace );
	
	// Down results.
	Vector vecDownPos, vecDownVel;
	VectorCopy( mv.m_vecAbsOrigin, vecDownPos );
	VectorCopy( mv.m_vecVelocity, vecDownVel );
	
	// Reset original values.
	mv.m_vecAbsOrigin = vecPos;
	VectorCopy( vecVel, mv.m_vecVelocity );
	
	// Move up a stair height.
	VectorCopy( mv.m_vecAbsOrigin, vecEndPos );
	if ( vars.m_bAllowAutoMovement )
	{
		vecEndPos.z += vars.Stepsize + vars.DIST_EPSILON;
	}
	
	trace = vars.traceFunc(mv.m_vecAbsOrigin, vecEndPos, mv);
	if ( !trace.StartSolid && !trace.AllSolid )
	{
		mv.m_vecAbsOrigin = trace.EndPos;
	}
	
	// Slide move up.
	TryPlayerMove(mv, vars);
	
	// Move down a stair (attempt to).
	VectorCopy( mv.m_vecAbsOrigin, vecEndPos );
	if ( vars.m_bAllowAutoMovement )
	{
		vecEndPos.z -= vars.Stepsize + vars.DIST_EPSILON;
	}
		
	trace = vars.traceFunc(mv.m_vecAbsOrigin, vecEndPos, mv);
	
	// If we are not on the ground any more then use the original movement attempt.
	if ( trace.Plane[2] < 0.7 )
	{
		mv.m_vecAbsOrigin = vecDownPos;
		VectorCopy( vecDownVel, mv.m_vecVelocity );
		float flStepDist = mv.m_vecAbsOrigin.z - vecPos.z;
		if ( flStepDist > 0.0f )
		{
			mv.m_outStepHeight += flStepDist;
		}
		return;
	}
	
	// If the trace ended up in empty space, copy the end over to the origin.
	if ( !trace.StartSolid && !trace.AllSolid )
	{
		mv.m_vecAbsOrigin = trace.EndPos;
	}
	
	// Copy this origin to up.
	Vector vecUpPos;
	VectorCopy( mv.m_vecAbsOrigin, vecUpPos );
	
	// decide which one went farther
	float flDownDist = ( vecDownPos.x - vecPos.x ) * ( vecDownPos.x - vecPos.x ) + ( vecDownPos.y - vecPos.y ) * ( vecDownPos.y - vecPos.y );
	float flUpDist = ( vecUpPos.x - vecPos.x ) * ( vecUpPos.x - vecPos.x ) + ( vecUpPos.y - vecPos.y ) * ( vecUpPos.y - vecPos.y );
	if ( flDownDist > flUpDist )
	{
		mv.m_vecAbsOrigin = vecDownPos;
		VectorCopy( vecDownVel, mv.m_vecVelocity );
	}
	else 
	{
		// copy z value from slide move
		mv.m_vecVelocity.z = vecDownVel.z;
	}
	
	float flStepDist = mv.m_vecAbsOrigin.z - vecPos.z;
	if ( flStepDist > 0 )
	{
		mv.m_outStepHeight += flStepDist;
	}
}

static void StayOnGround(PlayerData& mv, const MovementVars& vars)
{

}

static void Accelerate(PlayerData &mv, const MovementVars &vars, const Vector &wishdir, float wishspeed)
{
	int i;
	float addspeed, accelspeed, currentspeed;
	float wishspd;

	wishspd = wishspeed;

	if (mv.m_flWaterJumpTime > 0.0f)
		return;

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

static void AirAccelerate(PlayerData &mv, const MovementVars &vars, const Vector &wishdir, float wishspeed)
{
	int i;
	float addspeed, accelspeed, currentspeed;
	float wishspd;

	wishspd = wishspeed;

	if (mv.m_flWaterJumpTime > 0.0f)
		return;

	// Cap speed
	if (wishspd > vars.WishspeedCap)
		wishspd = vars.WishspeedCap;

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

static void FullWalkMove(PlayerData &mv, const MovementVars &vars, const StrafeOutput& output)
{
	if ( !CheckWater(mv, vars) ) 
	{
		StartGravity(mv, vars);
	}

	// If we are leaping out of the water, just update the counters.
	if (mv.m_flWaterJumpTime)
	{
		WaterJump(mv, vars);
		TryPlayerMove(mv, vars);
		// See if we are still in water?
		CheckWater(mv, vars);
		return;
	}

	// If we are swimming in the water, see if we are nudging against a place we can jump up out
	//  of, and, if so, start out jump.  Otherwise, if we are not moving up, then reset jump timer to 0
	if ( mv.m_waterLevel >= WaterLevel::WL_Waist ) 
	{
		if ( mv.m_waterLevel == WaterLevel::WL_Waist )
		{
			CheckWaterJump(mv, vars);
		}

			// If we are falling again, then we must not trying to jump out of water any more.
		if ( mv.m_vecVelocity[2] < 0 && 
			 mv.m_flWaterJumpTime )
		{
			mv.m_flWaterJumpTime = 0;
		}

		// Was jump button pressed?
		if (mv.m_nButtons & IN_JUMP)
		{
			CheckJumpButton(mv, vars);
		}
		else
		{
			mv.m_nOldButtons &= ~IN_JUMP;
		}

		// Perform regular water movement
		WaterMove(mv, vars);

		// Redetermine position vars
		CategorizePosition(mv, vars);

		// If we are on ground, no downward velocity.
		if ( mv.OnGround )
		{
			mv.m_vecVelocity[2] = 0;
		}
	}
	else
	// Not fully underwater
	{
		// Was jump button pressed?
		if (mv.m_nButtons & IN_JUMP)
		{
 			CheckJumpButton(mv, vars);
		}
		else
		{
			mv.m_nOldButtons &= ~IN_JUMP;
		}

		// Fricion is handled before we add in any base velocity. That way, if we are on a conveyor, 
		//  we don't slow when standing still, relative to the conveyor.
		if (mv.OnGround)
		{
			mv.m_vecVelocity[2] = 0.0;
			Friction(mv, vars);
		}

		// Make sure velocity is valid.
		CheckVelocity(mv, vars);

		if (mv.OnGround)
		{
			WalkMove(mv, vars, output);
		}
		else
		{
			AirMove(mv, vars, output);  // Take into account movement when in air.
		}

		// Set final flags.
		CategorizePosition(mv, vars);

		// Make sure velocity is valid.
		CheckVelocity(mv, vars);

		// Add any remaining gravitational component.
		if ( !CheckWater(mv, vars) )
		{
			FinishGravity(mv, vars);
		}

		// If we are on ground, no downward velocity.
		if ( mv.OnGround )
		{
			mv.m_vecVelocity[2] = 0;
		}
		CheckFalling(mv, vars);
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

	return blocked;
}


static void WalkMove(PlayerData &mv, const MovementVars &vars, const StrafeOutput &output)
{
		int i;

	Vector wishvel;
	float spd;
	float fmove, smove;
	Vector wishdir;
	float wishspeed;

	Vector dest;
	Strafe::TraceResult pm;
	Vector forward, right, up;

	AngleVectors (mv.m_vecViewAngles, &forward, &right, &up);  // Determine movement angles

	bool oldground = mv.OnGround;
	
	// Copy movement amounts
	fmove = output.Move[0];
	smove = output.Move[1];

	// Zero out z components of movement vectors
	if ( forward[2] != 0 )
	{
		forward[2] = 0;
		forward.VectorNormalize();
	}

	if ( right[2] != 0 )
	{
		right[2] = 0;
		right.VectorNormalize();
	}

	for (i=0 ; i<2 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	
	wishvel[2] = 0;             // Zero out z part of velocity

	VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
	wishspeed = wishdir.VectorNormalize();

	//
	// Clamp to server defined max speed
	//
	if ((wishspeed != 0.0f) && (wishspeed > vars.Maxspeed))
	{
		VectorScale (wishvel, vars.Maxspeed/wishspeed, wishvel);
		wishspeed = vars.Maxspeed;
	}

	// Set pmove velocity
	mv.m_vecVelocity[2] = 0;
	Accelerate( mv, vars, wishdir, wishspeed);
	mv.m_vecVelocity[2] = 0;

	// Add in any base velocity to the current velocity.
	VecAdd (mv.m_vecVelocity, mv.Basevelocity, mv.m_vecVelocity );

	spd = mv.m_vecVelocity.Length();

	if ( spd < 1.0f )
	{
		mv.m_vecVelocity.Zero();
		// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
		VecSubtract(mv.m_vecVelocity, mv.Basevelocity, mv->m_vecVelocity );
		return;
	}

	// first try just moving to the destination	
	dest[0] = mv.m_vecAbsOrigin[0] + mv.m_vecVelocity[0]*vars.Frametime;
	dest[1] = mv.m_vecAbsOrigin[1] + mv.m_vecVelocity[1]*vars.Frametime;
	dest[2] = mv.m_vecAbsOrigin[2];

	// first try moving directly to the next spot
	vars.traceFunc(mv.m_vecAbsOrigin, dest, mv);

	// If we made it all the way, then copy trace end as new player position.
	VecAdd(mv.m_outWishVel, VectorMult(wishdir, wishspeed), mv.m_outWishVel);

	if ( pm.Fraction == 1 )
	{
		mv.m_vecAbsOrigin = pm.EndPos;
		// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
		VecSubtract(mv.m_vecVelocity, mv.Basevelocity, mv.m_vecVelocity );

		StayOnGround(mv, vars);
		return;
	}

	// Don't walk up stairs if not on ground.
	if ( oldground == NULL && mv.m_waterLevel == WaterLevel::WL_NotInWater )
	{
		// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
		VecSubtract(mv.m_vecVelocity, mv.Basevelocity, mv.m_vecVelocity );
		return;
	}

	// If we are jumping out of water, don't do anything more.
	if ( mv.m_flWaterJumpTime )         
	{
		// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
		VecSubtract(mv.m_vecVelocity, mv.Basevelocity, mv.m_vecVelocity );
		return;
	}

	StepMove(mv, vars, dest, pm );

	// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
	VecSubtract(mv.m_vecVelocity, mv.Basevelocity, mv.m_vecVelocity );

	StayOnGround(mv, vars);
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
	auto output = Strafe::Strafe(player, vars, input);
	FullWalkMove(player, vars, output);
}
