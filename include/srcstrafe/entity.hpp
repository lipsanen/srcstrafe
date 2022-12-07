#pragma once

#include "srcstrafe/bspflags.h"
#include "srcstrafe/const.h"
#include "srcstrafe/shareddefs.h"
#include "srcstrafe/vector.hpp"

namespace Strafe
{
	enum class WaterLevel { WL_NotInWater, WL_Feet, WL_Waist, WL_InWater };

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
}