#include "camera.hpp"

#include <memory>

#include "util.hpp"
#include "frame_buffer.hpp"
#include "ray.hpp"
#include "object/renderable_object.hpp"

Camera::Camera(sycl::queue& q, Camera::Info camera_info, FrameBuffer::Info frame_buffer_info) :
	q{ q }, ray_column_count{ frame_buffer_info.width }, ray_row_count{ frame_buffer_info.height }
{
	// Construct the film plane.
	this->film_plane = sycl::malloc_shared<FilmPlane>(sizeof(FilmPlane), this->q);
	std::construct_at(this->film_plane);
	// Construct the rays.
	this->rays = sycl::malloc_shared<Ray>(sizeof(Ray) * this->ray_column_count * this->ray_row_count, this->q);
	this->q.parallel_for(
		{this->ray_column_count * this->ray_row_count},
		[
			ray_row_count = this->ray_row_count, ray_column_count = this->ray_column_count,
			film_plane = this->film_plane, rays = this->rays
		] (std::size_t i) {
			std::size_t pixel_x = i % ray_column_count;
			std::size_t pixel_y = i / ray_column_count;
			Real film_plane_x = (pixel_x / static_cast<Real>(ray_column_count)) * film_plane->width;
			Real film_plane_y = (pixel_y / static_cast<Real>(ray_row_count)) * film_plane->height;
			Vector3 direction{
				film_plane_x - (film_plane->width / 2), // Transform the X coordinate assuming the camera looks towards the film plane's middle.
				-film_plane_y + (film_plane->height / 2), // Transform the Y coordinate assuming the camera looks towards the film plane's middle.
				-film_plane->distance // Camera is pointing towards -Z and the film plane is that distance away.
			};
			rays[pixel_y * ray_column_count + pixel_x] = { Vector3{ 0, 0, 0 }, direction };
		}
	).wait();
	// Construct the view matrix.
	this->view = sycl::malloc_shared<Matrix3H>(sizeof(this->view), this->q);
	this->look_at(camera_info.position, camera_info.center, camera_info.up);
}

Camera::~Camera() {
	sycl::free(this->rays, this->q);
	sycl::free(this->view, this->q);
	sycl::free(this->film_plane, this->q);
}

void Camera::look_at(const Vector3& position, const Vector3& center, const Vector3& up) {
	// Get the forward, right, and (recalculated to be orthogonal) up vectors.
	Vector3 forward_vector = (center - position).normalized();
	Vector3 right_vector = forward_vector.cross(up).normalized();
	Vector3 up_vector = right_vector.cross(forward_vector).normalized();
	// Set the view matrix.
	this->view->coeffRef(0, 0) = right_vector.x();
	this->view->coeffRef(1, 0) = right_vector.y();
	this->view->coeffRef(2, 0) = right_vector.z();
	this->view->coeffRef(3, 0) = 0;
	this->view->coeffRef(0, 1) = up_vector.x();
	this->view->coeffRef(1, 1) = up_vector.y();
	this->view->coeffRef(2, 1) = up_vector.z();
	this->view->coeffRef(3, 1) = 0;
	this->view->coeffRef(0, 2) = -forward_vector.x();
	this->view->coeffRef(1, 2) = -forward_vector.y();
	this->view->coeffRef(2, 2) = -forward_vector.z();
	this->view->coeffRef(3, 2) = 0;
	this->view->coeffRef(0, 3) = -right_vector.dot(position);
	this->view->coeffRef(1, 3) = -up_vector.dot(position);
	this->view->coeffRef(2, 3) = forward_vector.dot(position);
	this->view->coeffRef(3, 3) = 1;
}

Vector3 Camera::get_position() const {
	// Obtain the position.
	// We do not need to divide by w because it will always be 1 as long as the camera respects affine transformations.
	return this->view->col(3).head<3>();
}

void Camera::set_position(const Vector3& position) {
	// Change the position values in the matrix.
	// We do not need to divide the matrix by w because it will always be 1 as long as the camera respects affine transformations.
	this->view->coeffRef(0, 3) = position.x();
	this->view->coeffRef(1, 3) = position.y();
	this->view->coeffRef(2, 3) = position.z();
}

Vector3 Camera::get_up_vector() const {
	return this->view->col(1).head<3>();
}

Vector3 Camera::get_forward_vector() const {
	return this->view->col(2).head<3>();
}

Vector3 Camera::get_right_vector() const {
	return this->get_up_vector().cross(this->get_forward_vector());
}

/// Translates the camera's position.
/// @param direction Vector representing the direction of translation.  This vector must be normalized.
/// @param distance The distance to translate. 
void Camera::translate(const Vector3& direction, Real distance) {
	this->set_position(this->get_position() - distance * direction);
}