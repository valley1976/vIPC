#pragma once

#include <utility>

namespace valley {
namespace base {

namespace detail {

inline void on_cleanup_exception() noexcept
{
#if !defined(NDEBUG)
    assert(false && "cleanup handler threw");
#endif
}

} // namespace detail

template<typename Fn>
class Cleanup
{
public:
    template<typename F,
        typename std::enable_if<
        !std::is_same<typename std::decay<F>::type, Cleanup>::value, int>::type = 0>
        explicit Cleanup(F&& fn) : fn_(std::forward<F>(fn))
    {}

    ~Cleanup() noexcept
    {
        if (enable_)
        {
            try
            {
                fn_();
            }
            catch (...)
            {
                detail::on_cleanup_exception();
            }
        }
    }
    Cleanup(const Cleanup&) = delete;
    Cleanup& operator=(const Cleanup&) = delete;

    Cleanup(Cleanup&& orig) noexcept
        : enable_(orig.enable_)
        , fn_(std::forward<Fn>(orig.fn_))
    {
        orig.cancel();
    }

    Cleanup& operator=(Cleanup&&) = delete;

    void cancel() { enable_ = false; }

private:
    bool enable_ = true;
    Fn   fn_;
};

template<typename Fn>
Cleanup<typename std::decay<Fn>::type> make_cleanup(Fn&& fn)
{
    return Cleanup<typename std::decay<Fn>::type>(std::forward<Fn>(fn));
}

}
}
