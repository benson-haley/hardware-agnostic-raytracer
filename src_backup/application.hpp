#ifndef GI_BAH8454_APPLICATION
#define GI_BAH8454_APPLICATION

#include <thread>
#include <cstdint>
#include <optional>

#include "web_socket_server.hpp"

template<RenderableObject... ObjectTypes>
class Application {
public:
	std::jthread launch_web(std::uint16_t port, const Renderer<ObjectTypes...>::Info& renderer_info) {
		this->web_socket_server.emplace(port, renderer_info);
		return std::jthread{ [this]() { this->web_socket_server->run(); } };
	}

private:
	std::optional<WebSocketServer<ObjectTypes...>> web_socket_server;
};

#endif