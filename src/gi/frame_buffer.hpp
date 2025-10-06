#ifndef GI_BAH8454_FRAME_BUFFER
#define GI_BAH8454_FRAME_BUFFER

// Include SYCL.
#include <sycl/sycl.hpp>

#include <cstdint>
#include <cmath>
#include <bit>
#include <span>
#include <algorithm>

#include "util.hpp"

class Pixel {
public:
	Pixel() : r{ 0 }, g{ 0 }, b{ 0 }, a{ 255 } {}
	Pixel(std::uint8_t r, std::uint8_t g, std::uint8_t b, std::uint8_t a = 255) : r{ r }, g{ g }, b{ b }, a{ a } {}

	std::uint8_t r;
	std::uint8_t g;
	std::uint8_t b;
	std::uint8_t a;
};

class FrameBuffer {
public:
	class Info {
	public:
		std::size_t width;
		std::size_t height;
	};

	FrameBuffer(sycl::queue& q, Info info) : q{ q }, width{ info.width }, height{ info.height } {
		this->pixels = sycl::malloc_shared<Vector3>(sizeof(Vector3) * this->width * this->height, q);
		this->illuminances = sycl::malloc_shared<Real>(sizeof(Real) * this->width * this->height, q);
		this->rgba_pixels = sycl::malloc_shared<Pixel>(sizeof(Pixel) * this->width * this->height, q);
	}

	FrameBuffer(const FrameBuffer&) = delete;

	~FrameBuffer() {
		sycl::free(this->pixels, this->q);
		sycl::free(this->illuminances, this->q);
		sycl::free(this->rgba_pixels, this->q);
	}

	void tone_reproduction_none() {
		for (std::size_t i = 0; i < this->width * this->height; ++i) {
			this->rgba_pixels[i].r = static_cast<std::uint8_t>(this->pixels[i][0] * 255);
			this->rgba_pixels[i].g = static_cast<std::uint8_t>(this->pixels[i][1] * 255);
			this->rgba_pixels[i].b = static_cast<std::uint8_t>(this->pixels[i][2] * 255);
			this->rgba_pixels[i].a = 255;
		}
	}

	void tone_reproduction_ward() {
		for (std::size_t i = 0; i < this->width * this->height; ++i) {
			this->illuminances[i] = absolute_illuminance(this->pixels[i]);
		}
		//Real max_illuminance = *std::max_element(this->illuminances, this->illuminances + (this->width * this->height));
		Real max_illuminance = 10;
		Real sum_of_log_illuminances = 0;
		for (std::size_t i = 0; i < this->width * this->height; ++i) {
			sum_of_log_illuminances += std::log(0.000001_r + this->illuminances[i]); // TODO: Epsilon.
		}
		Real log_average_luminance = std::exp((1_r / (this->width * this->height)) * sum_of_log_illuminances);
		//std::cout << "log average luminance " << log_average_luminance << std::endl;
		//std::cout << "SUM OF Log illuminances " << sum_of_log_illuminances << std::endl;
		Real sf = std::pow(((1.219_r + std::pow(max_illuminance / 2, 0.4)) / (1.219 + std::pow(log_average_luminance, 0.4))), 2.5);
		std::cout << "sf = " << sf << std::endl;
		for (std::size_t i = 0; i < this->width * this->height; ++i) {
			//std::cout << this->pixels[i][0] / max_illuminance /** this->illuminances[i]*/ * sf * 255 << std::endl;
			this->rgba_pixels[i].r = static_cast<std::uint8_t>(this->pixels[i][0] / max_illuminance /** this->illuminances[i]*/ * sf * 255);
			this->rgba_pixels[i].g = static_cast<std::uint8_t>(this->pixels[i][1] / max_illuminance /** this->illuminances[i]*/ * sf * 255);
			this->rgba_pixels[i].b = static_cast<std::uint8_t>(this->pixels[i][2] / max_illuminance /** this->illuminances[i]*/ * sf * 255);
			this->rgba_pixels[i].a = 255;
		}
	}

	void tone_reproduction_reinhard() {
		for (std::size_t i = 0; i < this->width * this->height; ++i) {
			this->illuminances[i] = absolute_illuminance(this->pixels[i]);
		}
		//Real max_illuminance = *std::max_element(this->illuminances, this->illuminances + (this->width * this->height));
		Real max_illuminance = 1;
		Real sum_of_log_illuminances = 0;
		for (std::size_t i = 0; i < this->width * this->height; ++i) {
			sum_of_log_illuminances += std::log(0.000001_r + this->illuminances[i]); // TODO: Epsilon.
		}
		Real log_average_luminance = std::exp((1_r / (this->width * this->height)) * sum_of_log_illuminances);
		for (std::size_t i = 0; i < this->width * this->height; ++i) {
			Vector3 rgb_s = (0.18_r / log_average_luminance) * this->pixels[i];
			Vector3 rgb_r = rgb_s.cwiseQuotient(Vector3{ 1, 1, 1 } + rgb_s);
			Vector3 rgb_target = rgb_r * max_illuminance;
			this->rgba_pixels[i].r = static_cast<std::uint8_t>(rgb_target[0] / max_illuminance * 255);
			this->rgba_pixels[i].g = static_cast<std::uint8_t>(rgb_target[1] / max_illuminance * 255);
			this->rgba_pixels[i].b = static_cast<std::uint8_t>(rgb_target[2] / max_illuminance * 255);
			this->rgba_pixels[i].a = 255;
		}
	}

	void tone_reproduction_adaptive_logarithmic_mapping() {
		Real sum_of_log_illuminances = 0;
		for (std::size_t i = 0; i < this->width * this->height; ++i) {
			this->illuminances[i] = absolute_illuminance(this->pixels[i]);
			sum_of_log_illuminances += std::log(0.000001_r + this->illuminances[i]); // TODO: Epsilon.
		}
		Real log_average_luminance = std::exp((1_r / (this->width * this->height)) * sum_of_log_illuminances);
		Real l_w_max = *std::max_element(this->illuminances, this->illuminances + (this->width * this->height));
		Real l_d_max = 1;
		Real b = 0.85;
		l_w_max /= log_average_luminance;
		for (std::size_t i = 0; i < this->width * this->height; ++i) {
			Real l_w = this->illuminances[i];
			l_w /= log_average_luminance;
			Real l_d = (1 / std::log10(l_w_max + 1)) * (std::log(l_w + 1) / log(2 + (std::pow(l_w / l_w_max , std::log(b) / std::log(0.5))) * 8));
			this->rgba_pixels[i].r = static_cast<std::uint8_t>(this->pixels[i][0] * l_d / l_d_max * 255);
			this->rgba_pixels[i].g = static_cast<std::uint8_t>(this->pixels[i][1] * l_d / l_d_max * 255);
			this->rgba_pixels[i].b = static_cast<std::uint8_t>(this->pixels[i][2] * l_d / l_d_max * 255);
			this->rgba_pixels[i].a = 255;
		}
	}

	Vector3& operator[](std::size_t i) {
		return this->pixels[i];
	}

	std::span<const std::byte> get_bytes() {
		return { std::bit_cast<std::byte*>(this->rgba_pixels), width * height * sizeof(Pixel) };
	}

	std::size_t width;
	std::size_t height;

	sycl::queue& q;
	Vector3* pixels;
	Real* illuminances;
	Pixel* rgba_pixels;

};

#endif