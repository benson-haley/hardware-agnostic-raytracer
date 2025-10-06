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
#include <string_view>
#include <string>

#include "util.hpp"
#include "frame_buffer.hpp"
#include "camera.hpp"
#include "light.hpp"
#include "material.hpp"
#include "object/renderable_object.hpp"

#include "../ply/happly.hpp"

template <RenderableObject... ObjectTypes>
class Renderer {
public:
    using Object = Variant<ObjectTypes...>;

    class Callbacks {
    public:
        std::function<void(Renderer<ObjectTypes...>&)> on_load;
        std::function<void(Renderer<ObjectTypes...>&, Real)> on_frame;
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
        lights{ SharedAllocator<Object>{this->q} },
        frame_buffer{ this->q, info.frame_buffer },
        camera{ this->q, info.camera, info.frame_buffer },
        callbacks{ info.callbacks }
    {
        // Perform code the user wants run before the session starts.
        this->callbacks.on_load(*this);
        // KD Tree.
        // auto start = std::chrono::high_resolution_clock::now();
        // for (auto& object : this->objects) {
        //     visit([&](auto& object) {
        //         if constexpr (std::is_same_v<decltype(object), PhongTriangle&>) {
        //             this->tree.objects.push_back(&object);
        //         }
        //     }, object);
        // }
        // this->tree.fully_deepen();
        // auto end = std::chrono::high_resolution_clock::now();
        // std::chrono::duration<Real> delta = end - start;
        // std::cout << "KD time taken: " << delta.count() << " seconds" << std::endl;
        // Device info.
        //std::cout << this->q.get_device().template get_info<sycl::info::device::name>() << std::endl;
    }

    // template <typename... Ts>
    // void register_shaders_with_device(Ts... functions) {
    //     this->q.parallel_for({1}, [](std::size_t i) {
             
    //     });
    // }

    void render() {
        // Start the delta timer.
        auto start = std::chrono::high_resolution_clock::now();
        // Make sure all of the objects are in camera coordinates.
        this->q.parallel_for(
            {this->objects.size()},
            [
                objects = this->objects.data(),
                camera_view = this->camera.view
            ](std::size_t i) {
                visit([&](auto& object) { object.obtain_camera_coordinates(*camera_view); }, objects[i]);
            }
        ).wait();
        // Make sure all of the lights are in camera coordinates.
        this->q.parallel_for(
            {this->lights.size()},
            [lights = this->lights.data(), camera_view = this->camera.view](std::size_t i) {
                lights[i].obtain_camera_coordinates(*camera_view);
            }
        ).wait();
        // Draw each pixel.
        for (std::size_t i = 0; i < this->frame_buffer.width * this->frame_buffer.height; ++i) {
            auto data = DeviceData<Object>{
                .view = this->camera.view,
                .film_plane = this->camera.film_plane,
                .rays = this->camera.rays,
                .pixels = this->frame_buffer.pixels,
                .objects = this->objects.data(),
                .object_count = this->objects.size(),
                .lights = this->lights.data(),
                .light_count = this->lights.size()
            };
            Vector3 color = Renderer::illuminate(data, data.rays[i]);
            data.pixels[i] = color;
        }
        // this->q.parallel_for(
        //     { this->frame_buffer.width * this->frame_buffer.height },
        //     [
        //         data = DeviceData<Object>{
        //             .view = this->camera.view,
        //             .film_plane = this->camera.film_plane,
        //             .rays = this->camera.rays,
        //             .pixels = this->frame_buffer.pixels,
        //             .objects = this->objects.data(),
        //             .object_count = this->objects.size(),
        //             .lights = this->lights.data(),
        //             .light_count = this->lights.size()
        //         }
        //     ](std::size_t i) {
        //         Vector3 color = Renderer::illuminate(data, data.rays[i]);
        //         data.pixels[i] = {
        //             static_cast<std::uint8_t>((color.x()) * 255),
        //             static_cast<std::uint8_t>((color.y()) * 255),
        //             static_cast<std::uint8_t>((color.z()) * 255)
        //         };
        //     }
        // ).wait();
        // Calculate delta.
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<Real> delta = end - start;
        std::cout << "Time taken: " << delta.count() << " seconds" << std::endl;
        // Perform per-frame callbacks.
        this->callbacks.on_frame(*this, delta.count());
        // Tone reproduction.
        //this->frame_buffer.tone_reproduction_adaptive_logarithmic_mapping();
        this->frame_buffer.tone_reproduction_ward();
    }

    static Vector3 illuminate(const DeviceData<Object>& data, const Ray& ray, std::size_t depth = 0) {
        // Check if there was a collision.
        if (auto success = Renderer::get_nearest_collision(data, ray)) {
            auto [object, position, normal] = *success;
            MaterialInfo material_info{ .position = position, .normal = normal, .light_position = data.lights[0].camera_position, .light_color = data.lights[0].color }; // TODO: Allow more than one light.
            Vector3 offset_position = position + (0.001 * normal); // TODO: Define epsilon.
            Vector3 shadow_ray_direction = (data.lights[0].camera_position - offset_position);
            Ray shadow_ray{ offset_position, shadow_ray_direction };
            Real distance_to_light = (data.lights[0].camera_position - offset_position).norm();
            Vector3 color;
            if (Renderer::get_nearest_collision(data, shadow_ray, distance_to_light)) {
                // This pixel is in shadow.
                visit([&](const auto& object) {
                    color = { 0, 0, 0 };
                }, *object); // TODO: Use optional reference instead of pointer.
                //color = Renderer::shader_hack(*object, material_info); // TODO: Remove this.
            } else {
                // Update the pixel color corresponding to this ray.
                color = Renderer::shader_hack(*object, material_info);
            }
            // Reflection and transmission.
            Real reflection_constant = visit([](const auto& object) { return object.material.reflection_constant; }, *object);
            Real transmission_constant = visit([](const auto& object) { return object.material.transmission_constant; }, *object);
            Real medium_index = visit([](const auto& object) { return object.material.medium_index; }, *object);
            if (depth > 5) { return color; } // TODO: Make max depth configurable.
            if (reflection_constant > 0) {
                // Perform reflection recursion.
                Ray reflection_ray = {
                    position + (0.001 * normal), // TODO: Define epsilon.
                    ray.direction - 2 * (ray.direction.dot(normal)) * normal
                };
                return (1 - reflection_constant) * color + reflection_constant * Renderer::illuminate(data, reflection_ray, depth + 1);
            }
            if (transmission_constant > 0) {
                // Perform transmission recursion.
                Real eta = 1 / medium_index;
                Vector3 ray_direction = ray.direction;
                if (normal.dot(-ray_direction) < 0) {
                    normal = -normal;
                    eta = 1.0 / eta;
                }
                Real cos_theta_i = -normal.dot(ray_direction);
                Real sin_theta_t_squared = eta * eta * (1.0 - cos_theta_i * cos_theta_i);
                Real cos_theta_t = std::sqrt(1.0 - sin_theta_t_squared);
                if (sin_theta_t_squared > 1.0) {
                    // Total internal reflection
                    Ray total_internal_reflection_ray = {
                        position + (0.001 * normal), // TODO: Define epsilon.
                        ray_direction - 2 * (ray_direction.dot(normal)) * normal
                    };
                    return (1 - transmission_constant) * color + transmission_constant * Renderer::illuminate(data, total_internal_reflection_ray, depth + 1);
                }
                Ray transmission_ray = {
                    position - (0.001 * normal), // TODO: Define epsilon.
                    eta * ray_direction + (eta * cos_theta_i - cos_theta_t) * normal
                };
                return (1 - transmission_constant) * color + transmission_constant * Renderer::illuminate(data, transmission_ray, depth + 1);
            }
            return color;
        } else {
            // If the ray hasn't hit anything, it should display the background color.
            //return this->background_color.data(); // TODO: Implement background_color.
            return { 0, 0, 0 };
        }
    }

    sycl::queue q;

    Shared<Object, SharedAllocator<Object>> objects;
    //KDTreeNode<PhongTriangle> tree;
    Shared<Light, SharedAllocator<Light>> lights;

    FrameBuffer frame_buffer;

    Camera camera;

    Callbacks callbacks;

private:
    static Optional<Tuple<const Object*, Vector3, Vector3>> get_nearest_collision(
        const DeviceData<Object>& data,
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
        for (std::size_t i = 0; i < data.object_count; ++i) {
            const Object& object_variant = data.objects[i];
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

    static Vector3 shader_hack(const Object& object, const MaterialInfo& info) {
        return visit([&](const auto& object) -> Vector3 {
            if constexpr (std::is_same_v<decltype(object), const Sphere&>) {
                return shader::phong(info, (info.normal + Vector3{ 1, 1, 1 }) / 2, Vector3{ 1, 1, 1 }, 0.2, 0.4, 0.4, 10);
            }
            if constexpr (std::is_same_v<decltype(object), const UVTriangle&>) {
                Vector3 cheque_color;
                Vector3 color_a{ 1, 0, 0 };
                Vector3 color_b{ 1, 1, 0 };
                Vector2 check_count{ 30, 30 };
                // Get barycentric coordinate point.
                Vector3 lambda = object.get_barycentric_coordinate(info.position);
                // Get UV from barycentric coordinate point.
                Vector2 uv = object.get_interpolated_attribute(info.position, object.uv);
                uv = uv.cwiseProduct(check_count);
                if (static_cast<std::size_t>(uv[0]) % 2 == static_cast<std::size_t>(uv[1]) % 2) {
                    cheque_color = color_a;
                } else {
                    cheque_color = color_b;
                }
                // Run phong.
                return shader::phong(info, cheque_color, Vector3{1, 1, 1}, 0.2, 0.4, 0.4, 10);
            }
            if constexpr (std::is_same_v<decltype(object), const PhongTriangle&>) {
                return shader::phong(info, (info.normal + Vector3{ 1, 1, 1 }) / 2, Vector3{ 1, 1, 1 }, 0.2, 0.4, 0.4, 10);
            }
            return { 1, 0, 1 }; // Debug failure color.
        }, object);
    }

public:
    void load_ply(std::string_view path) {
        happly::PLYData in(std::string{path});
        std::vector<std::array<double, 3>> vertex_positions = in.getVertexPositions();
        std::vector<std::vector<std::size_t>> face_indices = in.getFaceIndices<std::size_t>();
        for (std::size_t i = 0; i < face_indices.size(); ++i) {

            this->objects.push_back(
                PhongTriangle(
                    Vector3H{ static_cast<Real>(vertex_positions[face_indices[i][0]][0]), static_cast<Real>(vertex_positions[face_indices[i][0]][1]), static_cast<Real>(vertex_positions[face_indices[i][0]][2]), 1_r },
                    Vector3H{ static_cast<Real>(vertex_positions[face_indices[i][1]][0]), static_cast<Real>(vertex_positions[face_indices[i][1]][1]), static_cast<Real>(vertex_positions[face_indices[i][1]][2]), 1_r },
                    Vector3H{ static_cast<Real>(vertex_positions[face_indices[i][2]][0]), static_cast<Real>(vertex_positions[face_indices[i][2]][1]), static_cast<Real>(vertex_positions[face_indices[i][2]][2]), 1_r }
                )
            );
        }
    }
};

#endif