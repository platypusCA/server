#include "server.h"
#include <iostream>
#include <array>
#include <boost/asio/spawn.hpp>

server::server(boost::asio::io_context& io_context, int port)
        : io_context_(io_context),
          acceptor_(io_context, tcp::endpoint(tcp::v4(), port)),
          udp_socket_(io_context, udp::endpoint(udp::v4(), port)),
          ssl_context_(boost::asio::ssl::context::sslv23)
{
    // SSL context configuration
    ssl_context_.set_options(
            boost::asio::ssl::context::default_workarounds |
            boost::asio::ssl::context::no_sslv2 |
            boost::asio::ssl::context::single_dh_use);

    do_accept();
    boost::asio::spawn(io_context_, [this](boost::asio::yield_context yield) {
        do_read_udp(yield);
    });
}

void server::do_accept() {
    boost::asio::spawn(io_context_, [this](boost::asio::yield_context yield) {
        while (true) {
            boost::system::error_code ec;
            tcp::socket socket(io_context_);
            acceptor_.async_accept(socket, yield[ec]);
            if (!ec) {
                boost::asio::spawn(io_context_, [this, socket = std::move(socket)](boost::asio::yield_context yield) mutable {
                    do_read(std::move(socket), yield);
                });
            }
        }
    });
}

void server::do_read(tcp::socket socket, boost::asio::yield_context yield) {
    try {
        for (;;) {
            beast::flat_buffer buffer;
            http::request<http::string_body> req;
            boost::system::error_code ec;

            http::async_read(socket, buffer, req, yield[ec]);
            if (ec == boost::asio::error::eof || ec == beast::http::error::end_of_stream) {
                break; // Gracefully handle the end of stream
            } else if (ec) {
                throw boost::system::system_error(ec);
            }

            std::cout << "HTTP Request: " << req.body() << std::endl;

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::server, "Boost.Beast/1.1");
            res.set(http::field::content_type, "text/plain");
            res.keep_alive(req.keep_alive());
            res.body() = req.body();  // Echo back the received message
            res.prepare_payload();

            http::async_write(socket, res, yield[ec]);
            if (ec) {
                throw boost::system::system_error(ec);
            }

            if (!res.keep_alive()) {
                break; // Close the socket if keep-alive is not requested
            }
        }

        socket.shutdown(tcp::socket::shutdown_send);
    } catch (const std::exception& e) {
        std::cerr << "Error in TCP: " << e.what() << std::endl;
    }
}

void server::do_read_ssl(boost::asio::ssl::stream<tcp::socket> stream, boost::asio::yield_context yield) {
    try {
        stream.async_handshake(boost::asio::ssl::stream_base::server, yield);
        for (;;) {
            beast::flat_buffer buffer;
            http::request<http::string_body> req;
            boost::system::error_code ec;

            http::async_read(stream, buffer, req, yield[ec]);
            if (ec == boost::asio::error::eof || ec == beast::http::error::end_of_stream) {
                break; // Gracefully handle the end of stream
            } else if (ec) {
                throw boost::system::system_error(ec);
            }

            std::cout << "HTTP/SSL Request: " << req.body() << std::endl;

            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::server, "Boost.Beast/1.1");
            res.set(http::field::content_type, "text/plain");
            res.keep_alive(req.keep_alive());
            res.body() = req.body();  // Echo back the received message
            res.prepare_payload();

            http::async_write(stream, res, yield[ec]);
            if (ec) {
                throw boost::system::system_error(ec);
            }

            if (!res.keep_alive()) {
                break; // Close the socket if keep-alive is not requested
            }
        }

        stream.lowest_layer().shutdown(tcp::socket::shutdown_send);
    } catch (const std::exception& e) {
        std::cerr << "Error in TCP/SSL: " << e.what() << std::endl;
    }
}

void server::do_read_udp(boost::asio::yield_context yield) {
    while (true) {
        std::array<char, 1024> buf{};
        udp::endpoint sender_endpoint;
        boost::system::error_code ec;

        // Receive data from the client
        std::size_t length = udp_socket_.async_receive_from(boost::asio::buffer(buf), sender_endpoint, yield[ec]);
        if (!ec) {
            std::cout << "UDP: " << std::string(buf.data(), length) << std::endl;

            // Echo the message back to the sender
            udp_socket_.async_send_to(boost::asio::buffer(buf, length), sender_endpoint, yield[ec]);
            if (ec) {
                std::cerr << "Error sending UDP: " << ec.message() << std::endl;
            }
        } else {
            std::cerr << "Error receiving UDP: " << ec.message() << std::endl;
        }
    }
}
