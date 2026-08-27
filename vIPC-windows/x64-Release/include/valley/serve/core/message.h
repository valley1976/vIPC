#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "valley/serve/export.h"

namespace valley {
namespace serve {

class LIBVALLEY_SERVE_EXPORT Message
{
public:
    using Byte = uint8_t;
    using Payload = std::vector<Byte>;
    using iterator  = Payload::iterator;
    using const_iterator = Payload::const_iterator;

    Message();
    explicit Message(size_t size);
    ~Message();

    Message(const Message& other) = delete;
    Message& operator=(const Message& other) = delete;
    
    Message(Message&& orig) noexcept;
    Message& operator=(Message&& orig) noexcept;

    static void set_max_size(size_t kb);
    static size_t max_size();

    void reserve(size_t size);
    void resize(size_t size);

    bool empty() const { return payload_.empty(); }

    Byte* data();
    const Byte* data() const;
    size_t size() const;

    iterator begin();
    iterator end();

    const_iterator begin() const;
    const_iterator end() const;

    Byte& operator[](size_t pos);
    const Byte& operator[](size_t pos) const;

    template<typename Iter>
    void insert(iterator pos, Iter first, Iter last);

    template<typename Iter>
    void append(Iter first, Iter last);

    Message consume(size_t size);

private:
    class Bin;

    static size_t max_size_;
    Bin* bin_ = nullptr;
    Payload payload_;
};

// inline 

inline Message::Byte* Message::data()                   { return payload_.data(); }
inline const Message::Byte* Message::data() const       { return payload_.data(); }

inline size_t Message::max_size() { return max_size_; }

inline size_t Message::size() const                     { return payload_.size(); }

inline Message::iterator Message::begin() { return payload_.begin(); }
inline Message::iterator Message::end()   { return payload_.end(); }

inline Message::const_iterator Message::begin() const { return payload_.begin(); }
inline Message::const_iterator Message::end() const   { return payload_.end(); }

inline Message::Byte& Message::operator[](size_t pos) { return payload_[pos]; }
inline const Message::Byte& Message::operator[](size_t pos) const { return payload_[pos]; }

template<typename Iter>
inline void Message::insert(iterator pos, Iter first, Iter last) { payload_.insert(pos, first, last); }

template<typename Iter>
inline void Message::append(Iter first, Iter last) { payload_.insert(payload_.end(), first, last); }

Message LIBVALLEY_SERVE_EXPORT make_message(const char* str);
Message LIBVALLEY_SERVE_EXPORT make_message(const std::string& str);

}
}