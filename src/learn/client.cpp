#include "learn.grpc.pb.h"
#include "learn.pb.h"

#include <grpcpp/create_channel.h>
#include <grpcpp/grpcpp.h>
#include <grpcpp/security/credentials.h>

#include <chrono>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <iostream>
#include <mutex>
#include <string>
#include <thread>


std::string format_chat_time(
    const google::protobuf::Timestamp& timestamp)
{
    using namespace std::chrono;

    const sys_seconds point {seconds {timestamp.seconds()}};
    return std::format("{:%H:%M:%S}", zoned_time {current_zone(), point});
}

constexpr const char* ROOM_ID = "default";

std::mutex print_mutex;

void print_message(const chat::Message& message)
{
    std::cout << "[" << format_chat_time(message.timestamp()) << "] "
              << message.sender_id() << ": " << message.text() << '\n';
}

int main()
{
    auto channel      = grpc::CreateChannel("127.0.0.1:50051", grpc::InsecureChannelCredentials());
    auto chat_service = chat::ChatService::NewStub(channel);

    std::cout << "Who are you?: " << std::flush;
    std::string username;
    if (!std::getline(std::cin, username) || username.empty()) {
        return EXIT_FAILURE;
    }

    std::cout << "\033[2J\033[H"
              << "Room: " << ROOM_ID << " | User: " << username << '\n'
              << "Type messages; use /quit to exit.\n"
              << "------------------------------------------------\n";

    grpc::ClientContext history_context;
    history_context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(2));
    chat::GetMessagesRequest history_request;
    history_request.set_room_id(ROOM_ID);
    chat::GetMessagesResponse history;

    grpc::Status history_status = chat_service->GetMessages(&history_context, history_request, &history);
    if (!history_status.ok()) {
        std::cerr << "Could not load history: " << history_status.error_message() << '\n';
        return EXIT_FAILURE;
    }

    uint64_t last_seen_id = 0;
    for (const auto& message : history.messages()) {
        print_message(message);
        last_seen_id = message.id();
    }

    grpc::ClientContext    subscribe_context;
    chat::SubscribeRequest subscribe_request;
    subscribe_request.set_room_id(ROOM_ID);
    subscribe_request.set_after_message_id(last_seen_id);

    std::cout << "> " << std::flush;

    auto        reader = chat_service->Subscribe(&subscribe_context, subscribe_request);
    std::thread receive_thread([&] {
            chat::SubscribeResponse response;

            while (reader->Read(&response)) {
                std::lock_guard lock {print_mutex};
                // Clear the "> " prompt line, print the message, then redraw the prompt.
                std::cout << "\r\033[K";
                print_message(response.message());
                std::cout << "> " << std::flush;
            }

            grpc::Status status = reader->Finish();
            if (!status.ok() && (status.error_code() != grpc::StatusCode::CANCELLED)) {
                std::lock_guard lock {print_mutex};
                std::cerr << "\r\033[KSubscription ended: " << status.error_message() << '\n';
            }
        });

    // 3. Read input and send. Our own messages come back through the stream, like everyone else's.
    std::string line;
    while (std::getline(std::cin, line) && line != "/quit") {
        {
            std::lock_guard lock {print_mutex};
            // Erase the line the user just typed; the server echo replaces it.
            std::cout << "\033[A\r\033[K> " << std::flush;
        }

        if (line.empty()) {
            continue;
        }

        chat::SendMessageRequest request;
        request.set_room_id(ROOM_ID);
        request.set_sender_id(username);
        request.set_text(line);

        grpc::ClientContext send_context;
        send_context.set_deadline(std::chrono::system_clock::now() + std::chrono::seconds(2));
        chat::SendMessageResponse reply;

        grpc::Status status = chat_service->SendMessage(&send_context, request, &reply);

        if (!status.ok()) {
            std::lock_guard lock {print_mutex};
            std::cerr << "\r\033[KSend failed: " << status.error_message() << '\n';
            std::cout << "> " << std::flush;
        }
    }

    subscribe_context.TryCancel();
    receive_thread.join();

    return 0;
}
