#pragma once

#include <type_traits>
#include <utility>
#include <mutex>
#include <optional>
#include <atomic>

namespace utilmt
{
    /// @brief Identical to @a std::mutex except it records the owner thread id when it is locked
    /// so that you can query its lock state
    struct mutex: std::mutex
    {
    public:
        /// @brief Lock the mutex
        void lock()
        {
            std::mutex::lock();
            set_owner_id();
        }

        /// @brief Try to lock the mutex
        /// @return Whether succeed
        auto try_lock() -> bool
        {
            if(!std::mutex::try_lock())
                return false;

            set_owner_id();
            return true;
        }

        /// @brief Unlock the mutex
        void unlock()
        {
            reset_owner_id();
            std::mutex::unlock();
        }

        /// @brief Check whether this mutex is locked by a thread
        /// @return Result
        [[nodiscard]]
        auto is_locked() const -> bool
        {
            return get_owner_id() != std::thread::id();
        }

        /// @brief Get the id of the thread owns this mutex
        /// @return Thread id
        [[nodiscard]]
        auto get_owner_id() const -> std::thread::id
        {
            return m_owner_id;
        }

    private:
        void set_owner_id()
        {
            m_owner_id = std::this_thread::get_id();
        }

        void reset_owner_id()
        {
            m_owner_id = std::thread::id();
        }

        std::atomic<std::thread::id> m_owner_id{};
    };

    template<class T, class Mutex>
    struct guard;

    namespace internal
    {
        template<class T, class Mutex>
        struct guard_proxy
        {
        public:
            using value_type = T;
            using mutex_type = Mutex;
            using self_type = guard_proxy<value_type, mutex_type>;

            friend struct guard<value_type, mutex_type>;

        private:
            struct adopt_lock_t
            {
                explicit adopt_lock_t() = default;
            };
            static constexpr adopt_lock_t adopt_lock{};

        public:
            ~guard_proxy() = default;

            guard_proxy(const self_type&) = delete;
            auto operator=(const self_type&) -> self_type & = delete;

            guard_proxy(self_type&&) noexcept = default;
            auto operator=(self_type&&) -> self_type & = default;

            /// @brief Get the proxied value
            /// @return Proxied value
            [[nodiscard]]
            auto get() -> value_type&
            {
                return *m_value_ptr;
            }

            /// @brief Get the proxied value
            /// @return Proxied value
            [[nodiscard]]
            auto get() const -> const value_type&
            {
                return *m_value_ptr;
            }

            /// @brief Get the proxied value
            /// @return Proxied value pointer
            [[nodiscard]]
            auto operator->() -> value_type*
            {
                return m_value_ptr;
            }

            /// @brief Get the proxied value
            /// @return Proxied value pointer
            [[nodiscard]]
            auto operator->() const -> const value_type*
            {
                return m_value_ptr;
            }

            /// @brief Get the proxied value
            /// @return Proxied value
            [[nodiscard]]
            auto operator*() -> value_type&
            {
                return get();
            }

            /// @brief Get the proxied value
            /// @return Proxied value
            [[nodiscard]]
            auto operator*() const -> const value_type&
            {
                return get();
            }

        private:
            guard_proxy(value_type& value, mutex_type& mutex):
                m_lock(mutex),
                m_value_ptr(&value)
            {
            }

            guard_proxy(value_type& value, mutex_type& mutex, adopt_lock_t):
                m_lock(mutex, std::adopt_lock),
                m_value_ptr(&value)
            {
            }

        private:
            std::unique_lock<mutex_type> m_lock;
            value_type* m_value_ptr;
        };
    }

    /// @brief Wrapper to manager the lifetime of a value and a mutex
    /// @tparam T Value type
    /// @tparam Mutex Mutex type
    template<class T, class Mutex = std::mutex>
    struct guard
    {
    public:
        using value_type = T;
        using mutex_type = Mutex;
        using self_type = guard<value_type, mutex_type>;

        /// @brief Proxy type for data read and write while the mutex is locked with RAII
        using proxy = internal::guard_proxy<value_type, mutex_type>;

    public:
        /// @brief Constructor
        /// @tparam ...Args Argument types
        /// @param ...args Arguments
        template<class ...Args>
            requires std::is_constructible_v<value_type, Args...>
        guard(Args&& ...args):
            m_value(std::forward<Args...>(args)...)
        {
        }

        ~guard() = default;

        guard(const self_type&) = delete;
        auto operator=(const self_type&) -> self_type & = delete;

        guard(self_type&&) noexcept = default;
        auto operator=(self_type&&) noexcept -> self_type & = default;

        /// @brief Take over the ownership; This function blocks current thread
        /// @return Proxy
        auto take_over() -> proxy
        {
            return proxy(m_value, m_mutex);
        }

        /// @brief Try to take over the ownership, or returns @a std::nullopt
        /// @return Proxy on success, or @a std::nullopt on failure
        auto try_take_over() -> std::optional<proxy>
        {
            if(!m_mutex.try_lock())
                return std::nullopt;
            return proxy(m_value, m_mutex, proxy::adopt_lock);
        }

    private:
        value_type m_value;
        mutex_type m_mutex{};
    };
}