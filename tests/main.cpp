#include <iostream>
#include <vector>
#include <thread>
#include <format>
#include <chrono>

#include "utilmt/mutex.hpp"
#include "utilmt/ring_buffer.hpp"

namespace utilmt_test
{
    struct pod_struct
    {
        int i;
        float f;
        void* p;
    };

    void print_pod_struct(const pod_struct& pod)
    {
        std::cout << std::format("POD structure: i = {}, f = {}, p = {}", pod.i, pod.f, pod.p) << std::endl;
    }

    void test_guard()
    {
        using guard_type = utilmt::guard<std::vector<pod_struct>, utilmt::mutex>;
        guard_type guard{};
        constexpr size_t N = 5;

        std::jthread thread1(
            [&]()
            {
                std::optional<guard_type::proxy> opt = guard.try_take_over();
                while(!opt.has_value())
                {
                    std::cout << "test_guard thread 1 try_take_over failed\n";
                    opt = guard.try_take_over();
                }
                guard_type::proxy proxy = std::move(opt.value());
                for(size_t i = 0; i < N; i++)
                    proxy->push_back({ .i = 1,.f = 0.5f, .p = nullptr });
                std::cout << "test_guard thread 1 executed\n";
            }
        );

        std::jthread thread2(
            [&]()
            {
                std::optional<guard_type::proxy> opt = guard.try_take_over();
                while(!opt.has_value())
                {
                    std::cout << "test_guard thread 2 try_take_over failed\n";
                    opt = guard.try_take_over();
                }
                guard_type::proxy proxy = std::move(opt.value());
                for(size_t i = 0; i < N; i++)
                    proxy->push_back({ .i = 2,.f = 1.5f, .p = nullptr });
                std::cout << "test_guard thread 2 executed\n";
            }
        );

        std::jthread trd_3(
            [&]()
            {
                guard_type::proxy proxy = guard.take_over();
                for(size_t i = 0; i < N; i++)
                    (*proxy).push_back({ .i = 3,.f = 2.5f, .p = nullptr });
                std::cout << "test_guard thread 3 executed\n";
            }
        );

        thread1.join();
        thread2.join();

        for(const auto& elem : guard.take_over().get())
        {
            std::cout << std::format("i = {}, f = {}, p = {}", elem.i, elem.f, elem.p) << std::endl;
        }
    }

    void test_ring_buffer()
    {
        utilmt::ring_buffer<pod_struct, 16> ring_buf{};
        std::cout << std::format("Size of ring buffer: {}", ring_buf.size_v) << std::endl;

        std::jthread thread1(
            [&]()
            {
                for(std::size_t i = ring_buf.size_v; i < ring_buf.size_v * 2; ++i)
                {
                    ring_buf.write(i, float(i), nullptr);
                }
            }
        );

        std::jthread thread2(
            [&]()
            {
                for(std::size_t i = 0; i < ring_buf.size_v; ++i)
                {
                    print_pod_struct(ring_buf.read());
                }
            }
        );

        thread1.join();
        thread2.join();
    }
}

int main()
{
    std::cout << "Hello UtilMT" << std::endl;

    //utilmt_test::test_guard();
    utilmt_test::test_ring_buffer();

    return 0;
}