#include <iostream>
#include <vector>
#include <thread>
#include <format>
#include <chrono>

#include "utilmt/mutex.hpp"

namespace utilmt_test
{
    struct pod_struct
    {
        int i;
        float f;
        void* p;
    };

    void test_guard()
    {
        utilmt::guard<std::vector<pod_struct>, utilmt::mutex> pod_struct_guard{};
        constexpr size_t N = 5;

        std::jthread trd_1(
            [&]()
            {
                auto opt = pod_struct_guard.try_take_over();
                while(!opt.has_value())
                {
                    std::cout << "test_guard thread 1 try_take_over failed\n";
                    opt = pod_struct_guard.try_take_over();
                }
                auto proxy = std::move(opt.value());
                for(size_t i = 0; i < N; i++)
                    proxy->push_back({ .i = 1,.f = 0.5f, .p = nullptr });
                std::cout << "test_guard thread 1 executed\n";
            }
        );

        std::jthread trd_2(
            [&]()
            {
                auto opt = pod_struct_guard.try_take_over();
                while(!opt.has_value())
                {
                    std::cout << "test_guard thread 2 try_take_over failed\n";
                    opt = pod_struct_guard.try_take_over();
                }
                auto proxy = std::move(opt.value());
                for(size_t i = 0; i < N; i++)
                    proxy->push_back({ .i = 2,.f = 1.5f, .p = nullptr });
                std::cout << "test_guard thread 2 executed\n";
            }
        );

        std::jthread trd_3(
            [&]()
            {
                auto proxy = pod_struct_guard.take_over();
                for(size_t i = 0; i < N; i++)
                    (*proxy).push_back({ .i = 3,.f = 2.5f, .p = nullptr });
                std::cout << "test_guard thread 3 executed\n";
            }
        );

        trd_1.join();
        trd_2.join();

        for(const auto& elem : pod_struct_guard.take_over().get())
        {
            std::cout << std::format("i = {}, f = {}, p = {}", elem.i, elem.f, elem.p) << std::endl;
        }
    }
}

int main()
{
    std::cout << "Hello UtilMT" << std::endl;

    utilmt_test::test_guard();

    return 0;
}