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
            std::array<char, 1024> buf{};
            boost::system::error_code ec;
            std::size_t length = socket.async_read_some(boost::asio::buffer(buf), yield[ec]);
            if (ec == boost::asio::error::eof){
                break;
            }
            else if (ec) {
                throw boost::system::system_error(ec);
            }

            std::cout << "TCP: " << std::string(buf.data(), length) << std::endl;

            // Echo the message back to the sender
            boost::asio::async_write(socket, boost::asio::buffer(buf, length), yield[ec]);
            if (ec) {
                throw boost::system::system_error(ec);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in TCP: " << e.what() << std::endl;
    }
}

void server::do_read_ssl(boost::asio::ssl::stream<tcp::socket> stream, boost::asio::yield_context yield) {
    try {
        stream.async_handshake(boost::asio::ssl::stream_base::server, yield);
        for (;;) {
            std::array<char, 1024> buf{};
            boost::system::error_code ec;
            std::size_t length = stream.async_read_some(boost::asio::buffer(buf), yield[ec]);
            if (ec == boost::asio::error::eof){
                break;
            }
            else if (ec){
                throw boost::system::system_error(ec);
            }
            std::cout << "TCP/SSL: " << std::string(buf.data(), length) << std::endl;

            // Echo the message back to the sender
            boost::asio::async_write(stream, boost::asio::buffer(buf, length), yield[ec]);
            if (ec) {
                throw boost::system::system_error(ec);
            }
        }
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

void server::do_session(tcp::socket socket, boost::asio::yield_context yield) {
    try {
        beast::flat_buffer buffer;
        beast::websocket::stream<tcp::socket> ws{std::move(socket)};

        http::request<http::string_body> req;
        boost::system::error_code ec;
        http::async_read(ws.next_layer(), buffer, req, yield[ec]);

        if (websocket::is_upgrade(req)) {
            ws.async_accept(req, yield[ec]);
            for (;;) {
                beast::flat_buffer ws_buffer;
                ws.async_read(ws_buffer, yield[ec]);
                std::cout << "WebSocket: " << beast::make_printable(ws_buffer.data()) << std::endl;
                ws.async_write(ws_buffer.data(), yield[ec]);
            }
        } else {
            http::response<http::string_body> res{http::status::ok, req.version()};
            res.set(http::field::server, "Boost.Beast/1.0");
            res.set(http::field::content_type, "text/plain");
            res.keep_alive(req.keep_alive());
            res.body() = "Hello, HTTP!";
            res.prepare_payload();
            http::async_write(ws.next_layer(), res, yield[ec]);
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in session: " << e.what() << std::endl;
    }
}
