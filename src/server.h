#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast.hpp>
#include <memory>

namespace beast = boost::beast;
namespace http = beast::http;
namespace websocket = beast::websocket;
using tcp = boost::asio::ip::tcp;
using udp = boost::asio::ip::udp;

class server {
public:
    server(boost::asio::io_context& io_context, int port);

private:
    void do_accept();
    void do_read(tcp::socket socket, boost::asio::yield_context yield);
    void do_read_ssl(boost::asio::ssl::stream<tcp::socket> stream, boost::asio::yield_context yield);
    void do_read_udp(boost::asio::yield_context yield);
    void do_session(tcp::socket socket, boost::asio::yield_context yield);

    boost::asio::io_context& io_context_;
    tcp::acceptor acceptor_;
    udp::socket udp_socket_;
    boost::asio::ssl::context ssl_context_;
};

#endif // SERVER_H
