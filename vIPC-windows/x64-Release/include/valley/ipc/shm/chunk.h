#pragma once

#include <cassert>
#include <string>
#include <memory>
#include <mutex>
#include <type_traits>

#include "valley/ipc/export.h"
#include "valley/base/lang/optional.h"

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

    class Locked_data;

    Locked_data lock();
    base::Optional<Locked_data> try_lock();
    base::Optional<Locked_data> try_lock_for(const std::chrono::milliseconds& tiemout);

private:
    std::shared_ptr<internal::Chunk> impl_;
};

class LIBVALLEY_IPC_EXPORT Chunk::Locked_data
{
public:
    ~Locked_data();

    Locked_data(const Locked_data&) = delete;
    Locked_data& operator=(const Locked_data&) = delete;

    Locked_data(Locked_data&& orig) noexcept;
    Locked_data& operator=(Locked_data&& orig) noexcept;

    void* ptr();
    size_t size() const;

    bool is_abandoned() const;

    template<typename T>
    T* cast() {
        static_assert(std::is_standard_layout<T>::value, "bad T");
        assert(sizeof(T) <= size());
        return reinterpret_cast<T*>(ptr());
    }

private:
    friend Chunk;

    explicit Locked_data(internal::Chunk* locked_chunk);
private:
    internal::Chunk* impl_;
};

}
}