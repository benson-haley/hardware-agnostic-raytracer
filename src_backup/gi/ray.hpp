#ifndef GI_BAH8454_RAY
#define GI_BAH8454_RAY

#include "util.hpp"

class Ray {
public:
	Ray() : origin{ 0, 0, 0 }, direction{ 0, 0, 0 } {}
	Ray(const Vector3& origin, const Vector3& direction) : origin{ origin }, direction{ direction.normalized() } {}

	Vector3 origin;
	Vector3 direction;
};

#endif