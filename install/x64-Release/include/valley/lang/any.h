#pragma once

#include <typeinfo>
#include <type_traits>
#include <utility>

namespace valley {
namespace lang {

class Any
{
    struct Base
    {
        virtual ~Base() = default;
    };

    template <typename T>
    struct Derived : Base {
        T value;
        template <typename U>
        Derived(U&& val) : value(std::forward<U>(val)) {}
    };

public:
    Any() = default;

    template <typename T>
    Any(T&& value) : ptr_(new Derived<typename std::decay<T>::type>(std::forward<T>(value))) {}
    ~Any() { delete ptr_; }

    Any(const Any&) = delete;
    Any& operator=(const Any&) = delete;

    Any(Any&& other) noexcept : ptr_(other.ptr_) { other.ptr_ = nullptr; }

    Any& operator=(Any&& other) noexcept {
        if (this != &other) {
            delete ptr_;
            ptr_ = other.ptr_;
            other.ptr_ = nullptr;
        }
        return *this;
    }

    bool has_value() const noexcept { return ptr_ != nullptr; }

    template <typename T>
    T cast() const {
        if (!ptr_) throw std::bad_cast();
        auto derived = dynamic_cast<Derived<typename std::decay<T>::type*>>(ptr_);
        if (!derived) throw std::bad_cast();
        return derived->value;
    }

    template <typename T>
    T& cast_ref() {
        if (!ptr_) throw std::bad_cast();
        auto derived = dynamic_cast<Derived<typename std::decay<T>::type*>>(ptr_);
        if (!derived) throw std::bad_cast();
        return derived->value;
    }

    template <typename T>
    const T& cast_ref() const {
        if (!ptr_) throw std::bad_cast();
        auto derived = dynamic_cast<const Derived<typename std::decay<T>::type*>>(ptr_);
        if (!derived) throw std::bad_cast();
        return derived->value;
    }

private:
    Base* ptr_ = nullptr;
};

}
}