#pragma once

#include "const.h"
#include "strafe_utils.hpp"
#include "bspflags.h"
#include "shareddefs.h"

#define CTEXTURESMAX		512			// max number of textures loaded
#define CBTEXTURENAMEMAX	13			// only load first n chars of name

#define GAMEMOVEMENT_DUCK_TIME				1000.0f		// ms
#define GAMEMOVEMENT_JUMP_TIME				510.0f		// ms approx - based on the 21 unit height jump
#define GAMEMOVEMENT_JUMP_HEIGHT			21.0f		// units
#define GAMEMOVEMENT_TIME_TO_UNDUCK			( TIME_TO_UNDUCK * 1000.0f )		// ms
#define GAMEMOVEMENT_TIME_TO_UNDUCK_INV		( GAMEMOVEMENT_DUCK_TIME - GAMEMOVEMENT_TIME_TO_UNDUCK )

namespace Strafe
{
	struct surfacedata_t;

	struct plane_t
	{
		Vector normal;
	};

	struct trace_t
	{
		plane_t plane;
		Vector endpos;
		int contents = 0;
		float fraction = 1.0f;
		bool allsolid = false;
		bool startsolid = false;
	};

	struct CBaseHandle
	{
		int index = -1;
		int serial = -1;
	};

	typedef Vector QAngle;

	struct Ray_t
	{
		Vector start;
		Vector end;
	};

	struct CMoveData
	{
		float m_flConstraintRadius = 0;
		float m_flMaxSpeed = 320;
		float m_flForwardMove = 0;
		float m_flSideMove = 0;
		float m_flUpMove = 0;
		float m_flClientMaxSpeed = 320;
		int m_nOldButtons = 0;
		int m_nButtons = 0;
		float m_outStepHeight = 0.0f;
		bool m_bGameCodeMovedPlayer = false;
		Vector m_vecAbsOrigin;
		QAngle m_vecAngles;
		Vector m_vecOldAngles;
		Vector m_vecVelocity;
		QAngle m_vecViewAngles;
		Vector m_outWishVel;
		Vector m_outJumpVel;

		void SetAbsOrigin(const Vector& rhs) { m_vecAbsOrigin = rhs; }
		Vector GetAbsOrigin() const { return m_vecAbsOrigin; };
	};

	struct surfacedata_t
	{
		bool hasSurfaceData = false;
		float maxSpeedFactor = 1.0f;
		float jumpFactor = 1.0f;
	};

	struct CBasePlayer
	{
		struct {
			bool m_bDucked = false;
			Vector m_vecPunchAngle;
			Vector m_vecPunchAngleVel;
			float m_flDucktime = 0.0f;
			float m_flDuckJumpTime = 0.0f;
			float m_flJumpTime = 0.0f;
			bool m_bAllowAutoMovement = false;
			float m_flStepSize = 20.0f;
			bool m_bInDuckJump = false;
			bool m_bDucking = false;
			bool m_bSlowMovement = false;
			float m_flFallVelocity = 0.0f;
		} m_Local;

		struct {
			bool deadflag = false;
		} pl;

		int m_StuckLast = 0;
		int m_iCommandNumber = 0;
		int m_iHealth = 100;
		int m_nFlags = 0;
		int m_moveType = MOVETYPE_WALK;
		int m_moveCollide = MOVECOLLIDE_DEFAULT;
		float m_flWaterJumpTime = 0.0f;
		float m_surfaceFriction = 1.0f;
		Vector m_vecViewOffset;
		Vector m_vecBaseVelocity;
		Vector m_vecWaterJumpVel;
		Vector m_vecLadderNormal;
		surfacedata_t m_pSurfaceData;
		WaterLevel m_waterLevel;
		
		Vector GetViewOffset() { return m_vecViewOffset; }
		int GetWaterType() { return CONTENTS_WATER; }
		bool GetGroundEntity() { return m_nFlags & FL_ONGROUND; }
		void AddFlag(int flag) { m_nFlags |= flag; }
		void RemoveFlag(int flag) { m_nFlags &= ~flag; }
		int GetWaterLevel() const { return (int)m_waterLevel; }
		Vector GetBaseVelocity() const { return m_vecBaseVelocity; }
		void SetBaseVelocity(const Vector& rhs) { m_vecBaseVelocity = rhs; }
		void SetViewOffset(const Vector& rhs) { m_vecViewOffset = rhs; }
		int GetMoveType() const { return m_moveType; }
		int GetFlags() const { return m_nFlags; }
		int CurrentCommandNumber() const { return m_iCommandNumber; }
		int entindex() const { return 1; };
		bool IsObserver() const { return false; }
		float GetGravity() const { return 1.0f; }
		float GetStepSize() const { return m_Local.m_flStepSize; }
		void SetGravity(float f) {}
		void SetMoveType(int movetype) { m_moveType = movetype; }
		void SetMoveCollide(int movecollide) { m_moveCollide = movecollide; }
		int GetMoveCollide() { return m_moveCollide; }
		bool IsInAVehicle() { return false; }
	};

	struct GameVars
	{
		float sv_rollangle = 0.0f;
		float sv_rollspeed = 200.0f;
		float frametime = 0.015f;
		float sv_maxspeed = 320.0f;
		float sv_gravity = 600.0f;
		float sv_friction = 4.0f;
		float sv_accelerate = 10.0f;
		float sv_airaccelerate = 10.0f;
		float sv_stopspeed = 100.0f;
		float DIST_EPSILON = 0.03125f;
		float sv_maxvelocity = 3500.0f;
		float sv_specaccelerate = 5.0f;
		float sv_specspeed = 3.0f;
		float sv_bounce = 0.0f;
		float sv_noclipspeed = 5.0f;
		float sv_noclipaccelerate = 5.0f;
		bool sv_specnoclip = true;
		bool sv_optimizedmovement = true;
	};

	class CGameMovement
	{
	public:
		
		CGameMovement( void );
		virtual			~CGameMovement( void );

		virtual void	ProcessMovement( CBasePlayer *pPlayer, CMoveData *pMove );

		virtual int     GetPointContents(const Vector& v) { return CONTENTS_SOLID; };
		virtual void	StartTrackPredictionErrors( CBasePlayer *pPlayer );
		virtual void	FinishTrackPredictionErrors( CBasePlayer *pPlayer );
		virtual void	DiffPrint( char const *fmt, ... );
		virtual Vector	GetPlayerMins( bool ducked ) const;
		virtual Vector	GetPlayerMaxs( bool ducked ) const;
		virtual Vector	GetPlayerViewOffset( bool ducked ) const;

		virtual void			TracePlayerBBox( const Vector& start, const Vector& end, unsigned int fMask, int collisionGroup, trace_t& pm );
	#define BRUSH_ONLY true
		virtual unsigned int PlayerSolidMask( bool brushOnly = false );	///< returns the solid mask for the given player, so bots can have a more-restrictive set
		CBasePlayer		*player;
		CMoveData *GetMoveData() { return mv; }

		GameVars m_gameVars;

	protected:
		// Input/Output for this movement
		CMoveData		*mv;
		
		WaterLevel		m_nOldWaterLevel;
		float			m_flWaterEntryTime;
		int				m_nOnLadder;

		Vector			m_vecForward;
		Vector			m_vecRight;
		Vector			m_vecUp;


		// Does most of the player movement logic.
		// Returns with origin, angles, and velocity modified in place.
		// were contacted during the move.
		virtual float GetTickInterval();
		virtual void	PlayerMove(	void );

		// Set ground data, etc.
		void			FinishMove( void );

		virtual float	CalcRoll( const QAngle &angles, const Vector &velocity, float rollangle, float rollspeed );

		virtual	void	DecayPunchAngle( void );

		virtual void	CheckWaterJump(void );

		virtual void	WaterMove( void );

		void			WaterJump( void );

		// Handles both ground friction and water friction
		void			Friction( void );

		virtual void	AirAccelerate( Vector& wishdir, float wishspeed, float accel );

		virtual void	AirMove( void );
		
		virtual bool	CanAccelerate();
		virtual void	Accelerate( Vector& wishdir, float wishspeed, float accel);

		// Only used by players.  Moves along the ground when player is a MOVETYPE_WALK.
		virtual void	WalkMove( void );

		// Try to keep a walking player on the ground when running down slopes etc
		void			StayOnGround( void );

		// Handle MOVETYPE_WALK.
		virtual void	FullWalkMove();

		// Implement this if you want to know when the player collides during OnPlayerMove
		virtual void	OnTryPlayerMoveCollision( trace_t &tr ) {}

		virtual Vector	GetPlayerMins( void ) const; // uses local player
		virtual Vector	GetPlayerMaxs( void ) const; // uses local player

		typedef enum
		{
			GROUND = 0,
			STUCK,
			LADDER
		} IntervalType_t;

		virtual int		GetCheckInterval( IntervalType_t type );

		// Useful for things that happen periodically. This lets things happen on the specified interval, but
		// spaces the events onto different frames for different players so they don't all hit their spikes
		// simultaneously.
		bool			CheckInterval( IntervalType_t type );


		// Decompoosed gravity
		void			StartGravity( void );
		void			FinishGravity( void );

		// Apply normal ( undecomposed ) gravity
		void			AddGravity( void );

		// Handle movement in noclip mode.
		void			FullNoClipMove( float factor, float maxacceleration );

		// Returns true if he started a jump (ie: should he play the jump animation)?
		virtual bool	CheckJumpButton( void );	// Overridden by each game.

		// Dead player flying through air., e.g.
		virtual void    FullTossMove( void );
		
		// Player is a Observer chasing another player
		void			FullObserverMove( void );

		// Handle movement when in MOVETYPE_LADDER mode.
		virtual void	FullLadderMove();

		// The basic solid body movement clip that slides along multiple planes
		virtual int		TryPlayerMove( Vector *pFirstDest=NULL, trace_t *pFirstTrace=NULL );
		
		virtual bool	LadderMove( void );
		virtual bool	OnLadder( trace_t &trace );
		virtual float	LadderDistance( void ) const { return 2.0f; }	///< Returns the distance a player can be from a ladder and still attach to it
		virtual unsigned int LadderMask( void ) const { return MASK_PLAYERSOLID; }
		virtual float	ClimbSpeed( void ) const { return MAX_CLIMB_SPEED; }
		virtual float	LadderLateralMultiplier( void ) const { return 1.0f; }

		// See if the player has a bogus velocity value.
		void			CheckVelocity( void );

		// Does not change the entities velocity at all
		void			PushEntity( Vector& push, trace_t *pTrace );

		// Slide off of the impacting object
		// returns the blocked flags:
		// 0x01 == floor
		// 0x02 == step / wall
		int				ClipVelocity( Vector& in, Vector& normal, Vector& out, float overbounce );

		// If pmove.origin is in a solid position,
		// try nudging slightly on all axis to
		// allow for the cut precision of the net coordinates
		virtual int				CheckStuck( void );
		
		// Check if the point is in water.
		// Sets refWaterLevel and refWaterType appropriately.
		// If in water, applies current to baseVelocity, and returns true.
		virtual bool			CheckWater( void );
		
		// Determine if player is in water, on ground, etc.
		virtual void CategorizePosition( void );

		virtual void	CheckParameters( void );

		virtual	void	ReduceTimers( void );

		virtual void	CheckFalling( void );

		virtual void	PlayerRoughLandingEffects( float fvol );

		void			PlayerWaterSounds( void );

		void ResetGetPointContentsCache();
		int GetPointContentsCached( const Vector &point, int slot );

		// Ducking
		virtual void	Duck( void );
		virtual void	HandleDuckingSpeedCrop();
		virtual void	FinishUnDuck( void );
		virtual void	FinishDuck( void );
		virtual bool	CanUnduck();
		void			UpdateDuckJumpEyeOffset( void );
		bool			CanUnDuckJump( trace_t &trace );
		void			StartUnDuckJump( void );
		void			FinishUnDuckJump( trace_t &trace );
		void			SetDuckedEyeOffset( float duckFraction );
		void			FixPlayerCrouchStuck( bool moveup );

		float			SplineFraction( float value, float scale );

		void			CategorizeGroundSurface( trace_t &pm );

		bool			InWater( void );

		// Commander view movement
		void			IsometricMove( void );

		// Traces the player bbox as it is swept from start to end
		virtual CBaseHandle		TestPlayerPosition( const Vector& pos, int collisionGroup, trace_t& pm );

		// Checks to see if we should actually jump 
		void			PlaySwimSound();

		bool			IsDead( void ) const;

		// Figures out how the constraint should slow us down
		float			ComputeConstraintSpeedFactor( void );

		virtual void	SetGroundEntity( trace_t *pm );

		virtual void	StepMove( Vector &vecDestination, trace_t &trace );

	protected:

		// Performs the collision resolution for fliers.
		void			PerformFlyCollisionResolution( trace_t &pm, Vector &move );

		virtual bool	GameHasLadders() const;

		enum
		{
			// eyes, waist, feet points (since they are all deterministic
			MAX_PC_CACHE_SLOTS = 3,
		};

		// Cache used to remove redundant calls to GetPointContents().
		int m_CachedGetPointContents[ MAX_PLAYERS ][ MAX_PC_CACHE_SLOTS ];
		Vector m_CachedGetPointContentsPoint[ MAX_PLAYERS ][ MAX_PC_CACHE_SLOTS ];	

		Vector			m_vecProximityMins;		// Used to be globals in sv_user.cpp.
		Vector			m_vecProximityMaxs;

		float			m_fFrameTime;

	//private:
		bool			m_bSpeedCropped;

		float			m_flStuckCheckTime[MAX_PLAYERS+1][2]; // Last time we did a full test

		// special function for teleport-with-duck for episodic
	#ifdef HL2_EPISODIC
	public:
		void			ForceDuck( void );

	#endif
	};
}

