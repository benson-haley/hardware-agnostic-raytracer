#ifndef GI_BAH8454_SPHERE
#define GI_BAH8454_SPHERE

#include <tuple>
#include <optional>

#include "../util.hpp"
#include "../ray.hpp"
#include "../material.hpp"

class Sphere {
public:
	Sphere(const Vector3H& position, Real radius, const Material<Sphere>& material) :
		world_position{ position },
		radius{ radius },
		material{ material }
	{}

	// TODO: We might want to move these vectors with rvalue references.
	Optional<Tuple<Vector3, Vector3>> intersects(const Ray& ray) const {
		// Get a, b, and c to perform the quadratic formula (a is equal to 1 if the ray is normalized so we ignore it).
		Real b = 2 * (
			ray.direction.x() * (ray.origin.x() - this->camera_position.x()) +
			ray.direction.y() * (ray.origin.y() - this->camera_position.y()) +
			ray.direction.z() * (ray.origin.z() - this->camera_position.z())
		);
		Real c = (
			((ray.origin.x() - this->camera_position.x()) * (ray.origin.x() - this->camera_position.x())) +
			((ray.origin.y() - this->camera_position.y()) * (ray.origin.y() - this->camera_position.y())) +
			((ray.origin.z() - this->camera_position.z()) * (ray.origin.z() - this->camera_position.z())) -
			(this->radius * this->radius)
		);
		// Obtain the discriminant (b^2 - 4ac).
		Real discriminant = (b * b) - (4 * c);
		// Return early if the discriminant is negative.
		if (discriminant < 0) {
			return {};
		}
		// Obtain the result of the quadratic equation.
		Real root_0 = (-b + std::sqrt(discriminant)) / 2;
		Real root_1 = (-b - std::sqrt(discriminant)) / 2;
		// Use the least positive root.
		Real root;
		if (root_0 < 0) {
			if (root_1 < 0) {
				return {}; // Return early if both roots are negative.
			} else {
				root = root_1;
			}
		} else {
			if (root_1 < 0) {
				root = root_0;
			} else {
				root = std::min(root_0, root_1);
			}
		}
		// Get the point and normal.
		Vector3 point{
			(ray.origin.x() + ray.direction.x() * root),
			(ray.origin.y() + ray.direction.y() * root),
			(ray.origin.z() + ray.direction.z() * root)
		};
		Vector3 normal{
			(point.x() - this->camera_position.x()),
			(point.y() - this->camera_position.y()),
			(point.z() - this->camera_position.z())
		};
		// Return the point and normal.
		Vector3 normalized_normal = normal.normalized();
		return { { point, normalized_normal } };
	}

	void obtain_camera_coordinates(const Matrix3H& view) {
		this->camera_position = from_homogeneous(view * this->world_position);
	}

	Vector3H world_position;
	Vector3 camera_position;
	Real radius;

	Material<Sphere> material;
};

#endif