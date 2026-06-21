#pragma once

#include <memory>
#include <string>

#include <valley/lang/storage.h>

namespace valley {
namespace shm {

namespace internal {
class Channel;
}

class Channel
{
public:
    Channel(const std::string& name);
    ~Channel();

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

    class Impl;

    class Publisher
    {
    public:
        Publisher(Channel& channel);
        ~Publisher();

        Publisher(const Publisher&) = delete;

        template<typename T>
        T* prepare_cast()
        {
            assert(sizeof(T) <= data_size());

            return reinterpret_cast<T*>(prepare());
        }

        void* prepare();
        void write();

    private:
        size_t data_size() const;

    private:
        class Impl;
        lang::Storage<Impl, 16, 8> impl_;
    };

    struct Subscriber
    {
        Subscriber(Channel& channel, const std::string& name);
        ~Subscriber();

        Subscriber(const Publisher&) = delete;

        template<typename T>
        T* read_cast()
        {
            assert(sizeof(T) <= data_size());
            return reinterpret_cast<T*>(read());
        }

        void* read();

    private:
        size_t data_size() const;

    private:
        class Impl;

        lang::Storage<Impl, 32, 8> impl_;
    };

private:
    size_t data_size() const;

private:
    std::unique_ptr<internal::Channel> impl_;
};

}
}