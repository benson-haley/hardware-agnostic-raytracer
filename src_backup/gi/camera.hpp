#ifndef GI_BAH8454_CAMERA
#define GI_BAH8454_CAMERA

#include <array>

#include "util.hpp"
#include "frame_buffer.hpp"
#include "ray.hpp"
#include "object/renderable_object.hpp"

class Camera {
public:
	class Info {
	public:
		Vector3 position;
		Vector3 center;
		Vector3 up;
	};

	class FilmPlane {
		public:
			Real width = 1;
			Real height = 0.75;
			Real distance = 0.5;
		};

	Camera(sycl::queue& q, Camera::Info camera_info, FrameBuffer::Info frame_buffer_info);
	Camera(const Camera&) = delete;
	~Camera();

	void look_at(const Vector3& position, const Vector3& center, const Vector3& up);

	void set_position(const Vector3& position); 
	Vector3 get_position() const;

	Vector3 get_forward_vector() const;
	Vector3 get_up_vector() const;

	void translate(const Vector3& direction, Real distance);

	sycl::queue& q;

	std::size_t ray_column_count;
	std::size_t ray_row_count;

	Matrix3H* view;

	FilmPlane* film_plane;

	Ray* rays;
};

#endif