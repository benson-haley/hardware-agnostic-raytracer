#ifndef GI_BAH8454_LIGHT
#define GI_BAH8454_LIGHT

#include "util.hpp"

class Light {
public:
	Light(const Vector3& position, const Vector3& color = { 0, 0, 0 }) : position{ position }, color{ color } {}

	Vector3 position;
	Vector3 color;
};

#endif