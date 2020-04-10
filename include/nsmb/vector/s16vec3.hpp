#pragma once

#include "../vector.h"

class S16Vec3
{
public:
	void* vtable;
	union
	{
		fx16 a[3];
		struct
		{
			fx16 x, y, z;
		};
	};

	S16Vec3() : vtable(S16Vec3_vtable), x(0), y(0), z(0) {}
	S16Vec3(fx16 n) : vtable(S16Vec3_vtable), x(n), y(n), z(n) {}
	S16Vec3(fx16 x_, fx16 y_, fx16 z_) : vtable(S16Vec3_vtable), x(x_), y(y_), z(z_) {}
	S16Vec3(const VecFx16& v) : vtable(S16Vec3_vtable), x(v.x), y(v.y), z(v.z) {}

	VecFx16* ToVecFx16() { return cVFx16(x); }
	static VecFx16* ToVecFx16(S16Vec3& v) { return cVFx16(v.x); }

	// S16V3 <op> S16V3
	inline S16Vec3 operator + (const S16Vec3& v) { S16Vec3 out; S16Vec3::Add(out, *this, v); return out; }
	inline S16Vec3& operator += (const S16Vec3& v) { *this = *this + v; return *this; }
	inline S16Vec3 operator - (const S16Vec3& v) { S16Vec3 out; S16Vec3::Sub(out, *this, v); return out; }
	inline S16Vec3& operator -= (const S16Vec3& v) { *this = *this - v; return *this; }
	inline S16Vec3 operator * (const S16Vec3& v) { S16Vec3 out; S16Vec3::Mul(out, *this, v); return out; }
	inline S16Vec3& operator *= (const S16Vec3& v) { *this = *this * v; return *this; }
	inline S16Vec3 operator / (const S16Vec3& v) { S16Vec3 out; S16Vec3::Div(out, *this, v); return out; }
	inline S16Vec3& operator /= (const S16Vec3& v) { *this = *this / v; return *this; }

	// V3 <op> Fx
	inline S16Vec3 operator + (fx16 n) { S16Vec3 out; S16Vec3::Add(out, *this, n); return out; }
	inline S16Vec3& operator += (fx16 n) { *this = *this + n; return *this; }
	inline S16Vec3 operator - (fx16 n) { S16Vec3 out; S16Vec3::Sub(out, *this, n); return out; }
	inline S16Vec3& operator -= (fx16 n) { *this = *this - n; return *this; }
	inline S16Vec3 operator * (fx16 n) { S16Vec3 out; S16Vec3::Mul(out, *this, n); return out; }
	inline S16Vec3& operator *= (fx16 n) { *this = *this * n; return *this; }
	inline S16Vec3 operator / (fx16 n) { S16Vec3 out; S16Vec3::Div(out, *this, n); return out; }
	inline S16Vec3& operator /= (fx16 n) { *this = *this / n; return *this; }

	// other
	inline S16Vec3 operator -() { S16Vec3 out(-x, -y, -z); return out; }

	// vector math
	inline fx32 Mag() { return S16Vec3::Mag(*this); }
	inline fx32 Distance(const S16Vec3& v) { return S16Vec3::Distance(*this, v); }
	inline S16Vec3& Cross(const S16Vec3& v) { return S16Vec3::Cross(*this, *this, v); }
	inline fx32 Dot(const S16Vec3& v) { return S16Vec3::Dot(*this, v); }
	inline S16Vec3& Normalize() { return S16Vec3::Normalize(*this, *this); }

	// other math
	inline S16Vec3& Abs() { return S16Vec3::Abs(*this, *this); }
	inline S16Vec3& Int() { return S16Vec3::Int(*this, *this); }
	inline S16Vec3& Floor() { return S16Vec3::Floor(*this, *this); }
	inline S16Vec3& Ceil() { return S16Vec3::Ceil(*this, *this); }
	inline S16Vec3& Round() { return S16Vec3::Round(*this, *this); }
	inline S16Vec3& FlipX() { return S16Vec3::FlipX(*this, *this); }
	inline S16Vec3& FlipY() { return S16Vec3::FlipY(*this, *this); }
	inline S16Vec3& FlipZ() { return S16Vec3::FlipZ(*this, *this); }

private:
	// S16V3 <op> S16V3
	static inline S16Vec3& Add(S16Vec3& out, const S16Vec3& in_a, const S16Vec3& in_b)
	{
		out.x = in_a.x + in_b.x;
		out.y = in_a.y + in_b.y;
		out.z = in_a.z + in_b.z;
		return out;
	}

	static inline S16Vec3& Sub(S16Vec3& out, const S16Vec3& in_a, const S16Vec3& in_b)
	{
		out.x = in_a.x - in_b.x;
		out.y = in_a.y - in_b.y;
		out.z = in_a.z - in_b.z;
		return out;
	}

	static inline S16Vec3& Mul(S16Vec3& out, const S16Vec3& in_a, const S16Vec3& in_b)
	{
		out.x = in_a.x * in_b.x;
		out.y = in_a.y * in_b.y;
		out.z = in_a.z * in_b.z;
		return out;
	}

	static inline S16Vec3& Div(S16Vec3& out, const S16Vec3& in_a, const S16Vec3& in_b)
	{
		if (in_b.x == 0 || in_b.y == 0 || in_b.z == 0)
			return out;

		out.x = (fx16)FX_Div((fx32)in_a.x, (fx32)in_b.x);
		out.y = (fx16)FX_Div((fx32)in_a.y, (fx32)in_b.y);
		out.z = (fx16)FX_Div((fx32)in_a.z, (fx32)in_b.z);
		return out;
	}

	// S16V3 <op> Fx
	static inline S16Vec3& Add(S16Vec3& out, const S16Vec3& in_a, const fx16 in_b)
	{
		out.x = in_a.x + in_b;
		out.y = in_a.y + in_b;
		out.z = in_a.z + in_b;
		return out;
	}

	static inline S16Vec3& Sub(S16Vec3& out, const S16Vec3& in_a, const fx16 in_b)
	{
		out.x = in_a.x - in_b;
		out.y = in_a.y - in_b;
		out.z = in_a.z - in_b;
		return out;
	}

	static inline S16Vec3& Mul(S16Vec3& out, const S16Vec3& in_a, const fx16 in_b)
	{
		out.x = in_a.x * in_b;
		out.y = in_a.y * in_b;
		out.z = in_a.z * in_b;
		return out;
	}

	static inline S16Vec3& Div(S16Vec3& out, const S16Vec3& in_a, const fx16 in_b)
	{
		if (in_b == 0)
			return out;

		out.x = (fx16)FX_Div((fx32)in_a.x, (fx32)in_b);
		out.y = (fx16)FX_Div((fx32)in_a.y, (fx32)in_b);
		out.z = (fx16)FX_Div((fx32)in_a.z, (fx32)in_b);
		return out;
	}

	// vector math
	static inline fx32 Mag(const S16Vec3& in)
	{
		u64 sq = (u64)in.x * (u64)in.x + (u64)in.y * (u64)in.y + (u64)in.z * (u64)in.z;
		return (fx32)SQRT64(sq);
	}

	static inline fx32 Distance(const S16Vec3& in_a, const S16Vec3& in_b)
	{
		return S16Vec3::Mag(*cCS16V3(in_a) - *cCS16V3(in_b));
	}

	static inline S16Vec3& Cross(S16Vec3& out, const S16Vec3& in_a, const S16Vec3& in_b)
	{
		VEC_Fx16CrossProduct(cCS16V3(in_a)->ToVecFx16(), cCS16V3(in_b)->ToVecFx16(), out.ToVecFx16());
		return out;
	}

	static inline fx32 Dot(const S16Vec3& in_a, const S16Vec3& in_b)
	{
		return VEC_Fx16DotProduct(cCS16V3(in_a)->ToVecFx16(), cCS16V3(in_b)->ToVecFx16());
	}

	static inline S16Vec3& Normalize(S16Vec3& out, const S16Vec3& in)
	{
		VEC_Fx16Normalize(cCS16V3(in)->ToVecFx16(), out.ToVecFx16());
		return out;
	}

	// other math
	static inline S16Vec3& Abs(S16Vec3& out, const S16Vec3& in)
	{
		out.x = abs(in.x);
		out.y = abs(in.y);
		out.z = abs(in.z);
		return out;
	}

	static inline S16Vec3& Int(S16Vec3& out, const S16Vec3& in)
	{
		out.x = INT(in.x);
		out.y = INT(in.y);
		out.z = INT(in.z);
		return out;
	}

	static inline S16Vec3& Floor(S16Vec3& out, const S16Vec3& in)
	{
		out.x = FLOOR(in.x);
		out.y = FLOOR(in.y);
		out.z = FLOOR(in.z);
		return out;
	}

	static inline S16Vec3& Ceil(S16Vec3& out, const S16Vec3& in)
	{
		out.x = CEIL(in.x);
		out.y = CEIL(in.y);
		out.z = CEIL(in.z);
		return out;
	}

	static inline S16Vec3& Round(S16Vec3& out, const S16Vec3& in)
	{
		out.x = ROUND(in.x);
		out.y = ROUND(in.y);
		out.z = ROUND(in.z);
		return out;
	}

	static inline S16Vec3& FlipX(S16Vec3& out, const S16Vec3& in)
	{
		out.x = -in.x;
		out.y = in.y;
		out.z = in.z;
		return out;
	}

	static inline S16Vec3& FlipY(S16Vec3& out, const S16Vec3& in)
	{
		out.x = in.x;
		out.y = -in.y;
		out.z = in.z;
		return out;
	}

	static inline S16Vec3& FlipZ(S16Vec3& out, const S16Vec3& in)
	{
		out.x = in.x;
		out.y = in.y;
		out.z = -in.z;
		return out;
	}
};