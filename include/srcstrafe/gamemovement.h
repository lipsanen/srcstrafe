#pragma once

#include "const.h"
#include "srcstrafe/entity.hpp"
#include "srcstrafe/strafe_utils.h"
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

	struct CMoveData
	{
		float m_flForwardMove = 0;
		float m_flSideMove = 0;
		float m_flUpMove = 0;
		QAngle m_vecViewAngles;
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
