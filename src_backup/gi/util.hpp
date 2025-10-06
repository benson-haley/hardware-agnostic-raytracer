#ifndef GI_BAH8454_UTIL
#define GI_BAH8454_UTIL

#include <memory>

// Eigen misconfigures itself if it sees SYCL_DEVICE_ONLY.
#include <sycl/sycl.hpp>
#ifdef SYCL_DEVICE_ONLY
#undef SYCL_DEVICE_ONLY
#endif
#define EIGEN_NO_IO
#define EIGEN_NO_CUDA
#define EIGEN_DONT_VECTORIZE // Apparently we need this to stop SSE and AVX from erroring at compile time.
#define EIGEN_PERMANENTLY_DISABLE_STUPID_WARNINGS // TODO: Do we need to do this?
#define EIGEN_NO_DEBUG
#include <Eigen/Dense>

#define _CCCL_NO_EXCEPTIONS
#define CUDA_NO_EXCEPTIONS


// Prevent CUDA from including its own terminate function.
#define _LIBCUDACXX___EXCEPTION_TERMINATE_H
// Replace the default CUDA terminate function with a no-op.
namespace cuda { namespace std {
[[noreturn]] inline void terminate() noexcept {
	//__cccl_terminate();
	//_LIBCUDACXX_UNREACHABLE();
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

template <typename... Args>
auto visit(Args&&... args) {
	return cuda::std::visit(std::forward<Args>(args)...);
}

template<typename T, typename Allocator = std::allocator<T>>
using Shared = std::vector<T, Allocator>;

using Real = double;
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
class Pixel;

template <typename ObjectType>
class DeviceData {
public:
	// Camera data.
	Matrix3H* view;
	FilmPlane* film_plane;
	Ray* rays;
	// Frame buffer data.
	Pixel* pixels;
	// Renderer data.
	ObjectType* objects;
	std::size_t object_count;
};

#endif