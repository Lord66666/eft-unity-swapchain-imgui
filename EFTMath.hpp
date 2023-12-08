#include "DebugUtils.hpp"

class Vector2
{
public:
	float x, y;
	
	inline Vector2()
	{
		x = y = 0.0f;
	}

	inline Vector2(float _x, float _y)
	{
		x = _x; y = _y;
	}

	inline Vector2 operator+(float v) const 
	{
		return Vector2(x + v, y + v);
	}

	inline Vector2 operator-(float v) const 
	{
		return Vector2(x - v, y - v);
	}

	inline Vector2& operator+=(float v) 
	{
		x += v; y += v; return *this;
	}

	inline Vector2& operator*=(float v) 
	{
		x *= v; y *= v; return *this;
	}

	inline Vector2& operator/=(float v) 
	{
		x /= v; y /= v; return *this;
	}

	inline Vector2 operator-(const Vector2& v) const 
	{
		return Vector2(x - v.x, y - v.y);
	}

	inline Vector2 operator+(const Vector2& v) const 
	{
		return Vector2(x + v.x, y + v.y);
	}

	inline Vector2& operator+=(const Vector2& v) 
	{
		x += v.x; y += v.y; return *this;
	}

	inline Vector2& operator-=(const Vector2& v) 
	{
		x -= v.x; y -= v.y; return *this;
	}

	inline Vector2 operator/(float v) const 
	{
		return Vector2(x / v, y / v);
	}

	inline Vector2 operator*(float v) const 
	{
		return Vector2(x * v, y * v);
	}

	inline Vector2 operator/(const Vector2& v) const 
	{
		return Vector2(x / v.x, y / v.y);
	}

	inline bool Zero()
	{
		return x == 0 && y == 0;
	}
};

class Vector3
{
public:
	float x, y, z;

	inline Vector3()
	{
		x = y = z = 0.0f;
	}

	inline Vector3(float X, float Y, float Z)
	{
		x = X; y = Y; z = Z;
	}

	inline float operator[](int i) const
	{
		return ((float*)this)[i];
	}

	inline Vector3& operator+=(float v)
	{
		x += v; y += v; z += v;
		return *this;
	}

	inline Vector3& operator-=(float v)
	{
		x -= v; y -= v; z -= v;
		return *this;
	}

	inline Vector3& operator-=(const Vector3& v)
	{
		x -= v.x; y -= v.y; z -= v.z;
		return *this;
	}

	inline Vector3 operator*(float v) const
	{
		return Vector3(x * v, y * v, z * v);
	}

	inline Vector3 operator/(float v) const 
	{
		return Vector3(x / v, y / v, z / v)
			;
	}

	inline Vector3 operator*(const Vector3& V) const 
	{
		return Vector3(x * V.x, y * V.y, z * V.z);
	}

	inline Vector3 operator/(const Vector3& V) const
	{
		return Vector3(x / V.x, y / V.y, z / V.z);
	}

	inline Vector3& operator+=(const Vector3& v)
	{
		x += v.x; y += v.y; z += v.z;
		return *this;
	}

	inline Vector3 operator-(const Vector3& v) const
	{
		return Vector3(x - v.x, y - v.y, z - v.z);
	}

	inline Vector3 operator+(const Vector3& v) const
	{
		return Vector3(x + v.x, y + v.y, z + v.z);
	}

	inline Vector3& operator/=(float v)
	{
		x /= v; y /= v; z /= v;
		return *this;
	}

	inline bool Zero()
	{
		return x == 0 && y == 0 && z == 0;
	}

	inline float Dot(Vector3 v)
	{
		return x * v.x + y * v.y + z * v.z;
	}

	inline float Length()
	{
		return CRT::m_sqrtf(x * x + y * y + z * z);
	}

	inline float Distance(const Vector3& Other) const
	{
		const Vector3& a = *this;

		float dx = (a.x - Other.x);
		float dy = (a.y - Other.y);
		float dz = (a.z - Other.z);

		return CRT::m_sqrtf((dx * dx) + (dy * dy) + (dz * dz));
	}

	inline Vector3 Normalize()
	{
		Vector3 vector;
		float length = this->Length();

		if (length != 0) 
		{
			vector.x = x / length;
			vector.y = y / length;
			vector.z = z / length;
		}
		else
		{
			vector.x = 0.0f, vector.y = 0.0f, vector.z = 1.0f;
		}

		return vector;
	}
};

class Matrix4x4
{
public:
	float m[4][4];

	Matrix4x4() : m()
	{

	}

	inline float* operator[](int i)
	{
		return m[i];
	}

	inline const float* operator[](int i) const
	{
		return m[i];
	}

	inline Matrix4x4(
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33)
	{
		Init(
			m00, m01, m02, m03,
			m10, m11, m12, m13,
			m20, m21, m22, m23,
			m30, m31, m32, m33
		);
	}

	inline void Init(
		float m00, float m01, float m02, float m03,
		float m10, float m11, float m12, float m13,
		float m20, float m21, float m22, float m23,
		float m30, float m31, float m32, float m33
	)
	{
		m[0][0] = m00;
		m[0][1] = m01;
		m[0][2] = m02;
		m[0][3] = m03;

		m[1][0] = m10;
		m[1][1] = m11;
		m[1][2] = m12;
		m[1][3] = m13;

		m[2][0] = m20;
		m[2][1] = m21;
		m[2][2] = m22;
		m[2][3] = m23;

		m[3][0] = m30;
		m[3][1] = m31;
		m[3][2] = m32;
		m[3][3] = m33;
	}

	Matrix4x4 Transpose() const
	{
		return Matrix4x4(
			m[0][0], m[1][0], m[2][0], m[3][0],
			m[0][1], m[1][1], m[2][1], m[3][1],
			m[0][2], m[1][2], m[2][2], m[3][2],
			m[0][3], m[1][3], m[2][3], m[3][3]);
	}
};

class Ray
{
public:
	Vector3 m_Origin;
	Vector3 m_Direction;

	Ray(Vector3& origin, Vector3& direction)
	{
		m_Origin = origin;
		m_Direction = direction.Normalize();
	}
};

struct UnityColor
{
	UnityColor() : R(0.f), G(0.f), B(0.f), A(0.f)
	{

	}

	UnityColor(float _R, float _G, float _B, float _A) : R(_R / 255.0f), G(_G / 255.0f), B(_B / 255.0f), A(_A / 255.0f)
	{

	}

	float R;
	float G;
	float B;
	float A;
};