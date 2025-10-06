#ifndef GI_BAH8454_RENDERER
#define GI_BAH8454_RENDERER

// Include Boost libraries.
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
// Simplify boost namespaces.
namespace asio = boost::asio;
namespace beast = boost::beast;

// Include SYCL.
#include <sycl/sycl.hpp>

#include <chrono>
#include <execution>
#include <algorithm>

#include "util.hpp"
#include "frame_buffer.hpp"
#include "camera.hpp"
#include "light.hpp"
#include "material.hpp"
#include "object/renderable_object.hpp"

template <RenderableObject... ObjectTypes>
class Renderer {
public:
    using Object = Variant<ObjectTypes...>;

    class Callbacks {
    public:
        std::function<void(Renderer<ObjectTypes...>&)> on_load;
    };

    class Info {
    public:
        FrameBuffer::Info frame_buffer;
        Camera::Info camera;
        Callbacks callbacks;
        Vector3 background_color{ 0, 0, 0 };
    };

    Renderer(const Info& info) :
        q{ sycl::gpu_selector{} },
        objects{ SharedAllocator<Object>{this->q} },
        frame_buffer{ this->q, info.frame_buffer },
        camera{ this->q, info.camera, info.frame_buffer },
        callbacks{ info.callbacks }
    {}

    void render() {
        // Make sure all the objects are in camera coordinates.
        this->q.parallel_for(
            {this->objects.size()},
            [
                objects = this->objects.data(),
                camera_view = this->camera.view
            ](std::size_t i) {
                visit([&](auto& object) { object.obtain_camera_coordinates(*camera_view); }, objects[i]);
            }
        ).wait();
        // Draw each pixel.
        auto start = std::chrono::high_resolution_clock::now();
        this->q.parallel_for(
            { this->frame_buffer.width * this->frame_buffer.height },
            [
                data = DeviceData<Object>{
                    .view = this->camera.view,
                    .film_plane = this->camera.film_plane,
                    .rays = this->camera.rays,
                    .pixels = this->frame_buffer.pixels,
                    .objects = this->objects.data(),
                    .object_count = this->objects.size()
                }
            ]() {
                Vector3 color = Renderer::illuminate(data, ray);
                pixel = {
                    static_cast<std::uint8_t>((color.x()) * 255),
                    static_cast<std::uint8_t>((color.y()) * 255),
                    static_cast<std::uint8_t>((color.z()) * 255)
                }
            }
        ).wait();
        // for (std::size_t i = 0; i < this->frame_buffer.width * this->frame_buffer.height; ++i) {
        //     Ray& ray = this->camera.rays[i];
        //     Pixel& pixel = this->frame_buffer[i];
        //     Vector3 color = this->illuminate(ray);
        //     pixel = {
        //         static_cast<std::uint8_t>((color.x()) * 255),
        //         static_cast<std::uint8_t>((color.y()) * 255),
        //         static_cast<std::uint8_t>((color.z()) * 255)
        //     };
        // }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> duration = end - start;
        std::cout << "Time taken: " << duration.count() << " seconds" << std::endl;
    }

    static Vector3 illuminate(const DeviceData& data, const Ray& ray) {
        // Check if there was a collision.
        if (auto success = Renderer::get_nearest_collision(data, ray)) {
            auto [object, position, normal] = *success;
            MaterialInfo info{ .position = position, .normal = normal, .light_position = this->light.position, .light_color = this->light.color };
            Vector3 offset_position = position + (0.001 * normal); // TODO: Define epsilon.
            Vector3 shadow_ray_direction = (this->light.position - offset_position); // TODO: Light position is not in camera coords.
            Ray shadow_ray{ offset_position, shadow_ray_direction };
            Real distance_to_light = (this->light.position - offset_position).norm();
            Vector3 color;
            if (Renderer::get_nearest_collision(data, shadow_ray, distance_to_light)) {
                // This pixel is in shadow.
                visit([&](const auto& object) {
                    color = object.material.shadow_shader(object, info);
                }, *object); // TODO: Use optional reference instead of pointer.
            } else {
                // Update the pixel color corresponding to this ray.
                visit([&](const auto& object) {
                    color = object.material.shader(object, info);
                }, *object);
            }
            // Return the object color if no reflection recursion is necessary.
            Real reflection_constant = visit([](const auto& object) { return object.material.reflection_constant; }, *object);
            if (reflection_constant == 0) {
                return color;
            }
            // Perform reflection recursion.
            Ray reflection_ray = {
                position + (0.001 * normal), // TODO: Define epsilon.
                ray.direction - 2 * (ray.direction.dot(normal)) * normal
            };
            color = reflection_constant * Renderer::illuminate(data, reflection_ray);
            return color;
        } else {
            // If the ray hasn't hit anything, it should display the background color.
            //return this->background_color.data(); // TODO: Implement background_color.
            return { 0, 0, 0 };
        }
    }

    // Vector3 illuminate(const Ray& ray) {
    //     // Check if there was a collision.
    //     if (auto success = this->get_nearest_collision(ray)) {
    //         auto [object, position, normal] = *success;
    //         MaterialInfo info{ .position = position, .normal = normal, .light_position = this->light.position, .light_color = this->light.color };
    //         Vector3 offset_position = position + (0.001 * normal); // TODO: Define epsilon.
    //         Vector3 shadow_ray_direction = (this->light.position - offset_position); // TODO: Light position is not in camera coords.
    //         Ray shadow_ray{ offset_position, shadow_ray_direction };
    //         Real distance_to_light = (this->light.position - offset_position).norm();
    //         Vector3 color;
    //         if (this->get_nearest_collision(shadow_ray, distance_to_light)) {
    //             // This pixel is in shadow.
    //             visit([&](const auto& object) {
    //                 color = object.material.shadow_shader(object, info);
    //             }, *object); // TODO: Use optional reference instead of pointer.
    //         } else {
    //             // Update the pixel color corresponding to this ray.
    //             visit([&](const auto& object) {
    //                 color = object.material.shader(object, info);
    //             }, *object);
    //         }
    //         // Return the object color if no reflection recursion is necessary.
    //         Real reflection_constant = visit([](const auto& object) { return object.material.reflection_constant; }, *object);
    //         if (reflection_constant == 0) {
    //             return color;
    //         }
    //         // Perform reflection recursion.
    //         Ray reflection_ray = {
    //             position + (0.001 * normal), // TODO: Define epsilon.
    //             ray.direction - 2 * (ray.direction.dot(normal)) * normal
    //         };
    //         color = reflection_constant * this->illuminate(reflection_ray);
    //         return color;
    //     } else {
    //         // If the ray hasn't hit anything, it should display the background color.
    //         //return this->background_color.data(); // TODO: Implement background_color.
    //         return { 0, 0, 0 };
    //     }
    // }

    sycl::queue q;

    Shared<Object, SharedAllocator<Object>> objects;

    FrameBuffer frame_buffer;

    Camera camera;

    Callbacks callbacks;

private:
     Optional<Tuple<const Object*, Vector3, Vector3>> get_nearest_collision(
        const Ray& ray,
        Real maximum_distance = std::numeric_limits<Real>::infinity()
    ) {
        Real y = ray.origin[1];
        if (y < 0) {}
        // This is what we'll return.
        Optional<Tuple<const Object*, Vector3, Vector3>> result{};
        // Preset the distance the ray has traveled to a maximum.
        Real ray_distance = maximum_distance;
        // Iterate through each object.
        for (const auto& object_variant : this->objects) {
            visit([&](const auto& object) {
                // Check if the object will intersect with the path of the ray.
                if (auto success = object.intersects(ray)) {
                    auto& [position, normal] = *success;
                    // Check if we're closer than the previous collision.
                    Real distance = (ray.origin - position).norm();
                    if (distance > ray_distance) {
                        return; // continue;
                    }
                    // Update the ray distance.
                    ray_distance = distance;
                    // Update the object pointer.
                    result = { Tuple<const Object*, Vector3, Vector3>{ &object_variant, std::move(position), std::move(normal) } };
                }
            }, object_variant);
        }
        // Return the resultant nearest object.
        return result;
    }

    Light light{ Vector3{ 0, 2, -1 }, Vector3{ 1, 1, 1 } };
};

#endif