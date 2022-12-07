#pragma once

namespace Strafe
{
	struct Vector
	{
		float x = 0, y = 0, z = 0;

		Vector(float x, float y, float z);
		Vector() = default;
		Vector operator+(const Vector& rhs) const;
		Vector& operator+=(const Vector& rhs);
		Vector operator-(const Vector& rhs) const;
		Vector& operator-=(const Vector& rhs);
		Vector operator*(float rhs);
		Vector& operator*=(float rhs);
		Vector operator-() const;

		float LengthSqr() const;
		void Init(float x = 0, float y = 0, float z = 0);
		float& operator[](int x);
		bool operator==(const Vector& rhs) const;
		const float& operator[](int x) const;
		float Length2D() const;
		float Length() const;
		float Dot2D(const Vector& rhs) const;
		float Dot(const Vector& rhs) const;
		float VectorNormalize();
		void Scale(float f);
		void Add(const Vector& rhs);
		void Subtract(const Vector& rhs);
		void Zero();
	};

	typedef Vector QAngle;

	struct Ray_t
	{
		Vector start, end, mins, maxs;
		void Init(const Vector& start, const Vector& end, const Vector& mins, const Vector& maxs);
	};

	void AngleVectors(const Vector& v, Vector* fwd, Vector* right = nullptr, Vector* up = nullptr);
	int ClipVelocity(Vector& in, Vector& normal, Vector& out, float overbounce);
	void VectorScale(Vector& src, float scale, Vector& dest);
	void VectorMA( const Vector& start, float scale, const Vector& direction, Vector& dest );
	Vector VectorMult(const Vector& src, float scale);
	float VectorNormalize(Vector& v);
	float VectorLength(const Vector& v);
	void VectorCopy(const Vector& src, Vector& dest);
}
