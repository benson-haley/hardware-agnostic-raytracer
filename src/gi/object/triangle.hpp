#ifndef GI_BAH8454_TRIANGLE
#define GI_BAH8454_TRIANGLE

#include <tuple>
#include <optional>

#include "../util.hpp"
#include "../ray.hpp"
#include "../material.hpp"

template <typename AttributeType> using Attribute = Array<AttributeType, 3>;

template <typename Self>
class Triangle {
public:
	Triangle(const Vector3H& v0, const Vector3H& v1, const Vector3H& v2, const Material<Self>& material) : world_vertices{ v0, v1, v2 }, material{ material } {}

	// TODO: We might want to move these vectors with rvalue references.
	Optional<Tuple<Vector3, Vector3>> intersects(const Ray& ray) const { // TODO: Actually figure out what the heck this is doing.
		// Obtain the vertex positions.
		const Vector3& v0 = this->camera_vertices[0];
		const Vector3& v1 = this->camera_vertices[1];
		const Vector3& v2 = this->camera_vertices[2];
		// Compute the edges.
		Vector3 e1 = v1 - v0;
		Vector3 e2 = v2 - v0;
		// Compute the normal to the triangle.
		Vector3 ray_cross_e2 = ray.direction.cross(e2);
		Real a = e1.dot(ray_cross_e2);
		// Make sure we're not parallel to the triangle.
		if (std::abs(a) < 1e-8) {
			return {};
		}
		// Compute u.
		Real f = 1 / a;
		Vector3 s = ray.origin - v0;
		Real u = f * s.dot(ray_cross_e2);
		// Return if we're outside of the triangle.
		if (u < 0 || u > 1) {
			return {};
		}
		// Compute v.
		Vector3 q = s.cross(e1);
		Real v = f * ray.direction.dot(q);
		// Return if we're outside of the triangle.
		if (v < 0 || u + v > 1) {
			return {};
		}
		// Make sure we're not behind the ray's origin.
		Real t = f * e2.dot(q);
		if (t < 0) {
			return {};
		}
		// Compute the intersection position.
		Vector3 intersection_point = ray.origin + t * ray.direction;
		// Compute the normal.
		Vector3 normal = e1.cross(e2).normalized();
		// Return.
		return { { intersection_point, -normal } };
	}

	void obtain_camera_coordinates(const Matrix3H& view) {
		for (std::size_t i = 0; i < 3; ++i) {
			this->camera_vertices[i] = from_homogeneous(view * this->world_vertices[i]);
		}
	}

	Vector3 get_barycentric_coordinate(const Vector3& position) const {
		const Vector3& a = this->camera_vertices[0];
		const Vector3& b = this->camera_vertices[1];
		const Vector3& c = this->camera_vertices[2];
		Vector3 ab = b - a;
		Vector3 ac = c - a;
		Vector3 ap = position - a;
		Vector3 ab_cross_ac = ab.cross(ac);
		Vector3 ab_cross_ap = ab.cross(ap);
		Vector3 ac_cross_ap = ac.cross(ap);
		Real area_abc = std::abs(ab_cross_ac.dot(ab_cross_ac));
		Real area_pbc = std::abs(ab_cross_ap.dot(ab_cross_ac));
		Real area_pca = std::abs(ac_cross_ap.dot(ab_cross_ac));
		Real lambda0 = area_pbc / area_abc;
		Real lambda1 = area_pca / area_abc;
		Real lambda2 = 1.0 - lambda0 - lambda1;
		return Vector3{ lambda0, lambda1, lambda2 };
	}

	template <typename AttributeType>
	AttributeType get_interpolated_attribute(Vector3 position, const Attribute<AttributeType>& attribute) const {
		Vector3 lambda = this->get_barycentric_coordinate(position);
		return (
			attribute[0] * lambda[0] +
			attribute[1] * lambda[1] +
			attribute[2] * lambda[2]
		);
	}

	Array<Vector3H, 3> world_vertices;
	Array<Vector3, 3> camera_vertices;
	
	Material<Self> material;
};

class UVTriangle : public Triangle<UVTriangle> {
public:
	UVTriangle(
		const Attribute<Vector3H>& vertices,
		const Attribute<Vector2>& uv,
		const Material<UVTriangle>& material
	) : uv{ uv }, Triangle<UVTriangle>(vertices[0], vertices[1], vertices[2], material) {};

	Attribute<Vector2> uv;
};

class PhongTriangle : public Triangle<PhongTriangle> {
public:
	PhongTriangle(const Vector3H& v0, const Vector3H& v1, const Vector3H& v2) : Triangle<PhongTriangle>(v0, v1, v2, {}) {};
};

#endif