#include <iostream>
#include <execution>
#include <algorithm>

#include "application.hpp"

auto on_load = [] <RenderableObject... ObjectTypes> (Renderer<ObjectTypes...>&self) {
    // Constants.
    auto cheque = [](const UVTriangle& object, const MaterialInfo& info, Vector2 check_count, Vector3 color_a, Vector3 color_b) {
        // Get barycentric coordinate point.
        Vector3 lambda = object.get_barycentric_coordinate(info.position);
        // Get UV from barycentric coordinate point.
        Vector2 uv = object.get_interpolated_attribute(info.position, object.uv);
        uv = uv.cwiseProduct(check_count);
        if (static_cast<std::size_t>(uv[0]) % 2 == static_cast<std::size_t>(uv[1]) % 2) {
            return color_a;
        } else {
            return color_b;
        }
    };

    Material<UVTriangle> triangle_material{
        [&](const UVTriangle& object, const MaterialInfo& info) {
            return shader::phong(info, cheque(object, info, { 30, 30 }, { 1, 0, 0 }, { 1, 1, 0 }), Vector3{1, 1, 1}, 0.2, 0.4, 0.4, 10);
        },
        [&](const UVTriangle& object, const MaterialInfo& info) {
            return shader::phong_shadow(info, cheque(object, info, { 30, 30 }, { 1, 0, 0 }, { 1, 1, 0 }), 0.2);
        }
    };

    self.objects.push_back(
        Sphere(
            { 0, 0, 0, 1 }, 0.5,
            {
                0.0,
                [](const Sphere& object, const MaterialInfo& info) {
                    return shader::phong(info, (info.normal + Vector3{ 1, 1, 1 }) / 2, Vector3{ 1, 1, 1 }, 0.2, 0.4, 0.4, 10);
                },
                [](const Sphere& object, const MaterialInfo& info) {
                    return shader::phong_shadow(info, (info.normal + Vector3{ 1, 1, 1 }) / 2, 0.2);
                }
            }
        )
    );
    self.objects.push_back(
        Sphere(
            { 0.75, -0.5, -0.75, 1 }, 0.5,
            {
                1.0,
                [](const Sphere& object, const MaterialInfo& info) {
                    return shader::phong(info, Vector3{ 1, 1, 1 } - (info.normal + Vector3{ 1, 1, 1 }) / 2, Vector3{ 1, 1, 1 }, 0.2, 0.4, 0.4, 10);
                },
                [](const Sphere& object, const MaterialInfo& info) {
                    return shader::phong_shadow(info, Vector3{ 1, 1, 1 } - (info.normal + Vector3{ 1, 1, 1 }) / 2, 0.2);
                }
            }
        )
    );
    self.objects.push_back(
        UVTriangle(
            { Vector3H{ -6, -1.25, 6, 1 }, Vector3H{ -6, -1.25, -6, 1 }, Vector3H{ 6, -1.25, -6, 1 } },
            { Vector2{ 0, 0 }, Vector2{ 0, 1 }, Vector2{ 1, 1 } },
            triangle_material
        )
    );
    self.objects.push_back(
        UVTriangle(
            { Vector3H{ 6, -1.25, -6, 1 }, Vector3H{ 6, -1.25, 6, 1 }, Vector3H{ -6, -1.25, 6, 1 } },
            { Vector2{ 1, 1 }, Vector2{ 1, 0 }, Vector2{ 0, 0 } },
            triangle_material
        )
    );
};

int main() {
    // for (const auto& platform : sycl::platform::get_platforms()) {
    //     std::cout << "Platform: " << platform.get_info<sycl::info::platform::name>() << std::endl;
        
    //     for (const auto& device : platform.get_devices()) {
    //         std::cout << "  Device: " << device.get_info<sycl::info::device::name>() << std::endl;
    //     }
    // }

    // Renderer<Sphere, UVTriangle> renderer{{
    //     .frame_buffer = { .width = 1024, .height = 768 },
    //     .camera = {
    //         .position = { 0, 0, 2 },
    //         .center = { 0, 0, -1 },
    //         .up = { 0, 1, 0 }
    //     },
    //     .callbacks = {
    //         .on_load = on_load
    //     }
    // }};

    // renderer.render();

    // return 0;

    try {
        Application<Sphere, UVTriangle> app{};
        auto thread = app.launch_web(
            8080,
            {
                .frame_buffer = { .width = 1024, .height = 768 },
                .camera = {
                    .position = { 0, 0, 2 },
                    .center = { 0, 0, -1 },
                    .up = { 0, 1, 0 }
                },
                .callbacks = {
                    .on_load = on_load
                }
            }
        );
    } catch (const std::exception& e) {
        // Print errors to std::cerr if an exception is thrown.
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
