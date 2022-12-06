#include "srcstrafe/gamemovement.h"
#include "srcstrafe/in_buttons.h"
#include <stdarg.h>
#include <cstring>
#include <algorithm>

#define	STOP_EPSILON		0.1
#define	MAX_CLIP_PLANES		5

#include <stdarg.h>

// [MD] I'll remove this eventually. For now, I want the ability to A/B the optimizations.
bool g_bMovementOptimizations = true;

// Roughly how often we want to update the info about the ground surface we're on.
// We don't need to do this very often.
#define CATEGORIZE_GROUND_SURFACE_INTERVAL			0.3f
#define CHECK_STUCK_INTERVAL			1.0f
#define CHECK_STUCK_INTERVAL_SP			0.2f
#define CHECK_LADDER_INTERVAL			0.2f

#define	NUM_CROUCH_HINTS	3

using namespace Strafe;


#define CheckV( tick, ctx, vel )

void CGameMovement::DiffPrint( char const *fmt, ... )
{
}

//-----------------------------------------------------------------------------
// Purpose: Constructs GameMovement interface
//-----------------------------------------------------------------------------
CGameMovement::CGameMovement( void )
{
	m_nOldWaterLevel	= WaterLevel::WL_NotInWater;
	m_flWaterEntryTime	= 0;
	m_nOnLadder			= 0;

	mv					= NULL;

	memset( m_flStuckCheckTime, 0, sizeof(m_flStuckCheckTime) );
}


//-----------------------------------------------------------------------------
// Purpose: Destructor
//-----------------------------------------------------------------------------
CGameMovement::~CGameMovement( void )
{
}

//-----------------------------------------------------------------------------
// Purpose: Allow bots etc to use slightly different solid masks
//-----------------------------------------------------------------------------
unsigned int CGameMovement::PlayerSolidMask( bool brushOnly )
{
	return ( brushOnly ) ? MASK_PLAYERSOLID_BRUSHONLY : MASK_PLAYERSOLID;
}

float CGameMovement::GetTickInterval()
{
	return 0.015f;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
// Output : int
//-----------------------------------------------------------------------------
static int GetCheckInterval( const GameVars* vars, Strafe::IntervalType_t type )
{
	int tickInterval = 1;
	switch ( type )
	{
	default:
		tickInterval = 1;
		break;
	case IntervalType_t::GROUND:
		tickInterval = CATEGORIZE_GROUND_SURFACE_INTERVAL / vars->frametime;
		break;
	case IntervalType_t::STUCK:
		// If we are in the process of being "stuck", then try a new position every command tick until m_StuckLast gets reset back down to zero
		if(false) //if ( player->m_StuckLast != 0 )
		{
			tickInterval = 1;
		}
		else
		{
			if ( true ) //  gpGlobals->maxClients == 1 )
			{
				tickInterval = CHECK_STUCK_INTERVAL_SP / vars->frametime;
			}
			else
			{
				tickInterval = CHECK_STUCK_INTERVAL / vars->frametime;
			}
		}
		break;
	case IntervalType_t::LADDER:
		tickInterval = CHECK_LADDER_INTERVAL / vars->frametime;
		break;
	}
	return tickInterval;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : type - 
// Output : Returns true on success, false on failure.
//-----------------------------------------------------------------------------
bool Strafe::CheckInterval(const GameVars* vars, const CBasePlayer* player, IntervalType_t type )
{
	int tickInterval = GetCheckInterval( vars, type );

	if ( g_bMovementOptimizations )
	{
		return (player->CurrentCommandNumber() + player->entindex()) % tickInterval == 0;
	}
	else
	{
		return true;
	}
}

Vector Strafe::GetPlayerMins( const CBasePlayer* player )
{
	if ( player->IsObserver() )
	{
		return VEC_OBS_HULL_MIN;	
	}
	else
	{
		return player->m_Local.m_bDucked  ? VEC_DUCK_HULL_MIN : VEC_HULL_MIN;
	}
}

Vector Strafe::GetPlayerMins( bool ducked )
{
	return ducked  ? VEC_DUCK_HULL_MIN : VEC_HULL_MIN;
}

Vector Strafe::GetPlayerMaxs( const CBasePlayer* player )
{	
	if ( player->IsObserver() )
	{
		return VEC_OBS_HULL_MAX;	
	}
	else
	{
		return player->m_Local.m_bDucked  ? VEC_DUCK_HULL_MAX : VEC_HULL_MAX;
	}
}

Vector Strafe::GetPlayerMaxs( bool ducked )
{
	return ducked  ? VEC_DUCK_HULL_MAX : VEC_HULL_MAX;
}

float Strafe::GetSurfaceFrictionDefault(trace_t& pm)
{
	return 1.0f;
}

trace_t Strafe::TracePlayerDefault(const Ray_t& ray, const CBasePlayer& player, unsigned int fMask)
{
	trace_t pm;
	pm.endpos = ray.end;
	pm.m_pEnt = player.m_GroundEntity;
	pm.fraction = 1.0f;
	return pm;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : ducked - 
// Output : const Vector
//-----------------------------------------------------------------------------
Vector CGameMovement::GetPlayerViewOffset( bool ducked ) const
{
	return ducked ? VEC_DUCK_VIEW : VEC_VIEW;
}
//-----------------------------------------------------------------------------
// Traces player movement + position
//-----------------------------------------------------------------------------
void CGameMovement::TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm )
{
	//VPROF( "CGameMovement::TracePlayerBBox" );

	Ray_t ray;
	ray.Init( start, end, GetPlayerMins(player), GetPlayerMaxs(player) );
	pm = m_gameVars.tracePlayerFunc(ray, *player, fMask);
}


CBaseEntity CGameMovement::TestPlayerPosition( const Vector& pos, int collisionGroup, trace_t& pm )
{
	Ray_t ray;
	ray.Init( pos, pos, GetPlayerMins(player), GetPlayerMaxs(player) );

	pm = m_gameVars.tracePlayerFunc(ray, *player, PlayerSolidMask());
	if ( (pm.contents & PlayerSolidMask()) && pm.m_pEnt.m_bValid )
	{
		return pm.m_pEnt;
	}
	else
	{	
		return CBaseEntity();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void Strafe::CategorizeGroundSurface( CBasePlayer* player, const GameVars* vars, trace_t &pm )
{
	// Library user implements
	player->m_surfaceFriction = vars->surfaceFrictionFunc(pm);
}

bool CGameMovement::IsDead( void ) const
{
	return false;
}

//-----------------------------------------------------------------------------
// Figures out how the constraint should slow us down
//-----------------------------------------------------------------------------
float CGameMovement::ComputeConstraintSpeedFactor( void )
{
	return 1.0f;

#if 0
	// If we have a constraint, slow down because of that too.
	if ( !mv || mv->m_flConstraintRadius == 0.0f )
		return 1.0f;

	float flDistSq = player->GetAbsOrigin().DistToSqr( mv->m_vecConstraintCenter );

	float flOuterRadiusSq = mv->m_flConstraintRadius * mv->m_flConstraintRadius;
	float flInnerRadiusSq = mv->m_flConstraintRadius - mv->m_flConstraintWidth;
	flInnerRadiusSq *= flInnerRadiusSq;

	// Only slow us down if we're inside the constraint ring
	if ((flDistSq <= flInnerRadiusSq) || (flDistSq >= flOuterRadiusSq))
		return 1.0f;

	// Only slow us down if we're running away from the center
	Vector vecDesired;
	VectorMultiply( m_vecForward, mv->m_flForwardMove, vecDesired );
	VectorMA( vecDesired, mv->m_flSideMove, m_vecRight, vecDesired );
	VectorMA( vecDesired, mv->m_flUpMove, m_vecUp, vecDesired );

	Vector vecDelta;
	VectorSubtract( player->GetAbsOrigin(), mv->m_vecConstraintCenter, vecDelta );
	VectorNormalize( vecDelta );
	VectorNormalize( vecDesired );
	if (DotProduct( vecDelta, vecDesired ) < 0.0f)
		return 1.0f;

	float flFrac = (sqrt(flDistSq) - (mv->m_flConstraintRadius - mv->m_flConstraintWidth)) / mv->m_flConstraintWidth;

	float flSpeedFactor = Lerp( flFrac, 1.0f, mv->m_flConstraintSpeedFactor ); 
	return flSpeedFactor;
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::CheckParameters( void )
{
	QAngle	v_angle;

	if ( player->GetMoveType() != MOVETYPE_ISOMETRIC &&
		 player->GetMoveType() != MOVETYPE_NOCLIP &&
		 player->GetMoveType() != MOVETYPE_OBSERVER )
	{
		float spd;
		float maxspeed;

		spd = ( mv->m_flForwardMove * mv->m_flForwardMove ) +
			  ( mv->m_flSideMove * mv->m_flSideMove ) +
			  ( mv->m_flUpMove * mv->m_flUpMove );

		maxspeed = player->m_flClientMaxSpeed;
		if ( maxspeed != 0.0 )
		{
			m_flMaxSpeed = std::min( maxspeed, m_flMaxSpeed );
		}

		// Slow down by the speed factor
		float flSpeedFactor = 1.0f;
		if (player->m_pSurfaceData.hasSurfaceData)
		{
			flSpeedFactor = player->m_pSurfaceData.maxSpeedFactor;
		}

		// If we have a constraint, slow down because of that too.
		float flConstraintSpeedFactor = ComputeConstraintSpeedFactor();
		if (flConstraintSpeedFactor < flSpeedFactor)
			flSpeedFactor = flConstraintSpeedFactor;

		m_flMaxSpeed *= flSpeedFactor;

		if ( g_bMovementOptimizations )
		{
			// Same thing but only do the sqrt if we have to.
			if ( ( spd != 0.0 ) && ( spd > m_flMaxSpeed*m_flMaxSpeed ) )
			{
				float fRatio = m_flMaxSpeed / sqrt( spd );
				mv->m_flForwardMove *= fRatio;
				mv->m_flSideMove    *= fRatio;
				mv->m_flUpMove      *= fRatio;
			}
		}
		else
		{
			spd = sqrt( spd );
			if ( ( spd != 0.0 ) && ( spd > m_flMaxSpeed ) )
			{
				float fRatio = m_flMaxSpeed / spd;
				mv->m_flForwardMove *= fRatio;
				mv->m_flSideMove    *= fRatio;
				mv->m_flUpMove      *= fRatio;
			}
		}
	}

	if ( player->GetFlags() & FL_FROZEN ||
		 player->GetFlags() & FL_ONTRAIN || 
		 IsDead() )
	{
		mv->m_flForwardMove = 0;
		mv->m_flSideMove    = 0;
		mv->m_flUpMove      = 0;
	}
}

void CGameMovement::ReduceTimers( void )
{
	float frame_msec = 1000.0f * m_gameVars.frametime;

	if ( player->m_Local.m_flDucktime > 0 )
	{
		player->m_Local.m_flDucktime -= frame_msec;
		if ( player->m_Local.m_flDucktime < 0 )
		{
			player->m_Local.m_flDucktime = 0;
		}
	}
	if ( player->m_Local.m_flDuckJumpTime > 0 )
	{
		player->m_Local.m_flDuckJumpTime -= frame_msec;
		if ( player->m_Local.m_flDuckJumpTime < 0 )
		{
			player->m_Local.m_flDuckJumpTime = 0;
		}
	}
	if ( player->m_Local.m_flJumpTime > 0 )
	{
		player->m_Local.m_flJumpTime -= frame_msec;
		if ( player->m_Local.m_flJumpTime < 0 )
		{
			player->m_Local.m_flJumpTime = 0;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : *pMove - 
//-----------------------------------------------------------------------------
void CGameMovement::ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMove )
{
	// Cropping movement speed scales mv->m_fForwardSpeed etc. globally
	// Once we crop, we don't want to recursively crop again, so we set the crop
	//  flag globally here once per usercmd cycle.
	m_bSpeedCropped = false;
	
	player = pPlayer;

	mv = pMove;
	m_flMaxSpeed = m_gameVars.sv_maxspeed;

	// Run the command.
	PlayerMove();

	FinishMove();

	player = NULL;
}


//-----------------------------------------------------------------------------
// Purpose: Sets ground entity
//-----------------------------------------------------------------------------
void CGameMovement::FinishMove( void )
{
	player->m_nOldButtons = player->m_nButtons;
}

#define PUNCH_DAMPING		9.0f		// bigger number makes the response more damped, smaller is less damped
										// currently the system will overshoot, with larger damping values it won't
#define PUNCH_SPRING_CONSTANT	65.0f	// bigger number increases the speed at which the view corrects


//-----------------------------------------------------------------------------
// Purpose: Decays the punchangle toward 0,0,0.
//			Modelled as a damped spring
//-----------------------------------------------------------------------------
void CGameMovement::DecayPunchAngle( void )
{
	if ( player->m_Local.m_vecPunchAngle.LengthSqr() > 0.001 || player->m_Local.m_vecPunchAngleVel.LengthSqr() > 0.001 )
	{
		player->m_Local.m_vecPunchAngle += player->m_Local.m_vecPunchAngleVel * m_gameVars.frametime;
		float damping = 1 - (PUNCH_DAMPING * m_gameVars.frametime);
		
		if ( damping < 0 )
		{
			damping = 0;
		}
		player->m_Local.m_vecPunchAngleVel *= damping;
		 
		// torsional spring
		// UNDONE: Per-axis spring constant?
		float springForceMagnitude = PUNCH_SPRING_CONSTANT * m_gameVars.frametime;
		springForceMagnitude = clamp(springForceMagnitude, 0.0f, 2.0f );
		player->m_Local.m_vecPunchAngleVel -= player->m_Local.m_vecPunchAngle * springForceMagnitude;

		// don't wrap around
		player->m_Local.m_vecPunchAngle.Init( 
			clamp(player->m_Local.m_vecPunchAngle.x, -89.0f, 89.0f ), 
			clamp(player->m_Local.m_vecPunchAngle.y, -179.0f, 179.0f ),
			clamp(player->m_Local.m_vecPunchAngle.z, -89.0f, 89.0f ) );
	}
	else
	{
		player->m_Local.m_vecPunchAngle.Init( 0, 0, 0 );
		player->m_Local.m_vecPunchAngleVel.Init( 0, 0, 0 );
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::StartGravity( void )
{
	float ent_gravity;
	
	if (player->GetGravity())
		ent_gravity = player->GetGravity();
	else
		ent_gravity = 1.0;

	// Add gravity so they'll be in the correct position during movement
	// yes, this 0.5 looks wrong, but it's not.  
	player->m_vecVelocity[2] -= (ent_gravity * m_gameVars.sv_gravity * 0.5 * m_gameVars.frametime );
	player->m_vecVelocity[2] += player->GetBaseVelocity()[2] * m_gameVars.frametime;

	Vector temp = player->GetBaseVelocity();
	temp[ 2 ] = 0;
	player->SetBaseVelocity( temp );

	CheckVelocity();
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::CheckWaterJump( void )
{
	Vector	flatforward;
	Vector forward;
	Vector	flatvelocity;
	float curspeed;

	AngleVectors( mv->m_vecViewAngles, &forward );  // Determine movement angles

	// Already water jumping.
	if (player->m_flWaterJumpTime)
		return;

	// Don't hop out if we just jumped in
	if (player->m_vecVelocity[2] < -180)
		return; // only hop out if we are moving up

	// See if we are backing up
	flatvelocity[0] = player->m_vecVelocity[0];
	flatvelocity[1] = player->m_vecVelocity[1];
	flatvelocity[2] = 0;

	// Must be moving
	curspeed = VectorNormalize( flatvelocity );
	
	// see if near an edge
	flatforward[0] = forward[0];
	flatforward[1] = forward[1];
	flatforward[2] = 0;
	VectorNormalize (flatforward);

	// Are we backing into water from steps or something?  If so, don't pop forward
	if ( curspeed != 0.0 && ( DotProduct( flatvelocity, flatforward ) < 0.0 ) )
		return;

	Vector vecStart;
	// Start line trace at waist height (using the center of the player for this here)
	vecStart= player->GetAbsOrigin() + (GetPlayerMins(player) + GetPlayerMaxs(player) ) * 0.5;

	Vector vecEnd;
	VectorMA( vecStart, 24.0f, flatforward, vecEnd );
	
	trace_t tr;
	TracePlayerBBox( vecStart, vecEnd, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, tr );
	if ( tr.fraction < 1.0 )		// solid at waist
	{
		if ( tr.m_pEnt.m_bHeldByPlayer )
			return;

		vecStart.z = player->GetAbsOrigin().z + player->GetViewOffset().z + WATERJUMP_HEIGHT; 
		VectorMA( vecStart, 24.0f, flatforward, vecEnd );
		VectorMA( vec3_origin, -50.0f, tr.plane.normal, player->m_vecWaterJumpVel );

		TracePlayerBBox( vecStart, vecEnd, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, tr );
		if ( tr.fraction == 1.0 )		// open at eye level
		{
			// Now trace down to see if we would actually land on a standable surface.
			VectorCopy( vecEnd, vecStart );
			vecEnd.z -= 1024.0f;
			TracePlayerBBox( vecStart, vecEnd, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, tr );
			if ( ( tr.fraction < 1.0f ) && ( tr.plane.normal.z >= 0.7 ) )
			{
				player->m_vecVelocity[2] = 256.0f;			// Push up
				player->m_nOldButtons |= IN_JUMP;		// Don't jump again until released
				player->AddFlag( FL_WATERJUMP );
				player->m_flWaterJumpTime = 2000.0f;	// Do this for 2 seconds
			}
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::WaterJump( void )
{
	if (player->m_flWaterJumpTime > 10000)
		player->m_flWaterJumpTime = 10000;

	if (!player->m_flWaterJumpTime)
		return;

	player->m_flWaterJumpTime -= 1000.0f * m_gameVars.frametime;

	if (player->m_flWaterJumpTime <= 0 || !player->GetWaterLevel())
	{
		player->m_flWaterJumpTime = 0;
		player->RemoveFlag( FL_WATERJUMP );
	}
	
	player->m_vecVelocity[0] = player->m_vecWaterJumpVel[0];
	player->m_vecVelocity[1] = player->m_vecWaterJumpVel[1];
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::WaterMove( void )
{
	int		i;
	Vector	wishvel;
	float	wishspeed;
	Vector	wishdir;
	Vector	start, dest;
	Vector  temp;
	trace_t	pm;
	float speed, newspeed, addspeed, accelspeed;
	Vector forward, right, up;

	AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles

	//
	// user intentions
	//
	for (i=0 ; i<3 ; i++)
	{
		wishvel[i] = forward[i]*mv->m_flForwardMove + right[i]*mv->m_flSideMove;
	}

	// if we have the jump key down, move us up as well
	if (player->m_nButtons & IN_JUMP)
	{
		wishvel[2] += m_flMaxSpeed;
	}
	// Sinking after no other movement occurs
	else if (!mv->m_flForwardMove && !mv->m_flSideMove && !mv->m_flUpMove)
	{
		wishvel[2] -= 60;		// drift towards bottom
	}
	else  // Go straight up by upmove amount.
	{
		// exaggerate upward movement along forward as well
		float upwardMovememnt = mv->m_flForwardMove * forward.z * 2;
		upwardMovememnt = clamp( upwardMovememnt, 0.0f, m_flMaxSpeed );
		wishvel[2] += mv->m_flUpMove + upwardMovememnt;
	}

	// Copy it over and determine speed
	VectorCopy (wishvel, wishdir);
	wishspeed = VectorNormalize(wishdir);

	// Cap speed.
	if (wishspeed > m_flMaxSpeed)
	{
		VectorScale (wishvel, m_flMaxSpeed/wishspeed, wishvel);
		wishspeed = m_flMaxSpeed;
	}

	// Slow us down a bit.
	wishspeed *= 0.8;
	
	// Water friction
	VectorCopy(player->m_vecVelocity, temp);
	speed = VectorNormalize(temp);
	if (speed)
	{
		newspeed = speed - m_gameVars.frametime * speed * m_gameVars.sv_friction * player->m_surfaceFriction;
		if (newspeed < 0.1f)
		{
			newspeed = 0;
		}

		VectorScale (player->m_vecVelocity, newspeed/speed, player->m_vecVelocity);
	}
	else
	{
		newspeed = 0;
	}

	// water acceleration
	if (wishspeed >= 0.1f)  // old !
	{
		addspeed = wishspeed - newspeed;
		if (addspeed > 0)
		{
			VectorNormalize(wishvel);
			accelspeed = m_gameVars.sv_accelerate * wishspeed * m_gameVars.frametime * player->m_surfaceFriction;
			if (accelspeed > addspeed)
			{
				accelspeed = addspeed;
			}

			for (i = 0; i < 3; i++)
			{
				float deltaSpeed = accelspeed * wishvel[i];
				player->m_vecVelocity[i] += deltaSpeed;
			}
		}
	}

	VecAdd (player->m_vecVelocity, player->GetBaseVelocity(), player->m_vecVelocity);

	// Now move
	// assume it is a stair or a slope, so press down from stepheight above
	VectorMA (player->GetAbsOrigin(), m_gameVars.frametime, player->m_vecVelocity, dest);
	
	TracePlayerBBox( player->GetAbsOrigin(), dest, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );
	if ( pm.fraction == 1.0f )
	{
		VectorCopy( dest, start );
		if ( player->m_Local.m_bAllowAutoMovement )
		{
			start[2] += player->m_Local.m_flStepSize + 1;
		}
		
		TracePlayerBBox( start, dest, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );
		
		if (!pm.startsolid && !pm.allsolid)
		{
			// walked up the step, so just keep result and exit
			player->SetAbsOrigin( pm.endpos );
			VecSubtract( player->m_vecVelocity, player->GetBaseVelocity(), player->m_vecVelocity );
			return;
		}

		// Try moving straight along out normal path.
		TryPlayerMove();
	}
	else
	{
		if ( !player->GetGroundEntity().m_bValid )
		{
			TryPlayerMove();
			VecSubtract( player->m_vecVelocity, player->GetBaseVelocity(), player->m_vecVelocity );
			return;
		}

		StepMove( dest, pm );
	}
	
	VecSubtract( player->m_vecVelocity, player->GetBaseVelocity(), player->m_vecVelocity );
}

//-----------------------------------------------------------------------------
// Purpose: Does the basic move attempting to climb up step heights.  It uses
//          the player->GetAbsOrigin() and player->m_vecVelocity.  It returns a new
//          new player->GetAbsOrigin(), player->m_vecVelocity, and mv->m_outStepHeight.
//-----------------------------------------------------------------------------
void CGameMovement::StepMove( Vector &vecDestination, trace_t &trace )
{
	Vector vecEndPos;
	VectorCopy( vecDestination, vecEndPos );

	// Try sliding forward both on ground and up 16 pixels
	//  take the move that goes farthest
	Vector vecPos, vecVel;
	VectorCopy( player->GetAbsOrigin(), vecPos );
	VectorCopy( player->m_vecVelocity, vecVel );

	// Slide move down.
	TryPlayerMove( &vecEndPos, &trace );
	
	// Down results.
	Vector vecDownPos, vecDownVel;
	VectorCopy( player->GetAbsOrigin(), vecDownPos );
	VectorCopy( player->m_vecVelocity, vecDownVel );
	
	// Reset original values.
	player->SetAbsOrigin( vecPos );
	VectorCopy( vecVel, player->m_vecVelocity );
	
	// Move up a stair height.
	VectorCopy( player->GetAbsOrigin(), vecEndPos );
	if ( player->m_Local.m_bAllowAutoMovement )
	{
		vecEndPos.z += player->m_Local.m_flStepSize + m_gameVars.DIST_EPSILON;
	}
	
	TracePlayerBBox( player->GetAbsOrigin(), vecEndPos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );
	if ( !trace.startsolid && !trace.allsolid )
	{
		player->SetAbsOrigin( trace.endpos );
	}
	
	// Slide move up.
	TryPlayerMove();
	
	// Move down a stair (attempt to).
	VectorCopy( player->GetAbsOrigin(), vecEndPos );
	if ( player->m_Local.m_bAllowAutoMovement )
	{
		vecEndPos.z -= player->m_Local.m_flStepSize + m_gameVars.DIST_EPSILON;
	}
		
	TracePlayerBBox( player->GetAbsOrigin(), vecEndPos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );
	
	// If we are not on the ground any more then use the original movement attempt.
	if ( trace.plane.normal[2] < 0.7 )
	{
		player->SetAbsOrigin( vecDownPos );
		VectorCopy( vecDownVel, player->m_vecVelocity );
		return;
	}
	
	// If the trace ended up in empty space, copy the end over to the origin.
	if ( !trace.startsolid && !trace.allsolid )
	{
		player->SetAbsOrigin( trace.endpos );
	}
	
	// Copy this origin to up.
	Vector vecUpPos;
	VectorCopy( player->GetAbsOrigin(), vecUpPos );
	
	// decide which one went farther
	float flDownDist = ( vecDownPos.x - vecPos.x ) * ( vecDownPos.x - vecPos.x ) + ( vecDownPos.y - vecPos.y ) * ( vecDownPos.y - vecPos.y );
	float flUpDist = ( vecUpPos.x - vecPos.x ) * ( vecUpPos.x - vecPos.x ) + ( vecUpPos.y - vecPos.y ) * ( vecUpPos.y - vecPos.y );
	if ( flDownDist > flUpDist )
	{
		player->SetAbsOrigin( vecDownPos );
		VectorCopy( vecDownVel, player->m_vecVelocity );
	}
	else 
	{
		// copy z value from slide move
		player->m_vecVelocity.z = vecDownVel.z;
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::Friction( void )
{
	float	speed, newspeed, control;
	float	friction;
	float	drop;
	
	// If we are in water jump cycle, don't apply friction
	if (player->m_flWaterJumpTime)
		return;

	// Calculate speed
	speed = VectorLength( player->m_vecVelocity );
	
	// If too slow, return
	if (speed < 0.1f)
	{
		return;
	}

	drop = 0;

	// apply ground friction
	if (player->GetGroundEntity().m_bValid != false)  // On an entity that is the ground
	{
		friction = m_gameVars.sv_friction * player->m_surfaceFriction;

		// Bleed off some speed, but if we have less than the bleed
		//  threshold, bleed the threshold amount.

		control = (speed < m_gameVars.sv_stopspeed) ? m_gameVars.sv_stopspeed : speed;

		// Add the amount to the drop amount.
		drop += control*friction* m_gameVars.frametime;
	}

	// scale the velocity
	newspeed = speed - drop;
	if (newspeed < 0)
		newspeed = 0;

	if ( newspeed != speed )
	{
		// Determine proportion of old speed we are using.
		newspeed /= speed;
		// Adjust velocity according to proportion.
		VectorScale( player->m_vecVelocity, newspeed, player->m_vecVelocity );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::FinishGravity( void )
{
	float ent_gravity;

	if ( player->m_flWaterJumpTime )
		return;

	if ( player->GetGravity() )
		ent_gravity = player->GetGravity();
	else
		ent_gravity = 1.0;

	// Get the correct velocity for the end of the dt 
  	player->m_vecVelocity[2] -= (ent_gravity * m_gameVars.sv_gravity * m_gameVars.frametime * 0.5);

	CheckVelocity();
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : wishdir - 
//			accel - 
//-----------------------------------------------------------------------------
void CGameMovement::AirAccelerate( Vector& wishdir, float wishspeed, float accel )
{
	int i;
	float addspeed, accelspeed, currentspeed;
	float wishspd;

	wishspd = wishspeed;
	
#if 0
	if (player->pl.deadflag)
		return;
#endif
	
	if (player->m_flWaterJumpTime)
		return;

	// Cap speed
	if (wishspd > 30)
		wishspd = 30;

	// Determine veer amount
	currentspeed = player->m_vecVelocity.Dot(wishdir);

	// See how much to add
	addspeed = wishspd - currentspeed;

	// If not adding any, done.
	if (addspeed <= 0)
		return;

	// Determine acceleration speed after acceleration
	accelspeed = accel * wishspeed * m_gameVars.frametime * player->m_surfaceFriction;

	// Cap it
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	// Adjust pmove vel.
	for (i=0 ; i<3 ; i++)
	{
		player->m_vecVelocity[i] += accelspeed * wishdir[i];
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::AirMove( void )
{
	int			i;
	Vector		wishvel;
	float		fmove, smove;
	Vector		wishdir;
	float		wishspeed;
	Vector forward, right, up;

	AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles
	
	// Copy movement amounts
	fmove = mv->m_flForwardMove;
	smove = mv->m_flSideMove;
	
	// Zero out z components of movement vectors
	forward[2] = 0;
	right[2]   = 0;
	VectorNormalize(forward);  // Normalize remainder of vectors
	VectorNormalize(right);    // 

	for (i=0 ; i<2 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	wishvel[2] = 0;             // Zero out z part of velocity

	VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
	wishspeed = VectorNormalize(wishdir);

	//
	// clamp to server defined max speed
	//
	if ( wishspeed != 0 && (wishspeed > m_flMaxSpeed))
	{
		VectorScale (wishvel, m_flMaxSpeed/wishspeed, wishvel);
		wishspeed = m_flMaxSpeed;
	}
	
	AirAccelerate( wishdir, wishspeed, m_gameVars.sv_airaccelerate );

	// Add in any base velocity to the current velocity.
	VecAdd(player->m_vecVelocity, player->GetBaseVelocity(), player->m_vecVelocity );

	TryPlayerMove();

	// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
	VecSubtract( player->m_vecVelocity, player->GetBaseVelocity(), player->m_vecVelocity );
}


bool CGameMovement::CanAccelerate()
{
#if 0
	// Dead players don't accelerate.
	if (player->pl.deadflag)
		return false;
#endif

	// If waterjumping, don't accelerate
	if (player->m_flWaterJumpTime)
		return false;

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : wishdir - 
//			wishspeed - 
//			accel - 
//-----------------------------------------------------------------------------
void CGameMovement::Accelerate( Vector& wishdir, float wishspeed, float accel )
{
	int i;
	float addspeed, accelspeed, currentspeed;

	// This gets overridden because some games (CSPort) want to allow dead (observer) players
	// to be able to move around.
	if ( !CanAccelerate() )
		return;

	// See if we are changing direction a bit
	currentspeed = player->m_vecVelocity.Dot(wishdir);

	// Reduce wishspeed by the amount of veer.
	addspeed = wishspeed - currentspeed;

	// If not going to add any speed, done.
	if (addspeed <= 0)
		return;

	// Determine amount of accleration.
	accelspeed = accel * m_gameVars.frametime * wishspeed * player->m_surfaceFriction;

	// Cap at addspeed
	if (accelspeed > addspeed)
		accelspeed = addspeed;
	
	// Adjust velocity.
	for (i=0 ; i<3 ; i++)
	{
		player->m_vecVelocity[i] += accelspeed * wishdir[i];	
	}
}

//-----------------------------------------------------------------------------
// Purpose: Try to keep a walking player on the ground when running down slopes etc
//-----------------------------------------------------------------------------
void CGameMovement::StayOnGround( void )
{
	trace_t trace;
	Vector start( player->GetAbsOrigin() );
	Vector end( player->GetAbsOrigin() );
	start.z += 2;
	end.z -= player->GetStepSize();

	// See how far up we can go without getting stuck

	TracePlayerBBox( player->GetAbsOrigin(), start, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );
	start = trace.endpos;

	// using trace.startsolid is unreliable here, it doesn't get set when
	// tracing bounding box vs. terrain

	// Now trace down from a known safe position
	TracePlayerBBox( start, end, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );
	if ( trace.fraction > 0.0f &&			// must go somewhere
		trace.fraction < 1.0f &&			// must hit something
		!trace.startsolid &&				// can't be embedded in a solid
		trace.plane.normal[2] >= 0.7 )		// can't hit a steep slope that we can't stand on anyway
	{
		float flDelta = fabs(player->GetAbsOrigin().z - trace.endpos.z);

		//This is incredibly hacky. The real problem is that trace returning that strange value we can't network over.
		if ( flDelta > 0.5f * COORD_RESOLUTION)
		{
			player->SetAbsOrigin( trace.endpos );
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::WalkMove( void )
{
	int i;

	Vector wishvel;
	float spd;
	float fmove, smove;
	Vector wishdir;
	float wishspeed;

	Vector dest;
	trace_t pm;
	Vector forward, right, up;

	AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles

	bool oldground = player->GetGroundEntity().m_bValid;
	
	// Copy movement amounts
	fmove = mv->m_flForwardMove;
	smove = mv->m_flSideMove;

	// Zero out z components of movement vectors
	if ( g_bMovementOptimizations )
	{
		if ( forward[2] != 0 )
		{
			forward[2] = 0;
			VectorNormalize( forward );
		}

		if ( right[2] != 0 )
		{
			right[2] = 0;
			VectorNormalize( right );
		}
	}
	else
	{
		forward[2] = 0;
		right[2]   = 0;
		
		VectorNormalize (forward);  // Normalize remainder of vectors.
		VectorNormalize (right);    // 
	}

	for (i=0 ; i<2 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	
	wishvel[2] = 0;             // Zero out z part of velocity

	VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
	wishspeed = VectorNormalize(wishdir);

	//
	// Clamp to server defined max speed
	//
	if ((wishspeed != 0.0f) && (wishspeed > m_flMaxSpeed))
	{
		VectorScale (wishvel, m_flMaxSpeed/wishspeed, wishvel);
		wishspeed = m_flMaxSpeed;
	}

	// Set pmove velocity
	player->m_vecVelocity[2] = 0;
	Accelerate ( wishdir, wishspeed, m_gameVars.sv_accelerate );
	player->m_vecVelocity[2] = 0;

	// Add in any base velocity to the current velocity.
	VecAdd (player->m_vecVelocity, player->GetBaseVelocity(), player->m_vecVelocity );

	spd = VectorLength( player->m_vecVelocity );

	if ( spd < 1.0f )
	{
		player->m_vecVelocity.Init();
		// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
		VecSubtract( player->m_vecVelocity, player->GetBaseVelocity(), player->m_vecVelocity );
		return;
	}

	// first try just moving to the destination	
	dest[0] = player->GetAbsOrigin()[0] + player->m_vecVelocity[0]* m_gameVars.frametime;
	dest[1] = player->GetAbsOrigin()[1] + player->m_vecVelocity[1]* m_gameVars.frametime;	
	dest[2] = player->GetAbsOrigin()[2];

	// first try moving directly to the next spot
	TracePlayerBBox( player->GetAbsOrigin(), dest, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );

	if ( pm.fraction == 1 )
	{
		player->SetAbsOrigin( pm.endpos );
		// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
		VecSubtract( player->m_vecVelocity, player->GetBaseVelocity(), player->m_vecVelocity );

		StayOnGround();
		return;
	}

	// Don't walk up stairs if not on ground.
	if ( oldground == false && player->GetWaterLevel()  == 0 )
	{
		// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
		VecSubtract( player->m_vecVelocity, player->GetBaseVelocity(), player->m_vecVelocity );
		return;
	}

	// If we are jumping out of water, don't do anything more.
	if ( player->m_flWaterJumpTime )         
	{
		// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
		VecSubtract( player->m_vecVelocity, player->GetBaseVelocity(), player->m_vecVelocity );
		return;
	}

	StepMove( dest, pm );

	// Now pull the base velocity back out.   Base velocity is set if you are on a moving object, like a conveyor (or maybe another monster?)
	VecSubtract( player->m_vecVelocity, player->GetBaseVelocity(), player->m_vecVelocity );

	StayOnGround();
}



//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::FullWalkMove( )
{
	if ( !CheckWater(player) ) 
	{
		StartGravity();
	}

	// If we are leaping out of the water, just update the counters.
	if (player->m_flWaterJumpTime)
	{
		WaterJump();
		TryPlayerMove();
		// See if we are still in water?
		CheckWater(player);
		return;
	}

	// If we are swimming in the water, see if we are nudging against a place we can jump up out
	//  of, and, if so, start out jump.  Otherwise, if we are not moving up, then reset jump timer to 0
	if ( player->GetWaterLevel() >= (int)WaterLevel::WL_Waist ) 
	{
		if ( player->GetWaterLevel() == (int)WaterLevel::WL_Waist )
		{
			CheckWaterJump();
		}

			// If we are falling again, then we must not trying to jump out of water any more.
		if ( player->m_vecVelocity[2] < 0 && 
			 player->m_flWaterJumpTime )
		{
			player->m_flWaterJumpTime = 0;
		}

		// Was jump button pressed?
		if (player->m_nButtons & IN_JUMP)
		{
			CheckJumpButton();
		}
		else
		{
			player->m_nOldButtons &= ~IN_JUMP;
		}

		// Perform regular water movement
		WaterMove();

		// Redetermine position vars
		CategorizePosition();

		// If we are on ground, no downward velocity.
		if ( player->GetGroundEntity().m_bValid != false )
		{
			player->m_vecVelocity[2] = 0;			
		}
	}
	else
	// Not fully underwater
	{
		// Was jump button pressed?
		if (player->m_nButtons & IN_JUMP)
		{
 			CheckJumpButton();
		}
		else
		{
			player->m_nOldButtons &= ~IN_JUMP;
		}

		// Fricion is handled before we add in any base velocity. That way, if we are on a conveyor, 
		//  we don't slow when standing still, relative to the conveyor.
		if (player->GetGroundEntity().m_bValid != false)
		{
			player->m_vecVelocity[2] = 0.0;
			Friction();
		}

		// Make sure velocity is valid.
		CheckVelocity();

		if (player->GetGroundEntity().m_bValid != false)
		{
			WalkMove();
		}
		else
		{
			AirMove();  // Take into account movement when in air.
		}

		// Set final flags.
		CategorizePosition();

		// Make sure velocity is valid.
		CheckVelocity();

		// Add any remaining gravitational component.
		if ( !CheckWater(player) )
		{
			FinishGravity();
		}

		// If we are on ground, no downward velocity.
		if ( player->GetGroundEntity().m_bValid != false )
		{
			player->m_vecVelocity[2] = 0;
		}
		CheckFalling();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::FullObserverMove( void )
{
#if 0
	int mode = player->GetObserverMode();

	if ( mode == OBS_MODE_IN_EYE || mode == OBS_MODE_CHASE )
	{
		CBaseEntity * target = player->GetObserverTarget();

		if ( target != NULL )
		{
			player->SetAbsOrigin( target->GetAbsOrigin() );
			mv->m_vecViewAngles = target->GetAbsAngles();
			player->m_vecVelocity = target->GetAbsVelocity();
		}

		return;
	}

	if ( mode != OBS_MODE_ROAMING )
	{
		// don't move in fixed or death cam mode
		return;
	}

	if ( sv_specnoclip.GetBool() )
	{
		// roam in noclip mode
		FullNoClipMove( m_gameVars.sv_specspeed, m_gameVars.sv_specaccelerate );
		return;
	}

	// do a full clipped free roam move:

	Vector wishvel;
	Vector forward, right, up;
	Vector wishdir, wishend;
	float wishspeed;

	AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles
	
	// Copy movement amounts

	float factor = m_gameVars.sv_specspeed;

	if ( player->m_nButtons & IN_SPEED )
	{
		factor /= 2.0f;
	}

	float fmove = mv->m_flForwardMove * factor;
	float smove = mv->m_flSideMove * factor;
	
	VectorNormalize (forward);  // Normalize remainder of vectors
	VectorNormalize (right);    // 

	for (int i=0 ; i<3 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	wishvel[2] += mv->m_flUpMove;

	VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
	wishspeed = VectorNormalize(wishdir);

	//
	// Clamp to server defined max speed
	//

	float maxspeed = m_gameVars.sv_maxvelocity; 


	if (wishspeed > maxspeed )
	{
		VectorScale (wishvel, m_flMaxSpeed/wishspeed, wishvel);
		wishspeed = maxspeed;
	}

	// Set pmove velocity, give observer 50% acceration bonus
	Accelerate ( wishdir, wishspeed, m_gameVars.sv_specaccelerate );

	float spd = VectorLength( player->m_vecVelocity );
	if (spd < 1.0f)
	{
		player->m_vecVelocity.Init();
		return;
	}
		
	float friction = m_gameVars.sv_friction;
					
	// Add the amount to the drop amount.
	float drop = spd * friction * m_gameVars.frametime;

			// scale the velocity
	float newspeed = spd - drop;

	if (newspeed < 0)
		newspeed = 0;

	// Determine proportion of old speed we are using.
	newspeed /= spd;

	VectorScale( player->m_vecVelocity, newspeed, player->m_vecVelocity );

	CheckVelocity();

	TryPlayerMove();
#endif
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::FullNoClipMove( float factor, float maxacceleration )
{
	Vector wishvel;
	Vector forward, right, up;
	Vector wishdir;
	float wishspeed;
	float maxspeed =  m_gameVars.sv_maxspeed * factor;

	AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles

	if ( player->m_nButtons & IN_SPEED )
	{
		factor /= 2.0f;
	}
	
	// Copy movement amounts
	float fmove = mv->m_flForwardMove * factor;
	float smove = mv->m_flSideMove * factor;
	
	VectorNormalize (forward);  // Normalize remainder of vectors
	VectorNormalize (right);    // 

	for (int i=0 ; i<3 ; i++)       // Determine x and y parts of velocity
		wishvel[i] = forward[i]*fmove + right[i]*smove;
	wishvel[2] += mv->m_flUpMove * factor;

	VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
	wishspeed = VectorNormalize(wishdir);

	//
	// Clamp to server defined max speed
	//
	if (wishspeed > maxspeed )
	{
		VectorScale (wishvel, maxspeed/wishspeed, wishvel);
		wishspeed = maxspeed;
	}

	if ( maxacceleration > 0.0 )
	{
		// Set pmove velocity
		Accelerate ( wishdir, wishspeed, maxacceleration );

		float spd = VectorLength( player->m_vecVelocity );
		if (spd < 1.0f)
		{
			player->m_vecVelocity.Init();
			return;
		}
		
		// Bleed off some speed, but if we have less than the bleed
		//  threshhold, bleed the theshold amount.
		float control = (spd < maxspeed/4.0) ? maxspeed/4.0 : spd;
		
		float friction = m_gameVars.sv_friction * player->m_surfaceFriction;
				
		// Add the amount to the drop amount.
		float drop = control * friction * m_gameVars.frametime;

		// scale the velocity
		float newspeed = spd - drop;
		if (newspeed < 0)
			newspeed = 0;

		// Determine proportion of old speed we are using.
		newspeed /= spd;
		VectorScale( player->m_vecVelocity, newspeed, player->m_vecVelocity );
	}
	else
	{
		VectorCopy( wishvel, player->m_vecVelocity );
	}

	// Just move ( don't clip or anything )
	Vector out;
	VectorMA( player->GetAbsOrigin(), m_gameVars.frametime, player->m_vecVelocity, out );
	player->SetAbsOrigin( out );

	// Zero out velocity if in noaccel mode
	if ( maxacceleration < 0.0f )
	{
		player->m_vecVelocity.Init();
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CGameMovement::CheckJumpButton( void )
{
#if 0
	if (player->pl.deadflag)
	{
		player->m_nOldButtons |= IN_JUMP ;	// don't jump again until released
		return false;
	}
#endif

	// See if we are waterjumping.  If so, decrement count and return.
	if (player->m_flWaterJumpTime)
	{
		player->m_flWaterJumpTime -= m_gameVars.frametime;
		if (player->m_flWaterJumpTime < 0)
			player->m_flWaterJumpTime = 0;
		
		return false;
	}

	// If we are in the water most of the way...
	if ( player->GetWaterLevel() >= 2 )
	{	
		// swimming, not jumping
		SetGroundEntity( NULL );

		if(player->GetWaterType() == CONTENTS_WATER)    // We move up a certain amount
			player->m_vecVelocity[2] = 100;
		else if (player->GetWaterType() == CONTENTS_SLIME)
			player->m_vecVelocity[2] = 80;
		

		return false;
	}

	// No more effect
 	if (player->GetGroundEntity().m_bValid == false)
	{
		player->m_nOldButtons |= IN_JUMP;
		return false;		// in air, so no effect
	}

	// Don't allow jumping when the player is in a stasis field.
#if 0
	if ( player->m_Local.m_bSlowMovement )
		return false;
#endif

	if ( player->m_nOldButtons & IN_JUMP )
		return false;		// don't pogo stick

	// Cannot jump will in the unduck transition.
	if ( player->m_Local.m_bDucking && (  player->GetFlags() & FL_DUCKING ) )
		return false;

	// Still updating the eye position.
	if ( player->m_Local.m_flDuckJumpTime > 0.0f )
		return false;


	// In the air now.
    SetGroundEntity( NULL );

	float flGroundFactor = 1.0f;
	if (player->m_pSurfaceData.hasSurfaceData)
	{
		flGroundFactor = player->m_pSurfaceData.jumpFactor; 
	}

	float flMul;
	if ( g_bMovementOptimizations )
	{
		flMul = 160.0f;	// approx. 21 units.
		// flMul = 268.3281572999747f; // Used not in HL2
	}
	else
	{
		flMul = sqrt(2 * m_gameVars.sv_gravity * GAMEMOVEMENT_JUMP_HEIGHT);
	}

	// Acclerate upward
	// If we are ducking...
	if ( (  player->m_Local.m_bDucking ) || (  player->GetFlags() & FL_DUCKING ) )
	{
		// d = 0.5 * g * t^2		- distance traveled with linear accel
		// t = sqrt(2.0 * 45 / g)	- how long to fall 45 units
		// v = g * t				- velocity at the end (just invert it to jump up that high)
		// v = g * sqrt(2.0 * 45 / g )
		// v^2 = g * g * 2.0 * 45 / g
		// v = sqrt( g * 2.0 * 45 )
		player->m_vecVelocity[2] = flGroundFactor * flMul;  // 2 * gravity * height
	}
	else
	{
		player->m_vecVelocity[2] += flGroundFactor * flMul;  // 2 * gravity * height
	}

	// Add a little forward velocity based on your current forward velocity - if you are not sprinting.
#if defined( HL2_DLL ) || defined( HL2_CLIENT_DLL )
	if ( gpGlobals->maxClients == 1 )
	{
		CHLMoveData *pMoveData = ( CHLMoveData* )mv;
		Vector vecForward;
		AngleVectors( mv->m_vecViewAngles, &vecForward );
		vecForward.z = 0;
		VectorNormalize( vecForward );
		
		// We give a certain percentage of the current forward movement as a bonus to the jump speed.  That bonus is clipped
		// to not accumulate over time.
		float flSpeedBoostPerc = ( !pMoveData->m_bIsSprinting && !player->m_Local.m_bDucked ) ? 0.5f : 0.1f;
		float flSpeedAddition = fabs( mv->m_flForwardMove * flSpeedBoostPerc );
		float flMaxSpeed = m_flMaxSpeed + ( m_flMaxSpeed * flSpeedBoostPerc );
		float flNewSpeed = ( flSpeedAddition + player->m_vecVelocity.Length2D() );

		// If we're over the maximum, we want to only boost as much as will get us to the goal speed
		if ( flNewSpeed > flMaxSpeed )
		{
			flSpeedAddition -= flNewSpeed - flMaxSpeed;
		}

		if ( mv->m_flForwardMove < 0.0f )
			flSpeedAddition *= -1.0f;

		// Add it on
		VectorAdd( (vecForward*flSpeedAddition), player->m_vecVelocity, player->m_vecVelocity );
	}
#endif

	FinishGravity();

	CheckV( player->CurrentCommandNumber(), "CheckJump", player->m_vecVelocity );

	// Set jump time.
	if(true) // if ( gpGlobals->maxClients == 1 )
	{
		player->m_Local.m_flJumpTime = GAMEMOVEMENT_JUMP_TIME;
		player->m_Local.m_bInDuckJump = true;
	}

#if defined( HL2_DLL )

	if ( xc_uncrouch_on_jump.GetBool() )
	{
		// Uncrouch when jumping
		if ( player->GetToggledDuckState() )
		{
			player->ToggleDuck();
		}
	}

#endif

	// Flag that we jumped.
	player->m_nOldButtons |= IN_JUMP;	// don't jump again until released
	return true;
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::FullLadderMove()
{
	CheckWater(player);

	// Was jump button pressed? If so, set velocity to 270 away from ladder.  
	if ( player->m_nButtons & IN_JUMP )
	{
		CheckJumpButton();
	}
	else
	{
		player->m_nOldButtons &= ~IN_JUMP;
	}
	
	// Perform the move accounting for any base velocity.
	VecAdd (player->m_vecVelocity, player->GetBaseVelocity(), player->m_vecVelocity);
	TryPlayerMove();
	VecSubtract (player->m_vecVelocity, player->GetBaseVelocity(), player->m_vecVelocity);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : int
//-----------------------------------------------------------------------------
int CGameMovement::TryPlayerMove( Vector *pFirstDest, trace_t *pFirstTrace )
{
	int			bumpcount, numbumps;
	Vector		dir;
	float		d;
	int			numplanes;
	Vector		planes[MAX_CLIP_PLANES];
	Vector		primal_velocity, original_velocity;
	Vector      new_velocity;
	int			i, j;
	trace_t	pm;
	Vector		end;
	float		time_left, allFraction;
	int			blocked;		
	
	numbumps  = 4;           // Bump up to four times
	
	blocked   = 0;           // Assume not blocked
	numplanes = 0;           //  and not sliding along any planes

	VectorCopy (player->m_vecVelocity, original_velocity);  // Store original velocity
	VectorCopy (player->m_vecVelocity, primal_velocity);
	
	allFraction = 0;
	time_left = m_gameVars.frametime;   // Total time for this movement operation.

	new_velocity.Init();

	for (bumpcount=0 ; bumpcount < numbumps; bumpcount++)
	{
		if ( player->m_vecVelocity.Length() == 0.0 )
			break;

		// Assume we can move all the way from the current origin to the
		//  end point.
		VectorMA( player->GetAbsOrigin(), time_left, player->m_vecVelocity, end );

		// See if we can make it from origin to end point.
		if ( g_bMovementOptimizations )
		{
			// If their velocity Z is 0, then we can avoid an extra trace here during WalkMove.
			if ( pFirstDest && end == *pFirstDest )
				pm = *pFirstTrace;
			else
			{
#if defined( PLAYER_GETTING_STUCK_TESTING )
				trace_t foo;
				TracePlayerBBox( player->GetAbsOrigin(), player->GetAbsOrigin(), PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, foo );
				if ( foo.startsolid || foo.fraction != 1.0f )
				{
					Msg( "bah\n" );
				}
#endif
				TracePlayerBBox( player->GetAbsOrigin(), end, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );
			}
		}
		else
		{
			TracePlayerBBox( player->GetAbsOrigin(), end, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );
		}

		allFraction += pm.fraction;

		// If we started in a solid object, or we were in solid space
		//  the whole way, zero out our velocity and return that we
		//  are blocked by floor and wall.
		if (pm.allsolid)
		{	
			// entity is trapped in another solid
			VectorCopy (vec3_origin, player->m_vecVelocity);
			return 4;
		}

		// If we moved some portion of the total distance, then
		//  copy the end position into the pmove.origin and 
		//  zero the plane counter.
		if( pm.fraction > 0 )
		{	
			if ( numbumps > 0 && pm.fraction == 1 )
			{
				// There's a precision issue with terrain tracing that can cause a swept box to successfully trace
				// when the end position is stuck in the triangle.  Re-run the test with an uswept box to catch that
				// case until the bug is fixed.
				// If we detect getting stuck, don't allow the movement
				trace_t stuck;
				TracePlayerBBox( pm.endpos, pm.endpos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, stuck );
				if ( stuck.startsolid || stuck.fraction != 1.0f )
				{
					//Msg( "Player will become stuck!!!\n" );
					VectorCopy (vec3_origin, player->m_vecVelocity);
					break;
				}
			}

#if defined( PLAYER_GETTING_STUCK_TESTING )
			trace_t foo;
			TracePlayerBBox( pm.endpos, pm.endpos, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, foo );
			if ( foo.startsolid || foo.fraction != 1.0f )
			{
				Msg( "Player will become stuck!!!\n" );
			}
#endif
			// actually covered some distance
			player->SetAbsOrigin( pm.endpos);
			VectorCopy (player->m_vecVelocity, original_velocity);
			numplanes = 0;
		}

		// If we covered the entire distance, we are done
		//  and can return.
		if (pm.fraction == 1)
		{
			 break;		// moved the entire distance
		}

		// Save entity that blocked us (since fraction was < 1.0)
		//  for contact
		// Add it if it's not already in the list!!!
		// MoveHelper( )->AddToTouched( pm, player->m_vecVelocity );

		// If the plane we hit has a high z component in the normal, then
		//  it's probably a floor
		if (pm.plane.normal[2] > 0.7)
		{
			blocked |= 1;		// floor
		}
		// If the plane has a zero z component in the normal, then it's a 
		//  step or wall
		if (!pm.plane.normal[2])
		{
			blocked |= 2;		// step / wall
		}

		// Reduce amount of m_flFrameTime left by total time left * fraction
		//  that we covered.
		time_left -= time_left * pm.fraction;

		// Did we run out of planes to clip against?
		if (numplanes >= MAX_CLIP_PLANES)
		{	
			// this shouldn't really happen
			//  Stop our movement if so.
			VectorCopy (vec3_origin, player->m_vecVelocity);
			//Con_DPrintf("Too many planes 4\n");

			break;
		}

		// Set up next clipping plane
		VectorCopy (pm.plane.normal, planes[numplanes]);
		numplanes++;

		// modify original_velocity so it parallels all of the clip planes
		//

		// reflect player velocity 
		// Only give this a try for first impact plane because you can get yourself stuck in an acute corner by jumping in place
		//  and pressing forward and nobody was really using this bounce/reflection feature anyway...
		if ( numplanes == 1 &&
			player->GetMoveType() == MOVETYPE_WALK &&
			player->GetGroundEntity().m_bValid == false )	
		{
			for ( i = 0; i < numplanes; i++ )
			{
				if ( planes[i][2] > 0.7  )
				{
					// floor or slope
					ClipVelocity( original_velocity, planes[i], new_velocity, 1 );
					VectorCopy( new_velocity, original_velocity );
				}
				else
				{
					ClipVelocity( original_velocity, planes[i], new_velocity, 1.0 + m_gameVars.sv_bounce * (1 - player->m_surfaceFriction) );
				}
			}

			VectorCopy( new_velocity, player->m_vecVelocity );
			VectorCopy( new_velocity, original_velocity );
		}
		else
		{
			for (i=0 ; i < numplanes ; i++)
			{
				ClipVelocity (
					original_velocity,
					planes[i],
					player->m_vecVelocity,
					1);

				for (j=0 ; j<numplanes ; j++)
					if (j != i)
					{
						// Are we now moving against this plane?
						if (player->m_vecVelocity.Dot(planes[j]) < 0)
							break;	// not ok
					}
				if (j == numplanes)  // Didn't have to clip, so we're ok
					break;
			}
			
			// Did we go all the way through plane set
			if (i != numplanes)
			{	// go along this plane
				// pmove.velocity is set in clipping call, no need to set again.
				;  
			}
			else
			{	// go along the crease
				if (numplanes != 2)
				{
					VectorCopy (vec3_origin, player->m_vecVelocity);
					break;
				}
				CrossProduct (planes[0], planes[1], dir);
				dir.VectorNormalize();
				d = dir.Dot(player->m_vecVelocity);
				VectorScale (dir, d, player->m_vecVelocity );
			}

			//
			// if original velocity is against the original velocity, stop dead
			// to avoid tiny occilations in sloping corners
			//
			d = player->m_vecVelocity.Dot(primal_velocity);
			if (d <= 0)
			{
				//Con_DPrintf("Back\n");
				VectorCopy (vec3_origin, player->m_vecVelocity);
				break;
			}
		}
	}

	if ( allFraction == 0 )
	{
		VectorCopy (vec3_origin, player->m_vecVelocity);
	}

	// Check if they slammed into a wall
	float fSlamVol = 0.0f;

	float fLateralStoppingAmount = primal_velocity.Length2D() - player->m_vecVelocity.Length2D();
	if ( fLateralStoppingAmount > PLAYER_MAX_SAFE_FALL_SPEED * 2.0f )
	{
		fSlamVol = 1.0f;
	}
	else if ( fLateralStoppingAmount > PLAYER_MAX_SAFE_FALL_SPEED )
	{
		fSlamVol = 0.85f;
	}

	PlayerRoughLandingEffects( fSlamVol );

	return blocked;
}


//-----------------------------------------------------------------------------
// Purpose: Determine whether or not the player is on a ladder (physprop or world).
//-----------------------------------------------------------------------------
inline bool CGameMovement::OnLadder( trace_t &trace )
{
	if ( trace.contents & CONTENTS_LADDER )
		return true;

#if 0
	IPhysicsSurfaceProps *pPhysProps = MoveHelper( )->GetSurfaceProps();
	if ( pPhysProps )
	{
		const surfacedata_t *pSurfaceData = pPhysProps->GetSurfaceData( trace.surface.surfaceProps );
		if ( pSurfaceData )
		{
			if ( pSurfaceData->game.climbable != 0 )
				return true;
		}
	}
#endif

	return false;
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
bool CGameMovement::LadderMove( void )
{
	trace_t pm;
	bool onFloor;
	Vector floor;
	Vector wishdir;
	Vector end;

	if ( player->GetMoveType() == MOVETYPE_NOCLIP )
		return false;

	if ( !GameHasLadders() )
		return false;

	// If I'm already moving on a ladder, use the previous ladder direction
	if ( player->GetMoveType() == MOVETYPE_LADDER )
	{
		wishdir = -player->m_vecLadderNormal;
	}
	else
	{
		// otherwise, use the direction player is attempting to move
		if ( mv->m_flForwardMove || mv->m_flSideMove )
		{
			for (int i=0 ; i<3 ; i++)       // Determine x and y parts of velocity
				wishdir[i] = m_vecForward[i]*mv->m_flForwardMove + m_vecRight[i]*mv->m_flSideMove;

			VectorNormalize(wishdir);
		}
		else
		{
			// Player is not attempting to move, no ladder behavior
			return false;
		}
	}

	// wishdir points toward the ladder if any exists
	VectorMA( player->GetAbsOrigin(), LadderDistance(), wishdir, end );
	TracePlayerBBox( player->GetAbsOrigin(), end, LadderMask(), COLLISION_GROUP_PLAYER_MOVEMENT, pm );

	// no ladder in that direction, return
	if ( pm.fraction == 1.0f || !OnLadder( pm ) )
		return false;

	player->SetMoveType( MOVETYPE_LADDER );
	player->SetMoveCollide( MOVECOLLIDE_DEFAULT );

	player->m_vecLadderNormal = pm.plane.normal;

	// On ladder, convert movement to be relative to the ladder

	VectorCopy( player->GetAbsOrigin(), floor );
	floor[2] += GetPlayerMins(player)[2] - 1;

	if( GetPointContents( floor ) == CONTENTS_SOLID || player->GetGroundEntity().m_bValid != false )
	{
		onFloor = true;
	}
	else
	{
		onFloor = false;
	}

	player->SetGravity( 0 );

	float climbSpeed = ClimbSpeed();

	float forwardSpeed = 0, rightSpeed = 0;
	if ( player->m_nButtons & IN_BACK )
		forwardSpeed -= climbSpeed;
	
	if ( player->m_nButtons & IN_FORWARD )
		forwardSpeed += climbSpeed;
	
	if ( player->m_nButtons & IN_MOVELEFT )
		rightSpeed -= climbSpeed;
	
	if ( player->m_nButtons & IN_MOVERIGHT )
		rightSpeed += climbSpeed;

	if ( player->m_nButtons & IN_JUMP )
	{
		player->SetMoveType( MOVETYPE_WALK );
		player->SetMoveCollide( MOVECOLLIDE_DEFAULT );

		VectorScale( pm.plane.normal, 270, player->m_vecVelocity );
	}
	else
	{
		if ( forwardSpeed != 0 || rightSpeed != 0 )
		{
			Vector velocity, perp, cross, lateral, tmp;

			//ALERT(at_console, "pev %.2f %.2f %.2f - ",
			//	pev->velocity.x, pev->velocity.y, pev->velocity.z);
			// Calculate player's intended velocity
			//Vector velocity = (forward * gpGlobals->v_forward) + (right * gpGlobals->v_right);
			VectorScale( m_vecForward, forwardSpeed, velocity );
			VectorMA( velocity, rightSpeed, m_vecRight, velocity );

			// Perpendicular in the ladder plane
			VectorCopy( vec3_origin, tmp );
			tmp[2] = 1;
			CrossProduct( tmp, pm.plane.normal, perp );
			VectorNormalize( perp );

			// decompose velocity into ladder plane
			float normal = DotProduct( velocity, pm.plane.normal );

			// This is the velocity into the face of the ladder
			VectorScale( pm.plane.normal, normal, cross );

			// This is the player's additional velocity
			VecSubtract( velocity, cross, lateral );

			// This turns the velocity into the face of the ladder into velocity that
			// is roughly vertically perpendicular to the face of the ladder.
			// NOTE: It IS possible to face up and move down or face down and move up
			// because the velocity is a sum of the directional velocity and the converted
			// velocity through the face of the ladder -- by design.
			CrossProduct( pm.plane.normal, perp, tmp );
			VectorMA( lateral, -normal, tmp, player->m_vecVelocity );

			if ( onFloor && normal > 0 )	// On ground moving away from the ladder
			{
				VectorMA( player->m_vecVelocity, MAX_CLIMB_SPEED, pm.plane.normal, player->m_vecVelocity );
			}
			//pev->velocity = lateral - (CrossProduct( trace.vecPlaneNormal, perp ) * normal);
		}
		else
		{
			player->m_vecVelocity.Init();
		}
	}

	return true;
}


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : axis - 
// Output : const char
//-----------------------------------------------------------------------------
const char *DescribeAxis( int axis )
{
	switch ( axis )
	{
	case 0:
		return "X";
	case 1:
		return "Y";
	case 2:
	default:
		return "Z";
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::CheckVelocity( void )
{
	int i;

	//
	// bound velocity
	//

	Vector org = player->GetAbsOrigin();

	for (i=0; i < 3; i++)
	{
		// See if it's bogus.
		if (IS_NAN(player->m_vecVelocity[i]))
		{
			//DevMsg( 1, "PM  Got a NaN velocity %s\n", DescribeAxis( i ) );
			player->m_vecVelocity[i] = 0;
		}

		if (IS_NAN(org[i]))
		{
			//DevMsg( 1, "PM  Got a NaN origin on %s\n", DescribeAxis( i ) );
			org[ i ] = 0;
			player->SetAbsOrigin( org );
		}

		// Bound it.
		if (player->m_vecVelocity[i] > m_gameVars.sv_maxvelocity) 
		{
			//DevMsg( 1, "PM  Got a velocity too high on %s\n", DescribeAxis( i ) );
			player->m_vecVelocity[i] = m_gameVars.sv_maxvelocity;
		}
		else if (player->m_vecVelocity[i] < -m_gameVars.sv_maxvelocity)
		{
			//DevMsg( 1, "PM  Got a velocity too low on %s\n", DescribeAxis( i ) );
			player->m_vecVelocity[i] = -m_gameVars.sv_maxvelocity;
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::AddGravity( void )
{
	float ent_gravity;

	if ( player->m_flWaterJumpTime )
		return;

	if (player->GetGravity())
		ent_gravity = player->GetGravity();
	else
		ent_gravity = 1.0;

	// Add gravity incorrectly
	player->m_vecVelocity[2] -= (ent_gravity * m_gameVars.sv_gravity * m_gameVars.frametime);
	player->m_vecVelocity[2] += player->GetBaseVelocity()[2] * m_gameVars.frametime;
	Vector temp = player->GetBaseVelocity();
	temp[2] = 0;
	player->SetBaseVelocity( temp );
	
	CheckVelocity();
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : push - 
// Output : trace_t
//-----------------------------------------------------------------------------
void CGameMovement::PushEntity( Vector& push, trace_t *pTrace )
{
	Vector	end;
		
	VecAdd (player->GetAbsOrigin(), push, end);
	TracePlayerBBox( player->GetAbsOrigin(), end, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, *pTrace );
	player->SetAbsOrigin( pTrace->endpos );

	// So we can run impact function afterwards.
	// If
	if ( pTrace->fraction < 1.0 && !pTrace->allsolid )
	{
	//	MoveHelper( )->AddToTouched( *pTrace, player->m_vecVelocity );
	}
}	


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : in - 
//			normal - 
//			out - 
//			overbounce - 
// Output : int
//-----------------------------------------------------------------------------
int CGameMovement::ClipVelocity( Vector& in, Vector& normal, Vector& out, float overbounce )
{
	float	backoff;
	float	change;
	float angle;
	int		i, blocked;
	
	angle = normal[ 2 ];

	blocked = 0x00;         // Assume unblocked.
	if (angle > 0)			// If the plane that is blocking us has a positive z component, then assume it's a floor.
		blocked |= 0x01;	// 
	if (!angle)				// If the plane has no Z, it is vertical (wall/step)
		blocked |= 0x02;	// 
	

	// Determine how far along plane to slide based on incoming direction.
	backoff = DotProduct (in, normal) * overbounce;

	for (i=0 ; i<3 ; i++)
	{
		change = normal[i]*backoff;
		out[i] = in[i] - change; 
	}
	
	// iterate once to make sure we aren't still moving through the plane
	float adjust = DotProduct( out, normal );
	if( adjust < 0.0f )
	{
		out -= ( normal * adjust );
//		Msg( "Adjustment = %lf\n", adjust );
	}

	// Return blocking flags.
	return blocked;
}


//-----------------------------------------------------------------------------
// Purpose: Computes roll angle for a certain movement direction and velocity
// Input  : angles - 
//			velocity - 
//			rollangle - 
//			rollspeed - 
// Output : float 
//-----------------------------------------------------------------------------
float CGameMovement::CalcRoll ( const QAngle &angles, const Vector &velocity, float rollangle, float rollspeed )
{
	float   sign;
	float   side;
	float   value;
	Vector  forward, right, up;
	
	AngleVectors (angles, &forward, &right, &up);
	
	side = DotProduct (velocity, right);
	sign = side < 0 ? -1 : 1;
	side = fabs(side);
	value = rollangle;
	if (side < rollspeed)
	{
		side = side * value / rollspeed;
	}
	else
	{
		side = value;
	}
	return side*sign;
}

#define CHECKSTUCK_MINTIME 0.05  // Don't check again too quickly.

#if !defined(_STATIC_LINKED) || defined(CLIENT_DLL)
Vector rgv3tStuckTable[54];
#else
extern Vector rgv3tStuckTable[54];
#endif

#if !defined(_STATIC_LINKED) || defined(CLIENT_DLL)
//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CreateStuckTable( void )
{
	float x, y, z;
	int idx;
	int i;
	float zi[3];
	static int firsttime = 1;

	if ( !firsttime )
		return;

	firsttime = 0;

	memset(rgv3tStuckTable, 0, sizeof(rgv3tStuckTable));

	idx = 0;
	// Little Moves.
	x = y = 0;
	// Z moves
	for (z = -0.125 ; z <= 0.125 ; z += 0.125)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}
	x = z = 0;
	// Y moves
	for (y = -0.125 ; y <= 0.125 ; y += 0.125)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}
	y = z = 0;
	// X moves
	for (x = -0.125 ; x <= 0.125 ; x += 0.125)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}

	// Remaining multi axis nudges.
	for ( x = - 0.125; x <= 0.125; x += 0.250 )
	{
		for ( y = - 0.125; y <= 0.125; y += 0.250 )
		{
			for ( z = - 0.125; z <= 0.125; z += 0.250 )
			{
				rgv3tStuckTable[idx][0] = x;
				rgv3tStuckTable[idx][1] = y;
				rgv3tStuckTable[idx][2] = z;
				idx++;
			}
		}
	}

	// Big Moves.
	x = y = 0;
	zi[0] = 0.0f;
	zi[1] = 1.0f;
	zi[2] = 6.0f;

	for (i = 0; i < 3; i++)
	{
		// Z moves
		z = zi[i];
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}

	x = z = 0;

	// Y moves
	for (y = -2.0f ; y <= 2.0f ; y += 2.0)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}
	y = z = 0;
	// X moves
	for (x = -2.0f ; x <= 2.0f ; x += 2.0f)
	{
		rgv3tStuckTable[idx][0] = x;
		rgv3tStuckTable[idx][1] = y;
		rgv3tStuckTable[idx][2] = z;
		idx++;
	}

	// Remaining multi axis nudges.
	for (i = 0 ; i < 3; i++)
	{
		z = zi[i];
		
		for (x = -2.0f ; x <= 2.0f ; x += 2.0f)
		{
			for (y = -2.0f ; y <= 2.0f ; y += 2.0)
			{
				rgv3tStuckTable[idx][0] = x;
				rgv3tStuckTable[idx][1] = y;
				rgv3tStuckTable[idx][2] = z;
				idx++;
			}
		}
	}
	//Assert( idx < sizeof(rgv3tStuckTable)/sizeof(rgv3tStuckTable[0]));
}
#else
extern void CreateStuckTable( void );
#endif


//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIndex - 
//			server - 
//			offset - 
// Output : int
//-----------------------------------------------------------------------------
int GetRandomStuckOffsets( CBasePlayer *pPlayer, Vector& offset)
{
 // Last time we did a full
	int idx;
	idx = 0; // pPlayer->m_StuckLast++;

	VecCopy(rgv3tStuckTable[idx % 54], offset);

	return (idx % 54);
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : nIndex - 
//			server - 
//-----------------------------------------------------------------------------
void ResetStuckOffsets( CBasePlayer *pPlayer )
{
	//pPlayer->m_StuckLast = 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &input - 
// Output : int
//-----------------------------------------------------------------------------
int CGameMovement::CheckStuck( void )
{
#if 0
	Vector base;
	Vector offset;
	Vector test;
	EntityHandle_t hitent;
	int idx;
	float fTime;
	trace_t traceresult;

	CreateStuckTable();

	hitent = TestPlayerPosition( player->GetAbsOrigin(), COLLISION_GROUP_PLAYER_MOVEMENT, traceresult );
	if ( hitent == INVALID_ENTITY_HANDLE )
	{
		ResetStuckOffsets( player );
		return 0;
	}

	// Deal with stuckness...
#if 0
	if ( developer.GetBool() )
	{
		bool isServer = player->IsServer();
		engine->Con_NPrintf( isServer, "%s stuck on object %i/%s", 
			isServer ? "server" : "client",
			hitent.GetEntryIndex(), MoveHelper()->GetName(hitent) );
	}
#endif

	VectorCopy( player->GetAbsOrigin(), base );

	// 
	// Deal with precision error in network.
	// 
	// World or BSP model
	if ( !player->IsServer() )
	{
		if ( MoveHelper()->IsWorldEntity( hitent ) )
		{
			int nReps = 0;
			ResetStuckOffsets( player );
			do 
			{
				GetRandomStuckOffsets( player, offset );
				VectorAdd( base, offset, test );
				
				if ( TestPlayerPosition( test, COLLISION_GROUP_PLAYER_MOVEMENT, traceresult ) == INVALID_ENTITY_HANDLE )
				{
					ResetStuckOffsets( player );
					player->SetAbsOrigin( test );
					return 0;
				}
				nReps++;
			} while (nReps < 54);
		}
	}

	// Only an issue on the client.
	idx = player->IsServer() ? 0 : 1;

	fTime = engine->Time();
	// Too soon?
	if ( m_flStuckCheckTime[ player->entindex() ][ idx ] >=  fTime - CHECKSTUCK_MINTIME )
	{
		return 1;
	}
	m_flStuckCheckTime[ player->entindex() ][ idx ] = fTime;

	MoveHelper( )->AddToTouched( traceresult, player->m_vecVelocity );
	GetRandomStuckOffsets( player, offset );
	VectorAdd( base, offset, test );

	if ( TestPlayerPosition( test, COLLISION_GROUP_PLAYER_MOVEMENT, traceresult ) == INVALID_ENTITY_HANDLE)
	{
		ResetStuckOffsets( player );
		player->SetAbsOrigin( test );
		return 0;
	}

#endif
	return 0;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Output : bool
//-----------------------------------------------------------------------------
bool CGameMovement::InWater( void )
{
	return ( player->GetWaterLevel() > (int)WaterLevel::WL_Feet );
}


void CGameMovement::ResetGetPointContentsCache()
{
	for ( int slot = 0; slot < MAX_PC_CACHE_SLOTS; ++slot )
	{
		for ( int i = 0; i < MAX_PLAYERS; ++i )
		{
			m_CachedGetPointContents[ i ][ slot ] = -9999;
		}
	}
}


int CGameMovement::GetPointContentsCached( const Vector &point, int slot )
{
#if 0
	if ( g_bMovementOptimizations ) 
	{
		//Assert( player );
		//Assert( slot >= 0 && slot < MAX_PC_CACHE_SLOTS );

		int idx = player->entindex() - 1;

		if ( m_CachedGetPointContents[ idx ][ slot ] == -9999 || point.DistToSqr( m_CachedGetPointContentsPoint[ idx ][ slot ] ) > 1 )
		{
			m_CachedGetPointContents[ idx ][ slot ] = GetPointContents ( point );
			m_CachedGetPointContentsPoint[ idx ][ slot ] = point;
		}
		
		return m_CachedGetPointContents[ idx ][ slot ];
	}
	else
#else
	{
		return GetPointContents ( point );
	}
#endif
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &input - 
// Output : bool
//-----------------------------------------------------------------------------
bool Strafe::CheckWater( const CBasePlayer* player )
{
#if 0
	Vector	point;
	int		cont;

	// Pick a spot just above the players feet.
	point[0] = player->GetAbsOrigin()[0] + (GetPlayerMins()[0] + GetPlayerMaxs()[0]) * 0.5;
	point[1] = player->GetAbsOrigin()[1] + (GetPlayerMins()[1] + GetPlayerMaxs()[1]) * 0.5;
	point[2] = player->GetAbsOrigin()[2] + GetPlayerMins()[2] + 1;
	
	// Assume that we are not in water at all.
	player->SetWaterLevel( WL_NotInWater );
	player->SetWaterType( CONTENTS_EMPTY );

	// Grab point contents.
	cont = GetPointContentsCached( point, 0 );	
	
	// Are we under water? (not solid and not empty?)
	if ( cont & MASK_WATER )
	{
		// Set water type
		player->SetWaterType( cont );

		// We are at least at level one
		player->SetWaterLevel( WL_Feet );

		// Now check a point that is at the player hull midpoint.
		point[2] = player->GetAbsOrigin()[2] + (GetPlayerMins()[2] + GetPlayerMaxs()[2])*0.5;
		cont = GetPointContentsCached( point, 1 );
		// If that point is also under water...
		if ( cont & MASK_WATER )
		{
			// Set a higher water level.
			player->SetWaterLevel( WL_Waist );

			// Now check the eye position.  (view_ofs is relative to the origin)
			point[2] = player->GetAbsOrigin()[2] + player->GetViewOffset()[2];
			cont = GetPointContentsCached( point, 2 );
			if ( cont & MASK_WATER )
				player->SetWaterLevel( WL_Eyes );  // In over our eyes
		}

		// Adjust velocity based on water current, if any.
		if ( cont & MASK_CURRENT )
		{
			Vector v;
			VectorClear(v);
			if ( cont & CONTENTS_CURRENT_0 )
				v[0] += 1;
			if ( cont & CONTENTS_CURRENT_90 )
				v[1] += 1;
			if ( cont & CONTENTS_CURRENT_180 )
				v[0] -= 1;
			if ( cont & CONTENTS_CURRENT_270 )
				v[1] -= 1;
			if ( cont & CONTENTS_CURRENT_UP )
				v[2] += 1;
			if ( cont & CONTENTS_CURRENT_DOWN )
				v[2] -= 1;

			// BUGBUG -- this depends on the value of an unspecified enumerated type
			// The deeper we are, the stronger the current.
			Vector temp;
			VectorMA( player->GetBaseVelocity(), 50.0*player->GetWaterLevel(), v, temp );
			player->SetBaseVelocity( temp );
		}
	}

	// if we just transitioned from not in water to in water, record the time it happened
	if ( ( WL_NotInWater == m_nOldWaterLevel ) && ( player->GetWaterLevel() >  WL_NotInWater ) )
	{
		m_flWaterEntryTime = gpGlobals->curtime;
	}

	return ( player->GetWaterLevel() > WL_Feet );
#endif
	return ( player->GetWaterLevel() > (int)WaterLevel::WL_Feet );
}

void CGameMovement::SetGroundEntity( trace_t *pm )
{
	CBaseEntity newGround;
	if(pm)
		newGround = pm->m_pEnt;

	CBaseEntity oldGround = player->GetGroundEntity();
	Vector vecBaseVelocity = player->GetBaseVelocity();

	if ( !oldGround.m_bValid && newGround.m_bValid )
	{
		// Subtract ground velocity at instant we hit ground jumping
		vecBaseVelocity -= newGround.GetAbsVelocity(); 
		vecBaseVelocity.z = newGround.GetAbsVelocity().z;
	}
	else if ( oldGround.m_bValid && !newGround.m_bValid )
	{
		// Add in ground velocity at instant we started jumping
 		vecBaseVelocity += oldGround.GetAbsVelocity();
		vecBaseVelocity.z = oldGround.GetAbsVelocity().z;
	}

	player->SetBaseVelocity( vecBaseVelocity );
	player->SetGroundEntity( newGround );

	// If we are on something...
	if ( newGround.m_bValid )
	{
		CategorizeGroundSurface(player, &m_gameVars, *pm);

		// Then we are not in water jump sequence
		player->m_flWaterJumpTime = 0;

		// Standing on an entity other than the world, so signal that we are touching something.
#if 0
		if ( !pm->DidHitWorld() )
		{
			MoveHelper()->AddToTouched( *pm, player->m_vecVelocity );
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Traces the player's collision bounds in quadrants, looking for a plane that
// can be stood upon (normal's z >= 0.7f).  Regardless of success or failure,
// replace the fraction and endpos with the original ones, so we don't try to
// move the player down to the new floor and get stuck on a leaning wall that
// the original trace hit first.
//-----------------------------------------------------------------------------
void CGameMovement::TracePlayerBBoxForGround( const Vector& start, const Vector& end, const Vector& minsSrc,
							  const Vector& maxsSrc, unsigned int fMask,
							  int collisionGroup, trace_t& pm )
{
	//VPROF( "TracePlayerBBoxForGround" );

	Ray_t ray;
	Vector mins, maxs;

	float fraction = pm.fraction;
	Vector endpos = pm.endpos;

	// Check the -x, -y quadrant
	mins = minsSrc;
	maxs.Init( std::min( 0.0f, maxsSrc.x ), std::min( 0.0f, maxsSrc.y ), maxsSrc.z );
	ray.Init( start, end, mins, maxs );
	m_gameVars.tracePlayerFunc(ray, *player, fMask);
	if ( pm.m_pEnt.m_bValid && pm.plane.normal[2] >= 0.7)
	{
		pm.fraction = fraction;
		pm.endpos = endpos;
		return;
	}

	// Check the +x, +y quadrant
	mins.Init( std::max( 0.0f, minsSrc.x ), std::max( 0.0f, minsSrc.y ), minsSrc.z );
	maxs = maxsSrc;
	ray.Init( start, end, mins, maxs );
	m_gameVars.tracePlayerFunc(ray, *player, fMask);
	if ( pm.m_pEnt.m_bValid && pm.plane.normal[2] >= 0.7)
	{
		pm.fraction = fraction;
		pm.endpos = endpos;
		return;
	}

	// Check the -x, +y quadrant
	mins.Init( minsSrc.x, std::max( 0.0f, minsSrc.y ), minsSrc.z );
	maxs.Init( std::min( 0.0f, maxsSrc.x ), maxsSrc.y, maxsSrc.z );
	ray.Init( start, end, mins, maxs );
	m_gameVars.tracePlayerFunc(ray, *player, fMask);
	if ( pm.m_pEnt.m_bValid && pm.plane.normal[2] >= 0.7)
	{
		pm.fraction = fraction;
		pm.endpos = endpos;
		return;
	}

	// Check the +x, -y quadrant
	mins.Init( std::max( 0.0f, minsSrc.x ), minsSrc.y, minsSrc.z );
	maxs.Init( maxsSrc.x, std::min( 0.0f, maxsSrc.y ), maxsSrc.z );
	ray.Init( start, end, mins, maxs );
	m_gameVars.tracePlayerFunc(ray, *player, fMask);
	if ( pm.m_pEnt.m_bValid && pm.plane.normal[2] >= 0.7)
	{
		pm.fraction = fraction;
		pm.endpos = endpos;
		return;
	}

	pm.fraction = fraction;
	pm.endpos = endpos;
}

//-----------------------------------------------------------------------------
// Purpose: 
// Input  : &input - 
//-----------------------------------------------------------------------------
void CGameMovement::CategorizePosition( void )
{
	Vector point;
	trace_t pm;

	// Reset this each time we-recategorize, otherwise we have bogus friction when we jump into water and plunge downward really quickly
	player->m_surfaceFriction = 1.0f;

	// if the player hull point one unit down is solid, the player
	// is on ground
	
	// see if standing on something solid	

	// Doing this before we move may introduce a potential latency in water detection, but
	// doing it after can get us stuck on the bottom in water if the amount we move up
	// is less than the 1 pixel 'threshold' we're about to snap to.	Also, we'll call
	// this several times per frame, so we really need to avoid sticking to the bottom of
	// water on each call, and the converse case will correct itself if called twice.
	CheckWater(player);

	// observers don't have a ground entity
	if ( player->IsObserver() )
		return;

	float flOffset = 2.0f;

	point[0] = player->GetAbsOrigin()[0];
	point[1] = player->GetAbsOrigin()[1];
	point[2] = player->GetAbsOrigin()[2] - flOffset;

	Vector bumpOrigin;
	bumpOrigin = player->GetAbsOrigin();

	// Shooting up really fast.  Definitely not on ground.
	// On ladder moving up, so not on ground either
	// NOTE: 145 is a jump.
#define NON_JUMP_VELOCITY 140.0f

	float zvel = player->m_vecVelocity[2];
	bool bMovingUp = zvel > 0.0f;
	bool bMovingUpRapidly = zvel > NON_JUMP_VELOCITY;
	float flGroundEntityVelZ = 0.0f;
	if ( bMovingUpRapidly )
	{
		// Tracker 73219, 75878:  ywb 8/2/07
		// After save/restore (and maybe at other times), we can get a case where we were saved on a lift and 
		//  after restore we'll have a high local velocity due to the lift making our abs velocity appear high.  
		// We need to account for standing on a moving ground object in that case in order to determine if we really 
		//  are moving away from the object we are standing on at too rapid a speed.  Note that CheckJump already sets
		//  ground entity to NULL, so this wouldn't have any effect unless we are moving up rapidly not from the jump button.
		CBaseEntity ground = player->GetGroundEntity();
		if ( ground.m_bValid )
		{
			flGroundEntityVelZ = ground.GetAbsVelocity().z;
			bMovingUpRapidly = ( zvel - flGroundEntityVelZ ) > NON_JUMP_VELOCITY;
		}
	}

	// Was on ground, but now suddenly am not
	if ( bMovingUpRapidly || 
		( bMovingUp && player->GetMoveType() == MOVETYPE_LADDER ) )   
	{
		SetGroundEntity( NULL );
	}
	else
	{
		// Try and move down.
		TracePlayerBBox( bumpOrigin, point, MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm );
		
		// Was on ground, but now suddenly am not.  If we hit a steep plane, we are not on ground
		if ( !pm.m_pEnt.m_bValid || pm.plane.normal[2] < 0.7 )
		{
			// Test four sub-boxes, to see if any of them would have found shallower slope we could actually stand on
			TracePlayerBBoxForGround( bumpOrigin, point, GetPlayerMins(player), GetPlayerMaxs(player), MASK_PLAYERSOLID, COLLISION_GROUP_PLAYER_MOVEMENT, pm );
			if ( !pm.m_pEnt.m_bValid || pm.plane.normal[2] < 0.7 )
			{
				SetGroundEntity( NULL );
				// probably want to add a check for a +z velocity too!
				if ( ( player->m_vecVelocity.z > 0.0f ) && 
					( player->GetMoveType() != MOVETYPE_NOCLIP ) )
				{
					player->m_surfaceFriction = 0.25f;
				}
			}
			else
			{
				SetGroundEntity( &pm );
			}
		}
		else
		{
			SetGroundEntity( &pm );  // Otherwise, point to index of ent under us.
		}

#if 0
		
		//Adrian: vehicle code handles for us.
		if ( player->IsInAVehicle() == false )
		{
			// If our gamematerial has changed, tell any player surface triggers that are watching
			IPhysicsSurfaceProps *physprops = MoveHelper()->GetSurfaceProps();
			surfacedata_t *pSurfaceProp = physprops->GetSurfaceData( pm.surface.surfaceProps );
			char cCurrGameMaterial = pSurfaceProp->game.material;
			if ( !player->GetGroundEntity() )
			{
				cCurrGameMaterial = 0;
			}

			// Changed?
			if ( player->m_chPreviousTextureType != cCurrGameMaterial )
			{
				CEnvPlayerSurfaceTrigger::SetPlayerSurface( player, cCurrGameMaterial );
			}

			player->m_chPreviousTextureType = cCurrGameMaterial;
		}
#endif
	}
}

//-----------------------------------------------------------------------------
// Purpose: Determine if the player has hit the ground while falling, apply
//			damage, and play the appropriate impact sound.
//-----------------------------------------------------------------------------
void CGameMovement::CheckFalling( void )
{
	if ( player->GetGroundEntity().m_bValid != false &&
		 !IsDead() &&
		 player->m_Local.m_flFallVelocity >= PLAYER_FALL_PUNCH_THRESHOLD )
	{
		bool bAlive = true;
		float fvol = 0.5;

		if ( player->GetWaterLevel() > 0 )
		{
			// They landed in water.
		}
		else
		{
			// Scale it down if we landed on something that's floating...
			if ( player->GetGroundEntity().m_bFloating )
			{
				player->m_Local.m_flFallVelocity -= PLAYER_LAND_ON_FLOATING_OBJECT;
			}

			//
			// They hit the ground.
			//
			if( player->GetGroundEntity().m_vecAbsVelocity.z < 0.0f )
			{
				// Player landed on a descending object. Subtract the velocity of the ground entity.
				player->m_Local.m_flFallVelocity += player->GetGroundEntity().m_vecAbsVelocity.z;
				player->m_Local.m_flFallVelocity = std::max( 0.1f, player->m_Local.m_flFallVelocity );
			}

			if ( player->m_Local.m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED )
			{
				//
				// If they hit the ground going this fast they may take damage (and die).
				//
				//bAlive = MoveHelper( )->PlayerFallingDamage();
				fvol = 1.0;
			}
			else if ( player->m_Local.m_flFallVelocity > PLAYER_MAX_SAFE_FALL_SPEED / 2 )
			{
				fvol = 0.85;
			}
			else if ( player->m_Local.m_flFallVelocity < PLAYER_MIN_BOUNCE_SPEED )
			{
				fvol = 0;
			}
		}

		PlayerRoughLandingEffects( fvol );

		if (bAlive)
		{
			//MoveHelper( )->PlayerSetAnimation( PLAYER_WALK );
		}
	}

	//
	// Clear the fall velocity so the impact doesn't happen again.
	//
	if ( player->GetGroundEntity().m_bValid != false ) 
	{		
		player->m_Local.m_flFallVelocity = 0;
	}
}


void CGameMovement::PlayerRoughLandingEffects( float fvol )
{
	if ( fvol > 0.0 )
	{
		//
		// Play landing sound right away.
		//player->m_flStepSoundTime = 400;

		// Play step sound for current texture.
		//player->PlayStepSound( (Vector &)player->GetAbsOrigin(), player->m_pSurfaceData, fvol, true );

		//
		// Knock the screen around a little bit, temporary effect.
		//
		player->m_Local.m_vecPunchAngle[ROLL] = player->m_Local.m_flFallVelocity * 0.013;

		if ( player->m_Local.m_vecPunchAngle[PITCH] > 8 )
		{
			player->m_Local.m_vecPunchAngle[PITCH] = 8;
		}

	}
}
//-----------------------------------------------------------------------------
// Purpose: Use for ease-in, ease-out style interpolation (accel/decel)  Used by ducking code.
// Input  : value - 
//			scale - 
// Output : float
//-----------------------------------------------------------------------------
float CGameMovement::SplineFraction( float value, float scale )
{
	float valueSquared;

	value = scale * value;
	valueSquared = value * value;

	// Nice little ease-in, ease-out spline-like curve
	return 3 * valueSquared - 2 * valueSquared * value;
}

//-----------------------------------------------------------------------------
// Purpose: Determine if crouch/uncrouch caused player to get stuck in world
// Input  : direction - 
//-----------------------------------------------------------------------------
void CGameMovement::FixPlayerCrouchStuck( bool upward )
{
	CBaseEntity hitent;
	int i;
	Vector test;
	trace_t dummy;

	int direction = upward ? 1 : 0;

	hitent = TestPlayerPosition( player->GetAbsOrigin(), COLLISION_GROUP_PLAYER_MOVEMENT, dummy );
	if (!hitent.m_bValid)
		return;
	
	VectorCopy( player->GetAbsOrigin(), test );	
	for ( i = 0; i < 36; i++ )
	{
		Vector org = player->GetAbsOrigin();
		org.z += direction;
		player->SetAbsOrigin( org );
		hitent = TestPlayerPosition( player->GetAbsOrigin(), COLLISION_GROUP_PLAYER_MOVEMENT, dummy );
		if (!hitent.m_bValid)
			return;
	}

	player->SetAbsOrigin( test ); // Failed
}


bool CGameMovement::CanUnduck()
{
	int i;
	trace_t trace;
	Vector newOrigin;

	VectorCopy( player->GetAbsOrigin(), newOrigin );

	if ( player->GetGroundEntity().m_bValid != false )
	{
		for ( i = 0; i < 3; i++ )
		{
			newOrigin[i] += ( VEC_DUCK_HULL_MIN[i] - VEC_HULL_MIN[i] );
		}
	}
	else
	{
		// If in air an letting go of crouch, make sure we can offset origin to make
		//  up for uncrouching
		Vector hullSizeNormal = VEC_HULL_MAX - VEC_HULL_MIN;
		Vector hullSizeCrouch = VEC_DUCK_HULL_MAX - VEC_DUCK_HULL_MIN;
		Vector viewDelta = ( hullSizeNormal - hullSizeCrouch );
		VecAdd( newOrigin, -viewDelta, newOrigin );
	}

	bool saveducked = player->m_Local.m_bDucked;
	player->m_Local.m_bDucked = false;
	TracePlayerBBox( player->GetAbsOrigin(), newOrigin, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );
	player->m_Local.m_bDucked = saveducked;
	if ( trace.startsolid || ( trace.fraction != 1.0f ) )
		return false;	

	return true;
}

//-----------------------------------------------------------------------------
// Purpose: Stop ducking
//-----------------------------------------------------------------------------
void CGameMovement::FinishUnDuck( void )
{
	int i;
	trace_t trace;
	Vector newOrigin;

	VectorCopy( player->GetAbsOrigin(), newOrigin );

	if ( player->GetGroundEntity().m_bValid != false )
	{
		for ( i = 0; i < 3; i++ )
		{
			newOrigin[i] += ( VEC_DUCK_HULL_MIN[i] - VEC_HULL_MIN[i] );
		}
	}
	else
	{
		// If in air an letting go of crouch, make sure we can offset origin to make
		//  up for uncrouching
		Vector hullSizeNormal = VEC_HULL_MAX - VEC_HULL_MIN;
		Vector hullSizeCrouch = VEC_DUCK_HULL_MAX - VEC_DUCK_HULL_MIN;
		Vector viewDelta = ( hullSizeNormal - hullSizeCrouch );
		VecAdd( newOrigin, -viewDelta, newOrigin );
	}

	player->m_Local.m_bDucked = false;
	player->RemoveFlag( FL_DUCKING );
	player->m_Local.m_bDucking  = false;
	player->m_Local.m_bInDuckJump  = false;
	player->SetViewOffset( GetPlayerViewOffset( false ) );
	player->m_Local.m_flDucktime = 0;
	
	player->SetAbsOrigin( newOrigin );

	// Recategorize position since ducking can change origin
	CategorizePosition();
}


//-----------------------------------------------------------------------------
// 
//-----------------------------------------------------------------------------
void CGameMovement::UpdateDuckJumpEyeOffset( void )
{
	if ( player->m_Local.m_flDuckJumpTime != 0.0f )
	{
		float flDuckMilliseconds = std::max( 0.0f, GAMEMOVEMENT_DUCK_TIME - ( float )player->m_Local.m_flDuckJumpTime );
		float flDuckSeconds = flDuckMilliseconds / GAMEMOVEMENT_DUCK_TIME;						
		if ( flDuckSeconds > TIME_TO_UNDUCK )
		{
			player->m_Local.m_flDuckJumpTime = 0.0f;
			SetDuckedEyeOffset( 0.0f );
		}
		else
		{
			float flDuckFraction = SimpleSpline( 1.0f - ( flDuckSeconds / TIME_TO_UNDUCK ) );
			SetDuckedEyeOffset( flDuckFraction );
		}
	}
}

//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CGameMovement::FinishUnDuckJump( trace_t &trace )
{
	Vector vecNewOrigin;
	VectorCopy( player->GetAbsOrigin(), vecNewOrigin );

	//  Up for uncrouching.
	Vector hullSizeNormal = VEC_HULL_MAX - VEC_HULL_MIN;
	Vector hullSizeCrouch = VEC_DUCK_HULL_MAX - VEC_DUCK_HULL_MIN;
	Vector viewDelta = ( hullSizeNormal - hullSizeCrouch );

	float flDeltaZ = viewDelta.z;
	viewDelta.z *= trace.fraction;
	flDeltaZ -= viewDelta.z;

	player->RemoveFlag( FL_DUCKING );
	player->m_Local.m_bDucked = false;
	player->m_Local.m_bDucking  = false;
	player->m_Local.m_bInDuckJump = false;
	player->m_Local.m_flDucktime = 0.0f;
	player->m_Local.m_flDuckJumpTime = 0.0f;
	player->m_Local.m_flJumpTime = 0.0f;
	
	Vector vecViewOffset = GetPlayerViewOffset( false );
	vecViewOffset.z -= flDeltaZ;
	player->SetViewOffset( vecViewOffset );

	VecSubtract( vecNewOrigin, viewDelta, vecNewOrigin );
	player->SetAbsOrigin( vecNewOrigin );

	// Recategorize position since ducking can change origin
	CategorizePosition();
}


//-----------------------------------------------------------------------------
// Purpose: Finish ducking
//-----------------------------------------------------------------------------
void CGameMovement::FinishDuck( void )
{
	int i;

	player->AddFlag( FL_DUCKING );
	player->m_Local.m_bDucked = true;
	player->m_Local.m_bDucking = false;

	player->SetViewOffset( GetPlayerViewOffset( true ) );

	// HACKHACK - Fudge for collision bug - no time to fix this properly
	if ( player->GetGroundEntity().m_bValid != false )
	{
		for ( i = 0; i < 3; i++ )
		{
			Vector org = player->GetAbsOrigin();
			org[ i ]-= ( VEC_DUCK_HULL_MIN[i] - VEC_HULL_MIN[i] );
			player->SetAbsOrigin( org );
		}
	}
	else
	{
		Vector hullSizeNormal = VEC_HULL_MAX - VEC_HULL_MIN;
		Vector hullSizeCrouch = VEC_DUCK_HULL_MAX - VEC_DUCK_HULL_MIN;
		Vector viewDelta = ( hullSizeNormal - hullSizeCrouch );
		Vector out;
   		VecAdd( player->GetAbsOrigin(), viewDelta, out );
		player->SetAbsOrigin( out );
	}

	// See if we are stuck?
	FixPlayerCrouchStuck( true );

	// Recategorize position since ducking can change origin
	CategorizePosition();
}



//-----------------------------------------------------------------------------
// Purpose:
//-----------------------------------------------------------------------------
void CGameMovement::StartUnDuckJump( void )
{
	player->AddFlag( FL_DUCKING );
	player->m_Local.m_bDucked = true;
	player->m_Local.m_bDucking = false;

	player->SetViewOffset( GetPlayerViewOffset( true ) );

	Vector hullSizeNormal = VEC_HULL_MAX - VEC_HULL_MIN;
	Vector hullSizeCrouch = VEC_DUCK_HULL_MAX - VEC_DUCK_HULL_MIN;
	Vector viewDelta = ( hullSizeNormal - hullSizeCrouch );
	Vector out;
	VecAdd( player->GetAbsOrigin(), viewDelta, out );
	player->SetAbsOrigin( out );

	// See if we are stuck?
	FixPlayerCrouchStuck( true );

	// Recategorize position since ducking can change origin
	CategorizePosition();
}


//
//-----------------------------------------------------------------------------
// Purpose: 
// Input  : duckFraction - 
//-----------------------------------------------------------------------------
void CGameMovement::SetDuckedEyeOffset( float duckFraction )
{
	Vector vDuckHullMin = GetPlayerMins( true );
	Vector vStandHullMin = GetPlayerMins( false );

	float fMore = ( vDuckHullMin.z - vStandHullMin.z );

	Vector vecDuckViewOffset = GetPlayerViewOffset( true );
	Vector vecStandViewOffset = GetPlayerViewOffset( false );
	Vector temp = player->GetViewOffset();
	temp.z = ( ( vecDuckViewOffset.z - fMore ) * duckFraction ) +
				( vecStandViewOffset.z * ( 1 - duckFraction ) );
	player->SetViewOffset( temp );
}



//-----------------------------------------------------------------------------
// Purpose: Crop the speed of the player when ducking and on the ground.
//   Input: bInDuck - is the player already ducking
//          bInAir - is the player in air
//    NOTE: Only crop player speed once.
//-----------------------------------------------------------------------------
void CGameMovement::HandleDuckingSpeedCrop( void )
{
	if ( !m_bSpeedCropped && ( player->GetFlags() & FL_DUCKING ) && ( player->GetGroundEntity().m_bValid != false ) )
	{
		float frac = 0.33333333f;
		mv->m_flForwardMove	*= frac;
		mv->m_flSideMove	*= frac;
		mv->m_flUpMove		*= frac;
		m_bSpeedCropped		= true;
	}
}


//-----------------------------------------------------------------------------
// Purpose: Check to see if we are in a situation where we can unduck jump.
//-----------------------------------------------------------------------------
bool CGameMovement::CanUnDuckJump( trace_t &trace )
{
	// Trace down to the stand position and see if we can stand.
	Vector vecEnd( player->GetAbsOrigin() );
	vecEnd.z -= 36.0f;						// This will have to change if bounding hull change!
	TracePlayerBBox( player->GetAbsOrigin(), vecEnd, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, trace );
	if ( trace.fraction < 1.0f )
	{
		// Find the endpoint.
		vecEnd.z = player->GetAbsOrigin().z + ( -36.0f * trace.fraction );

		// Test a normal hull.
		trace_t traceUp;
		bool bWasDucked = player->m_Local.m_bDucked;
		player->m_Local.m_bDucked = false;
		TracePlayerBBox( vecEnd, vecEnd, PlayerSolidMask(), COLLISION_GROUP_PLAYER_MOVEMENT, traceUp );
		player->m_Local.m_bDucked = bWasDucked;
		if ( !traceUp.startsolid  )
			return true;	
	}

	return false;
}



//-----------------------------------------------------------------------------
// Purpose: See if duck button is pressed and do the appropriate things
//-----------------------------------------------------------------------------
void CGameMovement::Duck( void )
{
	int buttonsChanged	= ( player->m_nOldButtons ^ player->m_nButtons );	// These buttons have changed this frame
	int buttonsPressed	=  buttonsChanged & player->m_nButtons;			// The changed ones still down are "pressed"
	int buttonsReleased	=  buttonsChanged & player->m_nOldButtons;		// The changed ones which were previously down are "released"

	// Check to see if we are in the air.
	bool bInAir = ( player->GetGroundEntity().m_bValid == false );
	bool bInDuck = ( player->GetFlags() & FL_DUCKING ) ? true : false;
	bool bDuckJump = ( player->m_Local.m_flJumpTime > 0.0f );
	bool bDuckJumpTime = ( player->m_Local.m_flDuckJumpTime > 0.0f );

	if ( player->m_nButtons & IN_DUCK )
	{
		player->m_nOldButtons |= IN_DUCK;
	}
	else
	{
		player->m_nOldButtons &= ~IN_DUCK;
	}

	// Handle death.
	if ( IsDead() )
		return;

	// Slow down ducked players.
	HandleDuckingSpeedCrop();

	// If the player is holding down the duck button, the player is in duck transition, ducking, or duck-jumping.
	if ( ( player->m_nButtons & IN_DUCK ) || player->m_Local.m_bDucking  || bInDuck || bDuckJump )
	{
		// DUCK
		if ( ( player->m_nButtons & IN_DUCK ) || bDuckJump )
		{
			// Have the duck button pressed, but the player currently isn't in the duck position.
			if ( ( buttonsPressed & IN_DUCK ) && !bInDuck && !bDuckJump && !bDuckJumpTime )
			{
				player->m_Local.m_flDucktime = GAMEMOVEMENT_DUCK_TIME;
				player->m_Local.m_bDucking = true;
			}
			
			// The player is in duck transition and not duck-jumping.
			if ( player->m_Local.m_bDucking && !bDuckJump && !bDuckJumpTime )
			{
				float flDuckMilliseconds = std::max( 0.0f, GAMEMOVEMENT_DUCK_TIME - ( float )player->m_Local.m_flDucktime );
				float flDuckSeconds = flDuckMilliseconds * 0.001f;
				
				// Finish in duck transition when transition time is over, in "duck", in air.
				if ( ( flDuckSeconds > TIME_TO_DUCK ) || bInDuck || bInAir )
				{
					FinishDuck();
				}
				else
				{
					// Calc parametric time
					float flDuckFraction = SimpleSpline( flDuckSeconds / TIME_TO_DUCK );
					SetDuckedEyeOffset( flDuckFraction );
				}
			}

			if ( bDuckJump )
			{
				// Make the bounding box small immediately.
				if ( !bInDuck )
				{
					StartUnDuckJump();
				}
				else
				{
					// Check for a crouch override.
					if ( !( player->m_nButtons & IN_DUCK ) )
					{
						trace_t trace;
						if ( CanUnDuckJump( trace ) )
						{
							FinishUnDuckJump( trace );
							player->m_Local.m_flDuckJumpTime = ( GAMEMOVEMENT_TIME_TO_UNDUCK * ( 1.0f - trace.fraction ) ) + GAMEMOVEMENT_TIME_TO_UNDUCK_INV;
						}
					}
				}
			}
		}
		// UNDUCK (or attempt to...)
		else
		{
			if ( player->m_Local.m_bInDuckJump )
			{
				// Check for a crouch override.
   				if ( !( player->m_nButtons & IN_DUCK ) )
				{
					trace_t trace;
					if ( CanUnDuckJump( trace ) )
					{
						FinishUnDuckJump( trace );
					
						if ( trace.fraction < 1.0f )
						{
							player->m_Local.m_flDuckJumpTime = ( GAMEMOVEMENT_TIME_TO_UNDUCK * ( 1.0f - trace.fraction ) ) + GAMEMOVEMENT_TIME_TO_UNDUCK_INV;
						}
					}
				}
				else
				{
					player->m_Local.m_bInDuckJump = false;
				}
			}

			if ( bDuckJumpTime )
				return;

			// Try to unduck unless automovement is not allowed
			// NOTE: When not onground, you can always unduck
			if ( player->m_Local.m_bAllowAutoMovement || bInAir || player->m_Local.m_bDucking )
			{
				// We released the duck button, we aren't in "duck" and we are not in the air - start unduck transition.
				if ( ( buttonsReleased & IN_DUCK ) )
				{
					if ( bInDuck && !bDuckJump )
					{
						player->m_Local.m_flDucktime = GAMEMOVEMENT_DUCK_TIME;
					}
					else if ( player->m_Local.m_bDucking && !player->m_Local.m_bDucked )
					{
						// Invert time if release before fully ducked!!!
						float unduckMilliseconds = 1000.0f * TIME_TO_UNDUCK;
						float duckMilliseconds = 1000.0f * TIME_TO_DUCK;
						float elapsedMilliseconds = GAMEMOVEMENT_DUCK_TIME - player->m_Local.m_flDucktime;

						float fracDucked = elapsedMilliseconds / duckMilliseconds;
						float remainingUnduckMilliseconds = fracDucked * unduckMilliseconds;

						player->m_Local.m_flDucktime = GAMEMOVEMENT_DUCK_TIME - unduckMilliseconds + remainingUnduckMilliseconds;
					}
				}
				

				// Check to see if we are capable of unducking.
				if ( CanUnduck() )
				{
					// or unducking
					if ( ( player->m_Local.m_bDucking || player->m_Local.m_bDucked ) )
					{
						float flDuckMilliseconds = std::max( 0.0f, GAMEMOVEMENT_DUCK_TIME - (float)player->m_Local.m_flDucktime );
						float flDuckSeconds = flDuckMilliseconds * 0.001f;
						
						// Finish ducking immediately if duck time is over or not on ground
						if ( flDuckSeconds > TIME_TO_UNDUCK || ( bInAir && !bDuckJump ) )
						{
							FinishUnDuck();
						}
						else
						{
							// Calc parametric time
							float flDuckFraction = SimpleSpline( 1.0f - ( flDuckSeconds / TIME_TO_UNDUCK ) );
							SetDuckedEyeOffset( flDuckFraction );
							player->m_Local.m_bDucking = true;
						}
					}
				}
				else
				{
					// Still under something where we can't unduck, so make sure we reset this timer so
					//  that we'll unduck once we exit the tunnel, etc.
					if ( player->m_Local.m_flDucktime != GAMEMOVEMENT_DUCK_TIME )
					{
						SetDuckedEyeOffset(1.0f);
						player->m_Local.m_flDucktime = GAMEMOVEMENT_DUCK_TIME;
						player->m_Local.m_bDucked = true;
						player->m_Local.m_bDucking = false;
					}
				}
			}
		}
	}
	// HACK: (jimd 5/25/2006) we have a reoccuring bug (#50063 in Tracker) where the player's
	// view height gets left at the ducked height while the player is standing, but we haven't
	// been  able to repro it to find the cause.  It may be fixed now due to a change I'm
	// also making in UpdateDuckJumpEyeOffset but just in case, this code will sense the 
	// problem and restore the eye to the proper position.  It doesn't smooth the transition,
	// but it is preferable to leaving the player's view too low.
	//
	// If the player is still alive and not an observer, check to make sure that
	// his view height is at the standing height.
	else if ( !IsDead() && !player->IsObserver() && !player->IsInAVehicle() )
	{
		if ( ( player->m_Local.m_flDuckJumpTime == 0.0f ) && ( fabs(player->GetViewOffset().z - GetPlayerViewOffset( false ).z) > 0.1 ) )
		{
			// we should rarely ever get here, so assert so a coder knows when it happens
			//Assert(0);
			//DevMsg( 1, "Restoring player view height\n" );

			// set the eye height to the non-ducked height
			SetDuckedEyeOffset(0.0f);
		}
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::PlayerMove( void )
{
	CheckParameters();
	DecayPunchAngle();
	
	// clear output applied velocity

	//MoveHelper( )->ResetTouchList();                    // Assume we don't touch anything

	ReduceTimers();

	AngleVectors (mv->m_vecViewAngles, &m_vecForward, &m_vecRight, &m_vecUp );  // Determine movement angles

	// Always try and unstick us unless we are using a couple of the movement modes
	if ( player->GetMoveType() != MOVETYPE_NOCLIP && 
		 player->GetMoveType() != MOVETYPE_NONE && 		 
		 player->GetMoveType() != MOVETYPE_ISOMETRIC && 
		 player->GetMoveType() != MOVETYPE_OBSERVER ) // && !player->pl.deadflag )
	{
		if ( CheckInterval( &m_gameVars, player, IntervalType_t::STUCK ) )
		{
			if ( CheckStuck() )
			{
				// Can't move, we're stuck
				return;  
			}
		}
	}

	// Now that we are "unstuck", see where we are (player->GetWaterLevel() and type, player->GetGroundEntity()).
	if ( player->GetMoveType() != MOVETYPE_WALK ||
		player->m_bGameCodeMovedPlayer || 
		m_gameVars.sv_optimizedmovement )
	{
		CategorizePosition();
	}
	else
	{
		if ( player->m_vecVelocity.z > 250.0f )
		{
			SetGroundEntity( NULL );
		}
	}

	// Store off the starting water level
	m_nOldWaterLevel = (WaterLevel)player->GetWaterLevel();

	// If we are not on ground, store off how fast we are moving down
	if ( player->GetGroundEntity().m_bValid == false )
	{
		player->m_Local.m_flFallVelocity = -player->m_vecVelocity[ 2 ];
	}

	m_nOnLadder = 0;

	//player->UpdateStepSound( player->m_pSurfaceData, player->GetAbsOrigin(), player->m_vecVelocity );

	UpdateDuckJumpEyeOffset();
	Duck();

	// Don't run ladder code if dead on on a train
	if ( !(player->GetFlags() & FL_ONTRAIN) )
	{
		// If was not on a ladder now, but was on one before, 
		//  get off of the ladder
		
		// TODO: this causes lots of weirdness.
		//bool bCheckLadder = CheckInterval( LADDER );
		//if ( bCheckLadder || player->GetMoveType() == MOVETYPE_LADDER )
		{
			if ( !LadderMove() && 
				( player->GetMoveType() == MOVETYPE_LADDER ) )
			{
				// Clear ladder stuff unless player is dead or riding a train
				// It will be reset immediately again next frame if necessary
				player->SetMoveType( MOVETYPE_WALK );
				player->SetMoveCollide( MOVECOLLIDE_DEFAULT );
			}
		}
	}

	// Handle movement modes.
	switch (player->GetMoveType())
	{
		case MOVETYPE_NONE:
			break;

		case MOVETYPE_NOCLIP:
			FullNoClipMove( m_gameVars.sv_noclipspeed, m_gameVars.sv_noclipaccelerate );
			break;

		case MOVETYPE_FLY:
		case MOVETYPE_FLYGRAVITY:
			FullTossMove();
			break;

		case MOVETYPE_LADDER:
			FullLadderMove();
			break;

		case MOVETYPE_WALK:
			FullWalkMove();
			break;

		case MOVETYPE_ISOMETRIC:
			//IsometricMove();
			// Could also try:  FullTossMove();
			FullWalkMove();
			break;
			
		case MOVETYPE_OBSERVER:
			FullObserverMove(); // clips against world&players
			break;

		default:
			//DevMsg( 1, "Bogus pmove player movetype %i on (%i) 0=cl 1=sv\n", player->GetMoveType(), player->IsServer());
			break;
	}
}

//-----------------------------------------------------------------------------
// Performs the collision resolution for fliers.
//-----------------------------------------------------------------------------
void CGameMovement::PerformFlyCollisionResolution( trace_t &pm, Vector &move )
{
	Vector base;
	float vel;
	float backoff;

	switch (player->GetMoveCollide())
	{
	case MOVECOLLIDE_FLY_CUSTOM:
		// Do nothing; the velocity should have been modified by touch
		// FIXME: It seems wrong for touch to modify velocity
		// given that it can be called in a number of places
		// where collision resolution do *not* in fact occur

		// Should this ever occur for players!?
		//Assert(0);
		break;

	case MOVECOLLIDE_FLY_BOUNCE:	
	case MOVECOLLIDE_DEFAULT:
		{
			if (player->GetMoveCollide() == MOVECOLLIDE_FLY_BOUNCE)
				backoff = 2.0 - player->m_surfaceFriction;
			else
				backoff = 1;

			ClipVelocity (player->m_vecVelocity, pm.plane.normal, player->m_vecVelocity, backoff);
		}
		break;

	default:
		// Invalid collide type!
		//Assert(0);
		break;
	}

	// stop if on ground
	if (pm.plane.normal[2] > 0.7)
	{		
		base.Init();
		if (player->m_vecVelocity[2] < m_gameVars.sv_gravity * m_gameVars.frametime)
		{
			// we're rolling on the ground, add static friction.
			SetGroundEntity( &pm ); 
			player->m_vecVelocity[2] = 0;
		}

		vel = DotProduct( player->m_vecVelocity, player->m_vecVelocity );

		// Con_DPrintf("%f %f: %.0f %.0f %.0f\n", vel, trace.fraction, ent->velocity[0], ent->velocity[1], ent->velocity[2] );

		if (vel < (30 * 30) || (player->GetMoveCollide() != MOVECOLLIDE_FLY_BOUNCE))
		{
			SetGroundEntity( &pm ); 
			player->m_vecVelocity.Init();
		}
		else
		{
			VectorScale (player->m_vecVelocity, (1.0 - pm.fraction) * m_gameVars.frametime * 0.9, move);
			PushEntity( move, &pm );
		}
		VecSubtract( player->m_vecVelocity, base, player->m_vecVelocity );
	}
}


//-----------------------------------------------------------------------------
// Purpose: 
//-----------------------------------------------------------------------------
void CGameMovement::FullTossMove( void )
{
	trace_t pm;
	Vector move;
	
	CheckWater(player);

	// add velocity if player is moving 
	if ( (mv->m_flForwardMove != 0.0f) || (mv->m_flSideMove != 0.0f) || (mv->m_flUpMove != 0.0f))
	{
		Vector forward, right, up;
		float fmove, smove;
		Vector wishdir, wishvel;
		float wishspeed;
		int i;

		AngleVectors (mv->m_vecViewAngles, &forward, &right, &up);  // Determine movement angles

		// Copy movement amounts
		fmove = mv->m_flForwardMove;
		smove = mv->m_flSideMove;

		VectorNormalize (forward);  // Normalize remainder of vectors.
		VectorNormalize (right);    // 
		
		for (i=0 ; i<3 ; i++)       // Determine x and y parts of velocity
			wishvel[i] = forward[i]*fmove + right[i]*smove;

		wishvel[2] += mv->m_flUpMove;

		VectorCopy (wishvel, wishdir);   // Determine maginitude of speed of move
		wishspeed = VectorNormalize(wishdir);

		//
		// Clamp to server defined max speed
		//
		if (wishspeed > m_flMaxSpeed)
		{
			VectorScale (wishvel, m_flMaxSpeed/wishspeed, wishvel);
			wishspeed = m_flMaxSpeed;
		}

		// Set pmove velocity
		Accelerate ( wishdir, wishspeed, m_gameVars.sv_accelerate );
	}

	if ( player->m_vecVelocity[2] > 0 )
	{
		SetGroundEntity( NULL );
	}

	// If on ground and not moving, return.
	if ( player->GetGroundEntity().m_bValid != false )
	{
		if (player->GetBaseVelocity() == vec3_origin &&
		    player->m_vecVelocity == vec3_origin)
			return;
	}

	CheckVelocity();

	// add gravity
	if ( player->GetMoveType() == MOVETYPE_FLYGRAVITY )
	{
		AddGravity();
	}

	// move origin
	// Base velocity is not properly accounted for since this entity will move again after the bounce without
	// taking it into account
	VecAdd (player->m_vecVelocity, player->GetBaseVelocity(), player->m_vecVelocity);
	
	CheckVelocity();

	VectorScale (player->m_vecVelocity, m_gameVars.frametime, move);
	VecSubtract (player->m_vecVelocity, player->GetBaseVelocity(), player->m_vecVelocity);

	PushEntity( move, &pm );	// Should this clear basevelocity

	CheckVelocity();

	if (pm.allsolid)
	{	
		// entity is trapped in another solid
		SetGroundEntity( &pm );
		player->m_vecVelocity.Init();
		return;
	}
	
	if (pm.fraction != 1)
	{
		PerformFlyCollisionResolution( pm, move );
	}
	
	// check for in water
	CheckWater(player);
}

bool CGameMovement::GameHasLadders() const
{
	return true;
}

CBaseEntity CBasePlayer::GetGroundEntity()
{
	CBaseEntity entity;
	if(m_nFlags & FL_ONGROUND)
	{
		entity.m_bValid = true;
	}

	return entity;
}

void CBasePlayer::SetGroundEntity(CBaseEntity ground)
{
	if(ground.m_bValid)
	{
		AddFlag(FL_ONGROUND);
	}
	else
	{
		RemoveFlag(FL_ONGROUND);
	}
	m_GroundEntity = ground;
}
