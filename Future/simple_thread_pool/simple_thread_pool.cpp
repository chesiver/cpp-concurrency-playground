#include "simple_thread_pool.h"

int main() {
    simple_thread_pool::thread_pool pool(4);
    std::vector<std::future<int>> results;
    for(int i = 0; i < 8; i += 1) {
        results.emplace_back(
            pool.enqueue([i] {
                std::cout << "hello " << i << std::endl;
                std::this_thread::sleep_for(std::chrono::seconds(1));
                std::cout << "world " << i << std::endl;
                return i*i;
            })
        );
    }
    // pool.wait_for_all();
    for(auto && result: results)
        std::cout << result.get() << ' ';
    std::cout << std::endl;
    return 0;
}