#ifndef LEARN_SERVER_HPP
#define LEARN_SERVER_HPP

#include "learn.pb.h"

#include <learn.grpc.pb.h>

#include <google/protobuf/util/time_util.h>
#include <grpcpp/security/server_credentials.h>
#include <grpcpp/server.h>
#include <grpcpp/server_builder.h>
#include <grpcpp/server_context.h>
#include <grpcpp/support/status.h>

#include <condition_variable>


class ChatService : public chat::ChatService::Service
{
public:
    explicit ChatService()
    {
        messages_.reserve(100'000);
    }

    grpc::Status SendMessage(grpc::ServerContext* context,
        const chat::SendMessageRequest*           request,
        chat::Message*                            response) override
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


        response->set_id(next_id_);
        response->set_room_id(request->room_id());
        response->set_sender_id(request->sender_id());
        response->set_text(request->text());
        response->mutable_timestamp()->CopyFrom(google::protobuf::util::TimeUtil::GetCurrentTime());

        {
            std::lock_guard lock {messages_mutex_};
            messages_.push_back(*response);
            ++next_id_;
        }
        new_message_cv.notify_one();

        return grpc::Status::OK;
    }

    grpc::Status GetMessages(grpc::ServerContext* context,
        const chat::GetMessagesRequest*           request,
        chat::GetMessagesResponse*                response) override
    {
        std::vector<chat::Message> messages_copy;
        {
            std::lock_guard lock {messages_mutex_};
            messages_copy = messages_;
        }

        for (const auto& message : messages_copy) {
            *response->add_messages() = message;
        }

        return grpc::Status::OK;
    }

    grpc::Status Subscribe(grpc::ServerContext* context, const chat::SubscribeRequest* request,
        grpc::ServerWriter<chat::Message>* writer) override
    {
        int64_t                    last_id = request->after_message_id();
        std::vector<chat::Message> pending;
        while (!context->IsCancelled()) {
            {
                std::unique_lock lock {messages_mutex_};

                new_message_cv.wait_for(
                    lock,
                    std::chrono::milliseconds(200),
                    [&] { return context->IsCancelled() || (last_id < next_id_ - 1); });

                if (context->IsCancelled()) {
                    break;
                }


                pending.insert(pending.begin(), messages_.begin() + last_id, messages_.end());
            }

            for (const auto& message : pending) {
                if (!writer->Write(message)) {
                    return grpc::Status::OK;
                }

                last_id = message.id();
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
