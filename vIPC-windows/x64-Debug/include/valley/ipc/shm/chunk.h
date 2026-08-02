#pragma once

#include <cassert>
#include <string>
#include <memory>
#include <mutex>
#include <type_traits>

#include "valley/ipc/export.h"

namespace valley {
namespace ipc {
namespace internal {
class Chunk;
}

class LIBVALLEY_IPC_EXPORT Chunk
{
public:
    Chunk();
    explicit Chunk(std::shared_ptr<internal::Chunk>& impl);
    ~Chunk();

    Chunk(const Chunk&) = delete;
    Chunk& operator=(const Chunk&) = delete;

    Chunk(Chunk&& chunk) noexcept;
    Chunk& operator = (Chunk&& chunk) noexcept;

    static Chunk find(const std::string& arena, const std::string& name);

    operator bool() const { return impl_ != nullptr; }

    class Locked_data
    {
    public:
        explicit Locked_data(Chunk& chunk);
        ~Locked_data();

        Locked_data(const Locked_data&) = delete;
        Locked_data& operator=(const Locked_data&) = delete;

        Locked_data(Locked_data&& orig) noexcept;
        Locked_data& operator=(Locked_data&& orig) noexcept;

        void* data();
        size_t size() const;

    private:
        internal::Chunk* impl_;
    };

    Locked_data data();

    template<typename T>
    class Typed_data : Locked_data
    {
        static_assert(std::is_standard_layout<T>::value, "bad T");
    public:
        using Locked_data::Locked_data;

        T* operator->() { return cast(); }
        T& operator*() { return *cast(); }

    private:
        T* cast() {
            assert(sizeof(T) <= size());
            return reinterpret_cast<T*>(ptr());
        }
    };

    template<typename T>
    Typed_data<T> get() {
        return Typed_data<T>(*this);
    }

private:
    std::shared_ptr<internal::Chunk> impl_;
};

}
}