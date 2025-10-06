#include <iostream>
#include <execution>
#include <algorithm>

#include "application.hpp"

#include <sycl/sycl.hpp>

auto on_load = [] <RenderableObject... ObjectTypes> (Renderer<ObjectTypes...>& self) {
    self.objects.push_back(
        Sphere(
            { 0, 0, 0, 1 }, 0.5,
            { 0, 0.8, 0.95 }
        )
    );
    self.objects.push_back(
        Sphere(
            { 0.75, -0.5, -1.1, 1 }, 0.5,
            { 1, 0, 1 }
        )
    );
    self.objects.push_back(
        UVTriangle(
            { Vector3H{ -6, -1.25, 6, 1 }, Vector3H{ -6, -1.25, -6, 1 }, Vector3H{ 6, -1.25, -6, 1 } },
            { Vector2{ 0, 0 }, Vector2{ 0, 1 }, Vector2{ 1, 1 } },
            { }
        )
    );
    self.objects.push_back(
        UVTriangle(
            { Vector3H{ 6, -1.25, -6, 1 }, Vector3H{ 6, -1.25, 6, 1 }, Vector3H{ -6, -1.25, 6, 1 } },
            { Vector2{ 1, 1 }, Vector2{ 1, 0 }, Vector2{ 0, 0 } },
            { }
        )
    );

    self.lights.push_back(
        Light{ Vector3H{ 0, 1, 2, 1 } }
    );

    // self.load_ply("/mnt/c/Users/bah/Documents/RIT/Semester 7/GI/gi/src/ply/bun_zipper_res2.ply");
};

auto on_frame = [direction = true, speed = 1] <RenderableObject... ObjectTypes> (Renderer<ObjectTypes...>& self, Real delta) mutable {
    // visit([&](auto& object) {
    //     if constexpr (std::is_same_v<decltype(object), Sphere&>) {
    //         if (object.world_position[0] > 2) {
    //             direction = false;
    //         } else if (object.world_position[0] < -2) {
    //             direction = true;
    //         }

    //         if (direction) {
    //             object.world_position[0] += speed * delta;
    //         } else {
    //             object.world_position[0] -= speed * delta;
    //         }
    //     }
    // }, self.objects[0]);
};

int main() {
    // for (const auto& platform : sycl::platform::get_platforms()) {
    //     std::cout << "Platform: " << platform.get_info<sycl::info::platform::name>() << std::endl;
        
    //     for (const auto& device : platform.get_devices()) {
    //         std::cout << "  Device: " << device.get_info<sycl::info::device::name>() << std::endl;
    //     }
    // }

    try {
        Application<Sphere, UVTriangle/*, PhongTriangle*/> app{};
        auto thread = app.launch_web(
            8080,
            {
                .frame_buffer = { .width = 1024, .height = 768 },
                .camera = {
                    .position = { 0, 0, 2 },
                    //.position = { 0, 0.5, 0.3 },
                    .center = { 0, 0, -1 },
                    .up = { 0, 1, 0 }
                },
                .callbacks = {
                    .on_load = on_load,
                    .on_frame = on_frame
                }
            }
        );
    } catch (const std::exception& e) {
        // Print errors to std::cerr if an exception is thrown.
        std::cerr << e.what() << std::endl;
    }
    return 0;
}
