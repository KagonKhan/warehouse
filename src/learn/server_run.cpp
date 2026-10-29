#include "server.hpp"


int main()
{
    std::string server_address("0.0.0.0:50051");
    ChatService service;

    grpc::ServerBuilder builder;
    builder.AddListeningPort(server_address, grpc::InsecureServerCredentials());
    builder.RegisterService(&service);

    std::unique_ptr<grpc::Server> server = builder.BuildAndStart();
    if (!server) {
        std::cout << "Could not create the server!";
        return -1;
    }

    std::cout << "Server listening on " << server_address << std::endl;
    server->Wait();
}
