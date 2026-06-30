#pragma once

#include <utility>

namespace valley {
namespace lang {

template<typename Fn>
class Cleanup
{
public:
    template<typename F>
    explicit Cleanup(F&& fn) :
        fn_(std::forward<F>(fn))
    {}

    ~Cleanup() noexcept
    {
        if (enable_)
        {
            try {
                fn_();
            }
            catch (...)
            {
                // TODO
            }
        }
    }

    Cleanup(const Cleanup&) = delete;
    Cleanup& operator=(const Cleanup&) = delete;

    Cleanup(Cleanup&& orig) noexcept :
        enable_(orig.enable_),
        fn_(std::move(orig.fn_))
    {
        orig.cancel();
    }

    Cleanup& operator=(Cleanup&& orig) = delete;

    void cancel()
    {
        enable_ = false;
    }

private:
    bool enable_ = true;
    Fn fn_;
};

template<typename Fn>
Cleanup<Fn> make_cleanup(Fn&& fn) {
    return Cleanup<Fn>{ std::forward<Fn>(fn) };
}

}
}
