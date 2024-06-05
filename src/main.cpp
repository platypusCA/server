#include "server.h"
#include <boost/program_options.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        boost::program_options::options_description desc{"Options"};
        desc.add_options()
                ("help,h", "Help screen")
                ("http_port", boost::program_options::value<int>()->default_value(8080), "HTTP port")
                ("tcp_port", boost::program_options::value<int>()->default_value(8081), "TCP port")
                ("udp_port", boost::program_options::value<int>()->default_value(8082), "UDP port");

        boost::program_options::variables_map vm;
        boost::program_options::store(parse_command_line(argc, argv, desc), vm);
        boost::program_options::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        int http_port = vm["http_port"].as<int>();
        int tcp_port = vm["tcp_port"].as<int>();
        int udp_port = vm["udp_port"].as<int>();

        std::cout << "Server starting with the following parameters:" << std::endl;
        std::cout << "HTTP Port: " << http_port << std::endl;
        std::cout << "TCP Port: " << tcp_port << std::endl;
        std::cout << "UDP Port: " << udp_port << std::endl;

        boost::asio::io_context io_context;
        server s(io_context, http_port, tcp_port, udp_port);
        io_context.run();
    } catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    return 0;
}
