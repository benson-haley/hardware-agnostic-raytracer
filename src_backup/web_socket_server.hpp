#ifndef GI_BAH8454_WEB_SOCKET_SERVER
#define GI_BAH8454_WEB_SOCKET_SERVER

#include <cstdint>
#include <iostream>

// Include boost libraries.
#include <boost/asio.hpp>
#include <boost/beast.hpp>
#include <boost/beast/websocket.hpp>
// Simplify boost namespaces.
namespace asio = boost::asio;
namespace beast = boost::beast;

#include "gi/renderer.hpp"

/// @brief Represents a web socket server, creating sessions when clients connect to the server.
template <RenderableObject... ObjectTypes>
class WebSocketServer {
public:
    WebSocketServer(std::uint16_t port, const Renderer<ObjectTypes...>::Info& renderer_info) :
        io_context{},
        acceptor{ this->io_context, asio::ip::tcp::endpoint{ asio::ip::tcp::v4(), port } },
        renderer_info{ renderer_info }
    {
        listen();
    }

    /// @brief Calls the internal asio::io_context's run method.
    void run() {
        this->io_context.run();
    }

private:
    /// @brief Called within the constructor to begin asynchronously listening for new connections.
    void listen() {
        // Asynchronously listen for a new connection.
        this->acceptor.async_accept([this](boost::system::error_code error, asio::ip::tcp::socket socket) {
            // If there hasn't been an error yet, create a new session and start it.
            if (!error) {
                // This shared pointer won't die until we stop listening for new connections (which will never happen until we kill the program).
                std::make_shared<Session>(std::move(socket), io_context, this->renderer_info)->start();
            }
            // Continue listening.
            this->listen();
        });
    }

    class Session : public std::enable_shared_from_this<Session> {
    public:
        Session(asio::ip::tcp::socket socket, asio::io_context& io_context, const Renderer<ObjectTypes...>::Info& renderer_info) :
            ws{ std::move(socket) },
            io_context{ io_context },
            renderer{ renderer_info }
        {}

        void start() {
            // Make sure the web socket sends binary frames.
            this->ws.binary(true);
            // Perform code the user wants run before the session starts.
            this->renderer.callbacks.on_load(this->renderer);
            // Start the session.
            this->ws.async_accept([self = this->shared_from_this()](boost::system::error_code error) {
                if (!error) {
                    self->send_frame();
                    self->read();
                }
            });
        }

    private:
        void read() {
            this->ws.async_read(this->buffer, [self = this->shared_from_this()](boost::system::error_code error, std::size_t) {
                if (!error) {
                    std::string message = beast::buffers_to_string(self->buffer.data());
                    if (message == "move_forward") {
                        self->renderer.camera.translate(self->renderer.camera.get_forward_vector(), -0.01);
                    }
                    if (message == "move_backward") {
                        self->renderer.camera.translate(self->renderer.camera.get_forward_vector(), 0.01);
                    }
                    if (message == "move_up") {
                        self->renderer.camera.translate(self->renderer.camera.get_up_vector(), 0.01);
                    }
                    if (message == "move_down") {
                        self->renderer.camera.translate(self->renderer.camera.get_up_vector(), -0.01);
                    }
                    self->buffer.clear();
                    self->read();
                }
            });
        }

        void send_frame() {
            this->ws.async_write(asio::buffer(this->renderer.frame_buffer.get_bytes()), [this](boost::system::error_code error, std::size_t) {
                if (!error) {
                    this->renderer.render();
                    this->send_frame();
                }
            });
        }

        beast::websocket::stream<asio::ip::tcp::socket> ws;
        asio::io_context& io_context;
        Renderer<ObjectTypes...> renderer;

        beast::multi_buffer buffer;
    };

    asio::io_context io_context;
    asio::ip::tcp::acceptor acceptor;
    Renderer<ObjectTypes...>::Info renderer_info;
};

#endif