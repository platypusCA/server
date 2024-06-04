add_executable(server main.cpp server.cpp)

target_link_libraries(server PRIVATE Boost::program_options Boost::system Boost::asio Boost::filesystem OpenSSL::SSL OpenSSL::Crypto)
