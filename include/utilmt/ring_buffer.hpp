#pragma once

#include <array>
#include <assert.h>
#include <bitset>
#include <condition_variable>
#include <mutex>
#include <type_traits>

#include "utilmt/defer.hpp"

namespace utilmt
{
    template<class T, std::size_t Size = 256>
    struct ring_buffer
    {
    public:
        using value_type = T;

        static constexpr std::size_t size_v = Size;

    private:
        using _mutex_type = std::mutex;

    public:
        ring_buffer() = default;

        ~ring_buffer()
        {
            _clear_all();
        }

        ring_buffer(const ring_buffer&) = delete;
        ring_buffer(ring_buffer&&) noexcept = delete;

        auto operator=(const ring_buffer&) -> ring_buffer & = delete;
        auto operator=(ring_buffer&&) -> ring_buffer & = delete;

        template<class ...Args>
            requires std::is_constructible_v<value_type, Args...>
        void write(Args&& ...args)
        {
            std::unique_lock<_mutex_type> lock(_mutex);

            if(full())
            {
                _cond_var.wait(
                    lock,
                    [&]() -> bool
                    {
                        return !full();
                    }
                );
            }

            _write(std::forward<Args>(args)...);
        }

        template<class ...Args>
            requires std::is_constructible_v<value_type, Args...>
        auto try_write(Args&& ...args) -> bool
        {
            std::unique_lock<_mutex_type> lock(_mutex, std::try_to_lock);

            if(!lock || full())
            {
                return false;
            }

            _write(std::forward<Args>(args)...);
            _cond_var.notify_one();
            return true;
        }

        auto read() -> value_type
        {
            std::unique_lock<_mutex_type> lock(_mutex);

            if(empty())
            {
                _cond_var.wait(
                    lock,
                    [&]() -> bool
                    {
                        return !empty();
                    }
                );
            }

            return _read();
        }

        auto try_read() -> std::optional<value_type>
        {
            std::unique_lock<_mutex_type> lock(_mutex, std::try_to_lock);

            if(!lock || empty())
            {
                return std::nullopt;
            }

            return _read();
        }

        auto full() const -> bool
        {
            return _read_idx == _write_idx && _value_flags.all();
        }

        auto empty() const -> bool
        {
            return _read_idx == _write_idx && _value_flags.none();
        }

    private:
        // This function is non-blocking
        template<class ...Args>
            requires std::is_constructible_v<value_type, Args...>
        void _write(Args&& ...args)
        {
            _construct_at(_write_idx, std::forward<Args>(args)...);
            _write_idx = _next(_write_idx);
            _cond_var.notify_all();
        }

        // This function is non-blocking
        auto _read() -> value_type
        {
            defer(
                [this, idx = _read_idx]()
                {
                    _clear_at(idx);
                }
            );

            value_type& tmp = _at(_read_idx);
            _read_idx = _next(_read_idx);
            _cond_var.notify_all();
            return std::move(tmp);
        }

        auto _at(std::size_t idx) -> value_type&
        {
            return reinterpret_cast<value_type&>(_values.at(idx * sizeof(value_type)));
        }

        auto _next(std::size_t curr) const -> std::size_t
        {
            return (curr + 1) % size_v;
        }

        template<class ...Args>
        void _construct_at(std::size_t idx, Args&& ...args)
        {
            _clear_at(idx);
            (void) new(&_at(idx)) value_type(std::forward<Args>(args)...);
            _value_flags.set(idx);
        }

        void _clear_at(std::size_t idx)
        {
            assert(idx < size_v);

            if(!_value_flags.test(idx))
                return;

            reinterpret_cast<value_type&>(_values.at(idx)).~value_type();
            _value_flags.reset(idx);
        }

        void _clear_all()
        {
            for(std::size_t i = 0; i < size_v; ++i)
                _clear_at(i);
        }
    private:
        std::array<std::byte, size_v * sizeof(value_type)> _values{};
        std::bitset<size_v> _value_flags{};

        std::size_t _write_idx{ 0 };
        std::size_t _read_idx{ 0 };

        _mutex_type _mutex{};
        std::condition_variable _cond_var{};
    };
}