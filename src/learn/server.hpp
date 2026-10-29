#ifndef LEARN_SERVER_HPP
#define LEARN_SERVER_HPP

#include "learn.grpc.pb.h"
#include "learn.pb.h"

#include <google/protobuf/util/time_util.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/status.h>

#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <vector>


class ChatServiceImpl : public chat::ChatService::Service
{
public:
    ChatServiceImpl()
    {
        messages_.reserve(100'000);
    }

    grpc::Status SendMessage(grpc::ServerContext* context,
        const chat::SendMessageRequest*           request,
        chat::SendMessageResponse*                response) override
    {
        if (request->sender_id().empty()) {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Empty sender ID!");
        }

        if (request->text().empty()) {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Empty message!");
        }

        if (request->room_id().empty()) {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Empty room ID!");
        }


        chat::Message* message = response->mutable_message();
        message->set_room_id(request->room_id());
        message->set_sender_id(request->sender_id());
        message->set_text(request->text());
        message->mutable_timestamp()->CopyFrom(google::protobuf::util::TimeUtil::GetCurrentTime());

        {
            std::lock_guard lock {messages_mutex_};
            message->set_id(next_id_++);
            messages_.push_back(*message);
        }
        new_message_cv.notify_all();

        return grpc::Status::OK;
    }

    grpc::Status GetMessages(grpc::ServerContext* context,
        const chat::GetMessagesRequest*           request,
        chat::GetMessagesResponse*                response) override
    {
        if (request->room_id().empty()) {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Empty room ID!");
        }

        std::lock_guard lock {messages_mutex_};
        for (const auto& message : messages_) {
            if (message.room_id() == request->room_id()) {
                *response->add_messages() = message;
            }
        }

        return grpc::Status::OK;
    }

    grpc::Status Subscribe(grpc::ServerContext* context, const chat::SubscribeRequest* request,
        grpc::ServerWriter<chat::SubscribeResponse>* writer) override
    {
        if (request->room_id().empty()) {
            return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "Empty room ID!");
        }

        uint64_t next_index = request->after_message_id();

        {
            std::lock_guard lock {messages_mutex_};
            if (next_index > messages_.size()) {
                return grpc::Status(grpc::StatusCode::INVALID_ARGUMENT, "after_message_id is in the future!");
            }
        }

        while (!context->IsCancelled()) {
            std::vector<chat::Message> pending;
            {
                std::unique_lock lock {messages_mutex_};

                new_message_cv.wait_for(
                    lock,
                    std::chrono::milliseconds(200),
                    [&] { return context->IsCancelled() || (next_index < messages_.size()); });

                if (context->IsCancelled()) {
                    break;
                }

                for (; next_index < messages_.size(); ++next_index) {
                    if (messages_[next_index].room_id() == request->room_id()) {
                        pending.push_back(messages_[next_index]);
                    }
                }
            }

            chat::SubscribeResponse response;
            for (const auto& message : pending) {
                *response.mutable_message() = message;
                if (!writer->Write(response)) {
                    return grpc::Status::OK;
                }
            }
        }

        return grpc::Status::OK;
    }

private:
    std::vector<chat::Message> messages_;
    std::mutex                 messages_mutex_;
    uint64_t                   next_id_ {1};
    std::condition_variable    new_message_cv;
};

#endif // LEARN_SERVER_HPP
