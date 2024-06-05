#ifndef SERVER_H
#define SERVER_H

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/spawn.hpp>
#include <boost/beast.hpp>
#include <memory>

namespace beast = boost::beast;
namespace http = beast::http;
using tcp = boost::asio::ip::tcp;
using udp = boost::asio::ip::udp;

class server {
public:
    server(boost::asio::io_context& io_context, int http_port, int tcp_port, int udp_port);

private:
    void do_accept_http();
    void do_accept_tcp();
    void handle_tcp(tcp::socket socket, boost::asio::yield_context yield);
    void do_read_udp(boost::asio::yield_context yield);
    void handle_http(tcp::socket socket, boost::asio::yield_context yield);

    boost::asio::io_context& io_context_;
    tcp::acceptor http_acceptor_;
    tcp::acceptor tcp_acceptor_;
    udp::socket udp_socket_;
};

#endif // SERVER_H
