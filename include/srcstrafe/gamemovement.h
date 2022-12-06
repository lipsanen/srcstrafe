#pragma once

#include "const.h"
#include "srcstrafe/strafe_utils.h"
#include "srcstrafe/shareddefs.h"
#include "bspflags.h"

#define CTEXTURESMAX 512	// max number of textures loaded
#define CBTEXTURENAMEMAX 13 // only load first n chars of name

#define GAMEMOVEMENT_DUCK_TIME 1000.0f						   // ms
#define GAMEMOVEMENT_JUMP_TIME 510.0f						   // ms approx - based on the 21 unit height jump
#define GAMEMOVEMENT_JUMP_HEIGHT 21.0f						   // units
#define GAMEMOVEMENT_TIME_TO_UNDUCK (TIME_TO_UNDUCK * 1000.0f) // ms
#define GAMEMOVEMENT_TIME_TO_UNDUCK_INV (GAMEMOVEMENT_DUCK_TIME - GAMEMOVEMENT_TIME_TO_UNDUCK)

namespace Strafe
{
	struct surfacedata_t;
	struct GameVars;
	struct CMoveData;
	typedef int EntityHandle_t;
	struct CBasePlayer;

	struct plane_t
	{
		plane_t() {normal.z = 1.0f;}
		Vector normal;
	};

	struct CBaseEntity
	{
		Vector m_vecBaseVelocity;
		Vector m_vecAbsVelocity;
		bool m_bValid = false;
		bool m_bFloating = false;
		bool m_bHeldByPlayer = false;
		int index = -1;

		Vector GetAbsVelocity() { return m_vecAbsVelocity; }
	};

	struct trace_t
	{
		plane_t plane;
		Vector endpos;
		int contents = 0;
		float fraction = 1.0f;
		bool allsolid = false;
		bool startsolid = false;
		CBaseEntity m_pEnt;
	};

	typedef std::function<float(trace_t &pm)> GetSurfaceFriction_t;
	typedef std::function<trace_t(const Ray_t &, const CBasePlayer &player, unsigned int fMask)> TracePlayer_t;
	typedef std::function<void(int dmg)> TakeDamage_t;

	float GetSurfaceFrictionDefault(trace_t &pm);
	trace_t TracePlayerDefault(const Ray_t &, const CBasePlayer &player, unsigned int fMask);
	void TakeDamageDefault(int dmg);
	bool CheckInterval(const GameVars *vars, const CBasePlayer *player, IntervalType_t type);
	Vector GetPlayerMins(const CBasePlayer *player);
	Vector GetPlayerMins(bool ducked);
	Vector GetPlayerMaxs(const CBasePlayer *player);
	Vector GetPlayerMaxs(bool ducked);
	void CategorizeGroundSurface(CBasePlayer *player, const GameVars *vars, trace_t &pm);
	bool CheckWater(const CBasePlayer *player);
	void TracePlayerBBoxForGround(const GameVars *vars, const CBasePlayer *player, const Vector &start, const Vector &end, const Vector &minsSrc,
								  const Vector &maxsSrc, unsigned int fMask,
								  int collisionGroup, trace_t &pm);
	void TracePlayerBBox(const CBasePlayer *player, const GameVars *vars, const Vector &start, const Vector &end, unsigned int fMask, int collisionGroup, trace_t &pm);
	unsigned int PlayerSolidMask(bool brushOnly = false); ///< returns the solid mask for the given player, so bots can have a more-restrictive set
	CBaseEntity TestPlayerPosition(const CBasePlayer *player, const GameVars *vars, const Vector &pos, int collisionGroup, trace_t &pm);
	void Accelerate( CBasePlayer* player, const GameVars* vars, Vector& wishdir, float wishspeed, float accel);
	void AirAccelerate(CBasePlayer* player, const GameVars* vars, Vector &wishdir, float wishspeed, float accel);
	void StartGravity(CBasePlayer* player, const GameVars* vars);
	void CheckVelocity(CBasePlayer* player, const GameVars* vars);

	struct CBaseHandle
	{
		int index = -1;
		int serial = -1;
	};

	struct MovementState
	{
		float m_flMaxSpeed = 320.0f;
		Vector m_vecForward;
		Vector m_vecRight;
		Vector m_vecUp;
		bool m_bSpeedCropped = false;
		// Input/Output for this movement
		CMoveData *mv = nullptr;
	};

	typedef Vector QAngle;

	struct CMoveData
	{
		float m_flForwardMove = 0;
		float m_flSideMove = 0;
		float m_flUpMove = 0;
		QAngle m_vecViewAngles;
	};

	struct surfacedata_t
	{
		bool hasSurfaceData = false;
		float maxSpeedFactor = 1.0f;
		float jumpFactor = 1.0f;
	};

	struct CBasePlayer
	{
		virtual ~CBasePlayer() {}

		struct
		{
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
			float m_flFallVelocity = 0.0f;
		} m_Local;

		CBaseEntity m_GroundEntity;
		int m_iCommandNumber = 0;
		int m_nFlags = 0;
		int m_moveType = MOVETYPE_WALK;
		int m_moveCollide = MOVECOLLIDE_DEFAULT;
		float m_flWaterJumpTime = 0.0f;
		float m_surfaceFriction = 1.0f;
		float m_flClientMaxSpeed = 320.0f;
		WaterLevel m_waterLevel = WaterLevel::WL_NotInWater;
		bool m_bGameCodeMovedPlayer = false;
		Vector m_vecAbsOrigin;
		Vector m_vecViewOffset;
		Vector m_vecBaseVelocity;
		Vector m_vecWaterJumpVel;
		Vector m_vecLadderNormal;
		surfacedata_t m_pSurfaceData;
		int m_nOldButtons = 0;
		int m_nButtons = 0;
		QAngle m_vecAngles;
		Vector m_vecOldAngles;
		Vector m_vecVelocity;

		void SetAbsOrigin(const Vector &rhs) { m_vecAbsOrigin = rhs; }
		Vector GetAbsOrigin() const { return m_vecAbsOrigin; };
		Vector GetViewOffset() { return m_vecViewOffset; }
		int GetWaterType() { return CONTENTS_WATER; }
		void AddFlag(int flag) { m_nFlags |= flag; }
		void RemoveFlag(int flag) { m_nFlags &= ~flag; }
		int GetWaterLevel() const { return (int)m_waterLevel; }
		Vector GetBaseVelocity() const { return m_vecBaseVelocity; }
		void SetBaseVelocity(const Vector &rhs) { m_vecBaseVelocity = rhs; }
		void SetViewOffset(const Vector &rhs) { m_vecViewOffset = rhs; }
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
		TracePlayer_t tracePlayerFunc = TracePlayerDefault;
		GetSurfaceFriction_t surfaceFrictionFunc = GetSurfaceFrictionDefault;
		TakeDamage_t takeDamageFunc = TakeDamageDefault;
	};

	class CGameMovement
	{
	public:
		CGameMovement(void);
		virtual ~CGameMovement(void);

		virtual void ProcessMovement(CBasePlayer *pPlayer, CMoveData *pMove);

		virtual int GetPointContents(const Vector &v) { return CONTENTS_SOLID; };
		virtual void DiffPrint(char const *fmt, ...);
		virtual Vector GetPlayerViewOffset(bool ducked) const;
		CMoveData *GetMoveData() { return m_State.mv; }

		CBasePlayer *player;
		GameVars m_gameVars;
		MovementState m_State;

	protected:
		void SetGroundEntity(CBaseEntity ground);
		virtual CBaseEntity GetGroundEntity();

		// Does most of the player movement logic.
		// Returns with origin, angles, and velocity modified in place.
		// were contacted during the move.
		virtual void PlayerMove(void);

		// Set ground data, etc.
		void FinishMove(void);

		virtual float CalcRoll(const QAngle &angles, const Vector &velocity, float rollangle, float rollspeed);

		virtual void DecayPunchAngle(void);

		virtual void CheckWaterJump(void);

		virtual void WaterMove(void);

		void WaterJump(void);

		// Handles both ground friction and water friction
		void Friction(void);

		virtual void AirMove(void);

		// Only used by players.  Moves along the ground when player is a MOVETYPE_WALK.
		virtual void WalkMove(void);

		// Try to keep a walking player on the ground when running down slopes etc
		void StayOnGround(void);

		// Handle MOVETYPE_WALK.
		virtual void FullWalkMove();

		// Implement this if you want to know when the player collides during OnPlayerMove
		virtual void OnTryPlayerMoveCollision(trace_t &tr) {}

		// Decompoosed gravity
		void FinishGravity(void);

		// Apply normal ( undecomposed ) gravity
		void AddGravity(void);

		// Handle movement in noclip mode.
		void FullNoClipMove(float factor, float maxacceleration);

		// Returns true if he started a jump (ie: should he play the jump animation)?
		virtual bool CheckJumpButton(void); // Overridden by each game.

		// Handle movement when in MOVETYPE_LADDER mode.
		virtual void FullLadderMove();

		// The basic solid body movement clip that slides along multiple planes
		virtual int TryPlayerMove(Vector *pFirstDest = NULL, trace_t *pFirstTrace = NULL);

		virtual bool LadderMove(void);
		virtual bool OnLadder(trace_t &trace);
		virtual float LadderDistance(void) const { return 2.0f; } ///< Returns the distance a player can be from a ladder and still attach to it
		virtual unsigned int LadderMask(void) const { return MASK_PLAYERSOLID; }
		virtual float ClimbSpeed(void) const { return MAX_CLIMB_SPEED; }
		virtual float LadderLateralMultiplier(void) const { return 1.0f; }

		// Does not change the entities velocity at all
		void PushEntity(Vector &push, trace_t *pTrace);

		// Slide off of the impacting object
		// returns the blocked flags:
		// 0x01 == floor
		// 0x02 == step / wall
		int ClipVelocity(Vector &in, Vector &normal, Vector &out, float overbounce);

		// If pmove.origin is in a solid position,
		// try nudging slightly on all axis to
		// allow for the cut precision of the net coordinates
		virtual int CheckStuck(void);

		// Determine if player is in water, on ground, etc.
		virtual void CategorizePosition(void);

		virtual void CheckParameters(void);

		virtual void ReduceTimers(void);

		virtual void CheckFalling(void);

		virtual void PlayerRoughLandingEffects(float fvol);

		void PlayerWaterSounds(void);

		void ResetGetPointContentsCache();
		int GetPointContentsCached(const Vector &point, int slot);

		// Ducking
		virtual void Duck(void);
		virtual void HandleDuckingSpeedCrop();
		virtual void FinishUnDuck(void);
		virtual void FinishDuck(void);
		virtual bool CanUnduck();
		void UpdateDuckJumpEyeOffset(void);
		bool CanUnDuckJump(trace_t &trace);
		void StartUnDuckJump(void);
		void FinishUnDuckJump(trace_t &trace);
		void SetDuckedEyeOffset(float duckFraction);
		void FixPlayerCrouchStuck(bool moveup);

		float SplineFraction(float value, float scale);

		bool InWater(void);

		virtual void SetGroundEntity(trace_t *pm);

		virtual void StepMove(Vector &vecDestination, trace_t &trace);

	protected:
		// Performs the collision resolution for fliers.
		void PerformFlyCollisionResolution(trace_t &pm, Vector &move);

		virtual bool GameHasLadders() const;

		// private:

		//float m_flStuckCheckTime[MAX_PLAYERS + 1][2]; // Last time we did a full test
		// special function for teleport-with-duck for episodic
#ifdef HL2_EPISODIC
	public:
		void ForceDuck(void);

#endif
	};
}
