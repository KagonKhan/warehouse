#include "learn.grpc.pb.h"
#include "learn.pb.h"

#include <google/protobuf/util/time_util.h>
#include <grpcpp/create_channel.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/security/credentials.h>
#include <chrono>


#include <thread>


std::string format_chat_time(
    const google::protobuf::Timestamp& timestamp)
{
    using namespace std::chrono;

    const auto point =
        sys_time<nanoseconds>{
        seconds {timestamp.seconds()} +
        nanoseconds {timestamp.nanos()}
    };

    const auto     day = floor<days>(point);
    const hh_mm_ss time {point - day};

    return std::format(
        "{:02}:{:02}:{:02}",
        time.hours().count(),
        time.minutes().count(),
        time.seconds().count()
    );
}

int main()
{
    auto channel      = grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials());
    auto chat_service = chat::ChatService::NewStub(channel);

    grpc::ClientContext    subscribe_context;
    chat::SubscribeRequest subscribe_request;
    subscribe_request.set_room_id("default");

    auto        reader = chat_service->Subscribe(&subscribe_context, subscribe_request);
    std::thread receive_thread([&] {
            chat::Message message;

            while (reader->Read(&message)) {
                std::cout << "[" << format_chat_time(message.timestamp()) << "] "
                          << message.sender_id() << ": " << message.text()
                          << std::endl;
            }

            grpc::Status s = reader->Finish();
        });


    std::cout << "Who are you?: ";
    std::string username;
    std::cin >> username;

    std::string line;
    while (true) {
        system("clear");
        std::cout << "Room: default" << " | User: " << username << '\n'
                  << "Type messages; use /quit to exit.\n"
                  << "\n------------------------------------------------\n"
                  << std::flush;

        grpc::ClientContext get_context;
        get_context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(2));
        chat::GetMessagesRequest  get_request;
        chat::GetMessagesResponse get_response;
        grpc::Status              get_status = chat_service->GetMessages(&get_context, get_request, &get_response);

        if (get_status.ok()) {
            for (auto const& message : get_response.messages()) {
                std::cout << "[" << format_chat_time(message.timestamp()) << "] "
                          << message.sender_id() << ": " << message.text()
                          << std::endl;
            }
        }

        std::cout << "\n------------------------------------------------\n" << std::flush;
        if (!std::getline(std::cin, line)) {
            break;
        }

        if (line == "/quit") {
            break;
        }

        if (line.empty()) {
            std::cout << "> " << std::flush;
            continue;
        }

        chat::SendMessageRequest request;
        request.set_room_id("default");
        request.set_sender_id(username);
        request.set_text(line);

        grpc::ClientContext send_context;
        chat::Message       reply;

        grpc::Status status = chat_service->SendMessage(&send_context, request, &reply);

        if (!status.ok()) {
            std::cerr << "\nSend failed: "
                      << status.error_message() << '\n';
        }

        std::cout << "> " << std::flush;
    }


    subscribe_context.TryCancel();
    receive_thread.join();

    return 0;
}
