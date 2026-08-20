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
    using Bytes = std::vector<Byte>;
    using iterator = Bytes::iterator;
    using citerator = Bytes::const_iterator;

    Message();
    explicit Message(size_t size);
    ~Message();

    Message(const Message& other) : Message() { bytes_ = other.bytes_; }
    Message& operator=(const Message& other) { if (this != &other) bytes_ = other.bytes_; return *this; } 
    Message(Message&& orig) noexcept;
    Message& operator=(Message&& orig) noexcept;

    void reserve(size_t size);
    void resize(size_t size);

    bool empty() const { return bytes_.empty(); }

    Byte* data();
    const Byte* data() const;
    size_t size() const;

    iterator begin() { return bytes_.begin(); }
    iterator end()   { return bytes_.end(); }

    citerator begin() const { return bytes_.begin(); }
    citerator end() const   { return bytes_.end(); }

    Byte& operator[](size_t pos) { return bytes_[pos]; }
    const Byte& operator[](size_t pos) const { return bytes_[pos]; }

    template<typename Iter>
    void insert(iterator pos, Iter first, Iter last) { bytes_.insert(pos, first, last); }

    template<typename Iter>
    void append(Iter first, Iter last) { bytes_.insert(bytes_.end(), first, last); }

private:
    class Bin;

    Bin* bin_ = nullptr;
    Bytes bytes_;
};

// inline 

inline Message::Message(size_t size) : Message()        { resize(size); }

inline void Message::reserve(size_t size)               { bytes_.reserve(size); }
inline void Message::resize(size_t size)                { bytes_.resize(size); }

inline Message::Byte* Message::data()                   { return bytes_.data(); }
inline const Message::Byte* Message::data() const       { return bytes_.data(); }

inline size_t Message::size() const                     { return bytes_.size(); }

Message make_message(const char* str);
Message make_message(const std::string& str);

}
}