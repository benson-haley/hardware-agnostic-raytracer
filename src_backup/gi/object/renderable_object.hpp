#ifndef GI_BAH8454_RENDERABLE_OBJECT
#define GI_BAH8454_RENDERABLE_OBJECT

#include "../util.hpp"
#include "../ray.hpp"
#include "../material.hpp"
#include "sphere.hpp"
#include "triangle.hpp"

template <typename T>
concept RenderableObject = requires (T object, const Ray& ray, const Matrix3H& view) {
	//{ object.intersects(ray) } -> std::same_as<Optional<Tuple<Vector3, Vector3>>>;
	//{ object.obtain_camera_coordinates(view) } -> std::same_as<void>;
	//{ object.materal } -> std::same_as<Material<T>>;
	true;
};

#endif