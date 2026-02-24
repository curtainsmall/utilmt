#include <functional>
#include <type_traits>

namespace utilmt
{
    struct defer
    {
    public:
        using function_type = std::function<void()>;
    public:
        defer() = default;
        defer(const function_type& fn):
            _fn(fn)
        {
        }

        template<class ...Args>
            requires std::is_constructible_v<function_type, Args...>
        defer(Args&& ...args):
            _fn(std::forward<Args>(args)...)
        {
        }

        ~defer()
        {
            _fn();
        }

        defer(const defer&) = delete;
        defer(defer&&) = default;

        auto operator=(const defer&) -> defer & = delete;
        auto operator=(defer&&) noexcept -> defer & = default;

        void set(const function_type& fn)
        {
            _fn = fn;
        }

        template<class ...Args>
        void set(Args&& ...args)
        {
            _fn = std::function(std::forward<Args>(args)...);
        }

        void reset()
        {
            _fn = nullptr;
        }
    private:
        function_type _fn{};
    };
}