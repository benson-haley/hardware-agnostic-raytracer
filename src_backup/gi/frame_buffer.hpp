#ifndef GI_BAH8454_FRAME_BUFFER
#define GI_BAH8454_FRAME_BUFFER

// Include SYCL.
#include <sycl/sycl.hpp>

#include <cstdint>
#include <bit>
#include <span>

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
		this->pixels = sycl::malloc_shared<Pixel>(sizeof(Pixel) * this->width * this->height, q);
	}

	FrameBuffer(const FrameBuffer&) = delete;

	~FrameBuffer() {
		sycl::free(this->pixels, this->q);
	}

	Pixel& operator[](std::size_t i) {
		return this->pixels[i];
	}

	std::span<const std::byte> get_bytes() {
		return { std::bit_cast<std::byte*>(this->pixels), width * height * sizeof(Pixel) };
	}

	std::size_t width;
	std::size_t height;

private:
	sycl::queue& q;
	Pixel* pixels;

};

#endif