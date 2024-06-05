#include "server.h"
#include <iostream>
#include <array>
#include <boost/asio/spawn.hpp>

server::server(boost::asio::io_context& io_context, int http_port, int tcp_port, int udp_port)
        : io_context_(io_context),
          http_acceptor_(io_context, tcp::endpoint(tcp::v4(), http_port)),
          tcp_acceptor_(io_context, tcp::endpoint(tcp::v4(), tcp_port)),
          udp_socket_(io_context, udp::endpoint(udp::v4(), udp_port)) {
    do_accept_http();
    do_accept_tcp();
    boost::asio::spawn(io_context_, [this](boost::asio::yield_context yield) {
        do_read_udp(yield);
    });
}

void server::do_accept_http() {
    boost::asio::spawn(io_context_, [this](boost::asio::yield_context yield) {
        while (true) {
            boost::system::error_code ec;
            tcp::socket socket(io_context_);
            http_acceptor_.async_accept(socket, yield[ec]);
            if (!ec) {
                boost::asio::spawn(io_context_, [this, socket = std::move(socket)](boost::asio::yield_context yield) mutable {
                    handle_http(std::move(socket), yield);
                });
            }
        }
    });
}

void server::do_accept_tcp() {
    boost::asio::spawn(io_context_, [this](boost::asio::yield_context yield) {
        while (true) {
            boost::system::error_code ec;
            tcp::socket socket(io_context_);
            tcp_acceptor_.async_accept(socket, yield[ec]);
            if (!ec) {
                boost::asio::spawn(io_context_, [this, socket = std::move(socket)](boost::asio::yield_context yield) mutable {
                    handle_tcp(std::move(socket), yield);
                });
            }
        }
    });
}

void server::handle_tcp(tcp::socket socket, boost::asio::yield_context yield) {
    try {
        std::array<char, 1024> buf{};
        boost::system::error_code ec;
        for (;;) {
            std::size_t length = socket.async_read_some(boost::asio::buffer(buf), yield[ec]);
            if (ec == boost::asio::error::eof || ec == boost::asio::error::connection_reset) {
                break;
            } else if (ec) {
                throw boost::system::system_error(ec);
            }

            std::string message(buf.data(), length);
            std::cout << "TCP: " << message << std::endl;

            boost::asio::async_write(socket, boost::asio::buffer(message), yield[ec]);
            if (ec) {
                throw boost::system::system_error(ec);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in TCP: " << e.what() << std::endl;
    }
}

void server::do_read_udp(boost::asio::yield_context yield) {
    while (true) {
        std::array<char, 1024> buf{};
        udp::endpoint sender_endpoint;
        boost::system::error_code ec;

        std::size_t length = udp_socket_.async_receive_from(boost::asio::buffer(buf), sender_endpoint, yield[ec]);
        if (!ec) {
            std::string message(buf.data(), length);
            std::cout << "UDP: " << message << std::endl;

            udp_socket_.async_send_to(boost::asio::buffer(message), sender_endpoint, yield[ec]);
            if (ec) {
                std::cerr << "Error sending UDP: " << ec.message() << std::endl;
            }
        } else {
            std::cerr << "Error receiving UDP: " << ec.message() << std::endl;
        }
    }
}

void server::handle_http(tcp::socket socket, boost::asio::yield_context yield) {
    try {
        beast::flat_buffer buffer;
        http::request<http::string_body> req;
        boost::system::error_code ec;

        http::async_read(socket, buffer, req, yield[ec]);
        if (ec == boost::asio::error::eof || ec == beast::http::error::end_of_stream) {
            return;
        } else if (ec) {
            throw boost::system::system_error(ec);
        }

        std::string message = req.body();
        std::cout << "HTTP Request: " << message << std::endl;

        http::response<http::string_body> res{http::status::ok, req.version()};
        res.set(http::field::server, "Boost.Beast/1.1");
        res.set(http::field::content_type, "text/plain");
        res.keep_alive(req.keep_alive());
        res.body() = message;  // Echo back the received message
        res.prepare_payload();

        http::async_write(socket, res, yield[ec]);
        if (ec) {
            throw boost::system::system_error(ec);
        }

        if (!res.keep_alive()) {
            socket.shutdown(tcp::socket::shutdown_send, ec);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in HTTP: " << e.what() << std::endl;
    }
}
