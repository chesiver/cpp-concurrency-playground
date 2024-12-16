#ifndef THREAD_POOL_H
#define THREAD_POOL_H

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <iostream>
#include <memory>
#include <mutex>
#include <queue>
#include <stdexcept>
#include <thread>
#include <vector>

namespace simple_thread_pool {

class thread_pool {
   public:
    explicit thread_pool(size_t);
    template <typename F, typename... Args,
              typename ReturnType = std::invoke_result_t<F &&, Args &&...>>
    auto enqueue(F &&f, Args &&...args) -> std::future<ReturnType>;
    auto wait_for_all() -> void;
    ~thread_pool();

   private:
    // need to keep track of threads so we can join them
    std::vector<std::jthread> _workers;
    // the task queue
    std::queue<std::function<void()>> _tasks;
    // synchronization
    std::mutex _queue_mutex;
    std::condition_variable_any _condition;
    // for waiting for all tasks
    std::atomic<int> _running_tasks_num{0};
    std::atomic<bool> _task_completed{false};
};

// the constructor just launches some amount of _workers
thread_pool::thread_pool(size_t threads) {
    for (size_t i = 0; i < threads; i += 1)
        _workers.emplace_back([this](std::stop_token stop_token) {
            while (true) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(_queue_mutex);
                    _condition.wait(lock, stop_token,
                                    [this] { return !_tasks.empty(); });
                    if (stop_token.stop_requested()) {
                        break;
                    }
                    task = std::move(_tasks.front());
                    _tasks.pop();
                }
                int prev_running_tasks_num = _running_tasks_num.fetch_add(1);
                if (prev_running_tasks_num == 0) {
                    _task_completed.store(false);
                }
                std::invoke(task);
                _running_tasks_num.fetch_sub(1);
                if (_running_tasks_num.load() == 0) {
                    _task_completed.store(true);
                    _task_completed.notify_one();
                }
            }
        });
}

// add new work item to the pool
template <typename F, typename... Args,
          typename ReturnType = std::invoke_result_t<F &&, Args &&...>>
auto thread_pool::enqueue(F &&f, Args &&...args) -> std::future<ReturnType> {
    auto task = std::make_shared<std::packaged_task<ReturnType()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<ReturnType> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(_queue_mutex);
        _tasks.emplace([task]() { std::invoke(*task); });
    }
    _condition.notify_one();
    return res;
}

auto thread_pool::wait_for_all() -> void {
    if (_running_tasks_num.load() > 0) {
        _task_completed.wait(false);
    }
}

thread_pool::~thread_pool() {
    wait_for_all();
    for (auto &worker : _workers) {
        worker.request_stop();
    }
}

}  // namespace simple_thread_pool
#endif