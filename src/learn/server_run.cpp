#include "server.hpp"

#include <cstdlib>
#include <iostream>
#include <memory>
#include <string>


int main()
{
    std::string     server_address("0.0.0.0:50051");
    ChatServiceImpl service;

    grpc::ServerBuilder builder;
    builder.AddChannelArgument(GRPC_ARG_ALLOW_REUSEPORT, 0);
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    if (!server) {
        std::cerr << "Could not create the server!\n";
        return EXIT_FAILURE;
    }

    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}
