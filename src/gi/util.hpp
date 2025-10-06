#ifndef GI_BAH8454_UTIL
#define GI_BAH8454_UTIL

#include <utility>
#include <memory>
#include <exception>

// Eigen misconfigures itself if it sees SYCL_DEVICE_ONLY so we must include SYCL first and then disable this definition.
#include <sycl/sycl.hpp>
#ifdef SYCL_DEVICE_ONLY
#undef SYCL_DEVICE_ONLY
#endif
#define EIGEN_NO_IO // GPUs do not have file handles to print to.
#define EIGEN_NO_CUDA // Don't enable CUDA-specific optimizations that will break on 
#define EIGEN_DONT_VECTORIZE // We need this to stop Eigen from trying to use SSE and AVX intrinsics.
#define EIGEN_PERMANENTLY_DISABLE_STUPID_WARNINGS // This stops Eigen from emitting warnings on the highest warning level.
#define EIGEN_NO_DEBUG // This prevent Eigen from throwing exceptions (GPU stack cannot be unrolled).
#include <Eigen/Dense>

#define _CCCL_NO_EXCEPTIONS // Remove some CUDA library exceptions.
#define CUDA_NO_EXCEPTIONS // Remove the rest of the CUDA library exceptions.

// Prevent CUDA from including its own terminate function.
#define _LIBCUDACXX___EXCEPTION_TERMINATE_H
// Replace the default CUDA terminate function.
namespace cuda { namespace std {
[[noreturn]] ACPP_UNIVERSAL_TARGET inline void terminate() {
	__acpp_if_target_host(::std::terminate(););
	__acpp_if_target_cuda(__trap(););
	::std::unreachable();
}
} }

#include <cuda/std/variant>
template <typename... Ts>
using Variant = cuda::std::variant<Ts...>;
#include <cuda/std/tuple>
template <typename... Ts>
using Tuple = cuda::std::tuple<Ts...>;
#include <cuda/std/optional>
template <typename T>
using Optional = cuda::std::optional<T>;
#include <cuda/std/array>
template <typename T, std::size_t n>
using Array = cuda::std::array<T, n>;

#include <cuda/std/cmath>
template <typename... Args>
auto pow(Args&&... args) {
	return cuda::std::pow(std::forward<Args>(args)...);
}

template <typename... Args>
auto visit(Args&&... args) {
	return cuda::std::visit(std::forward<Args>(args)...);
}

template<typename T, typename Allocator = std::allocator<T>>
using Shared = std::vector<T, Allocator>;

using Real = float;

inline Real operator ""_r(long double x) { return static_cast<Real>(x); }
inline Real operator ""_r(unsigned long long x) { return static_cast<Real>(x); }

using Vector2 = Eigen::Vector2<Real>;
using Vector3H = Eigen::Vector4<Real>;
using Vector3 = Eigen::Vector3<Real>;
using Matrix3H = Eigen::Matrix4<Real>;
using Matrix3 = Eigen::Matrix3<Real>;

inline Vector3 reflect(const Vector3& x, const Vector3& y) {
	return x - 2 * x.dot(y) * y;
}

inline Vector3 from_homogeneous(const Vector3H& v) {
	return v.head<3>() / v.w();
}

inline Real absolute_illuminance(Vector3 value) {
	return 0.27_r * value[0] + 0.67_r * value[1] + 0.06_r * value[2];
}

template <typename T>
class SharedAllocator {
public:
	using value_type = T;

	SharedAllocator(sycl::queue& q) : q{ q } {};

    template <typename U>
    SharedAllocator(const SharedAllocator<U>& other) : q{ other.q } {}

	T* allocate(std::size_t n) {
        return sycl::malloc_shared<T>(sizeof(T) * n, this->q);
    }

	void deallocate(T* p, std::size_t n) {
		sycl::free(p, this->q);
    }

	sycl::queue& q;
};

class Ray;
class FilmPlane;
class Light;

template <typename ObjectType>
class DeviceData {
public:
	// Camera data.
	Matrix3H* view;
	FilmPlane* film_plane;
	Ray* rays;
	// Frame buffer data.
	Vector3* pixels;
	// Renderer data.
	ObjectType* objects;
	std::size_t object_count;
	Light* lights;
	std::size_t light_count;
};

template <typename T>
class KDTreeNode {
public:
	KDTreeNode() {}

	enum class Axis { x, y, z };
	Axis axis{ Axis::x };
	Real value = 0;
	std::vector<T*> objects{};
	std::unique_ptr<KDTreeNode<T>> front = nullptr;
	std::unique_ptr<KDTreeNode<T>> back = nullptr;

	void fully_deepen(std::size_t depth = 0) {
		if (depth > 12) { return; }
		this->deepen();
		this->front->fully_deepen(depth + 1);
		this->back->fully_deepen(depth + 1);
	}

	void deepen() {
		// Return if deepening is impossible.
		if (this->objects.size() <= 1) { return; }
		// Get d.
		std::size_t d;
		if (this->axis == Axis::x) { d = 0; }
		else if (this->axis == Axis::y) { d = 1; }
		else /*if (this->axis == Axis::z)*/ { d = 2; }
		// Obtain a value to partition on.
		this->value = this->objects[(this->objects.size() - 1) / 2]->camera_vertices[0][d];
		// Create the front and back nodes, and populate them with the triangles.
		this->front = std::make_unique<KDTreeNode<T>>();
		this->back = std::make_unique<KDTreeNode<T>>();
		for (T* object : this->objects) {
			// If all three vertices are in front of the axis, put it in front.
			if (object->camera_vertices[0][d] > this->value && object->camera_vertices[1][d] > this->value && object->camera_vertices[2][d] > this->value) {
				this->front->objects.push_back(object);
			}
			// If all three vertices are behind the axis, put it in back.
			else if (object->camera_vertices[0][d] < this->value && object->camera_vertices[1][d] < this->value && object->camera_vertices[2][d] < this->value) {
				this->back->objects.push_back(object);
			}
			// Otherwise, put it in both.
			else {
				this->front->objects.push_back(object);
				this->back->objects.push_back(object);
			}
		}
		// Set the axes of the front and back nodes to the axis after the current one (round-robin).
		Axis next_axis;
		if (this->axis == Axis::x) { next_axis = Axis::y; }
		else if (this->axis == Axis::y) { next_axis = Axis::z; }
		else /*if (this->axis == Axis::z)*/ { next_axis = Axis::x; }
		front->axis = next_axis;
		back->axis = next_axis;
		// Clear the objects in this node.
		this->objects.clear();
	}
};

#endif