#pragma once
#include <cstdint>
#include <functional>
#include <vector>
#include <algorithm>

struct Message {
    enum class ID { ChannelStatistics };
    ID type;
    explicit Message(ID id) : type(id) {}
    virtual ~Message() = default;
};

struct ChannelStatistics {
    int32_t max_db{-90};
};

struct ChannelStatisticsMessage : public Message {
    ChannelStatistics statistics{};
    ChannelStatisticsMessage() : Message(Message::ID::ChannelStatistics) {}
};

// Global message dispatcher — handlers register/unregister themselves.
struct HandlerEntry {
    Message::ID id;
    std::function<void(const Message*)> fn;
    int token;
};

inline std::vector<HandlerEntry>& sim_handlers() {
    static std::vector<HandlerEntry> v;
    return v;
}
inline int& sim_next_token() {
    static int t = 0;
    return t;
}
inline void dispatch_message(const Message* msg) {
    // Copy the list first in case a handler modifies it (e.g. causes a pop).
    auto snap = sim_handlers();
    for (auto& h : snap)
        if (h.id == msg->type) h.fn(msg);
}

class MessageHandlerRegistration {
public:
    MessageHandlerRegistration(Message::ID id, std::function<void(const Message*)> fn)
        : token_(++sim_next_token()) {
        sim_handlers().push_back({id, std::move(fn), token_});
    }
    ~MessageHandlerRegistration() {
        auto& v = sim_handlers();
        v.erase(std::remove_if(v.begin(), v.end(),
            [t = token_](const HandlerEntry& e) { return e.token == t; }), v.end());
    }
    // Non-copyable, moveable
    MessageHandlerRegistration(const MessageHandlerRegistration&) = delete;
    MessageHandlerRegistration& operator=(const MessageHandlerRegistration&) = delete;
private:
    int token_;
};
