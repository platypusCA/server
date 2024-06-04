#include "server.h"
#include <boost/program_options.hpp>
#include <iostream>

int main(int argc, char* argv[]) {
    try {
        boost::program_options::options_description desc{"Options"};
        desc.add_options()
            ("help,h", "Help screen")
            ("port,p", boost::program_options::value<int>()->default_value(8080), "Listening port");

        boost::program_options::variables_map vm;
        boost::program_options::store(parse_command_line(argc, argv, desc), vm);
        boost::program_options::notify(vm);

        if (vm.count("help")) {
            std::cout << desc << std::endl;
            return 0;
        }

        int port = vm["port"].as<int>();

        boost::asio::io_context io_context;
        server s(io_context, port);
        io_context.run();
    }
    catch (const std::exception& ex) {
        std::cerr << "Error: " << ex.what() << std::endl;
    }

    return 0;
}
