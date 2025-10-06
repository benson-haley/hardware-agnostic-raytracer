#ifndef GI_BAH8454_LIGHT
#define GI_BAH8454_LIGHT

#include "util.hpp"

class Light {
public:
	Light(const Vector3H& world_position, const Vector3& color = { 1.1, 1.1, 1.1 }) : world_position{ world_position }, color{ color } {}

	void obtain_camera_coordinates(const Matrix3H& view) {
		this->camera_position = from_homogeneous(view * this->world_position);
	}

	Vector3H world_position;
	Vector3 camera_position;
	Vector3 color;
};

#endif