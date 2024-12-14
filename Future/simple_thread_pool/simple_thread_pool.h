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
    template <class F, class... Args>
    auto enqueue(F&& f, Args&&... args)
        -> std::future<typename std::result_of<F(Args...)>::type>;
    auto wait_for_all() -> void;
    ~thread_pool();

   private:
    // need to keep track of threads so we can join them
    std::vector<std::thread> workers;
    // the task queue
    std::queue<std::function<void()>> _tasks;
    // synchronization
    std::mutex queue_mutex;
    std::condition_variable _condition;
    bool _stop;
    // for waiting for all tasks
    std::atomic<int> _running_tasks_num{0};
    std::atomic<bool> _task_completed{false};
};

// the constructor just launches some amount of workers
thread_pool::thread_pool(size_t threads) : _stop(false) {
    for (size_t i = 0; i < threads; i += 1)
        workers.emplace_back([this] {
            for (;;) {
                std::function<void()> task;
                {
                    std::unique_lock<std::mutex> lock(this->queue_mutex);
                    _condition.wait(lock, [this] {
                        return _stop || !this->_tasks.empty();
                    });
                    if (_stop && _tasks.empty()) {
                        return;
                    }
                    task = std::move(_tasks.front());
                    _tasks.pop();
                }
                int prev_running_tasks_num = _running_tasks_num.fetch_add(1);
                if (prev_running_tasks_num == 0) {
                    _task_completed.store(false);
                }
                task();
                _running_tasks_num.fetch_sub(1);
                if (_running_tasks_num.load() == 0) {
                    _task_completed.store(true);
                    _task_completed.notify_one();
                }
            }
        });
}

// add new work item to the pool
template <class F, class... Args>
auto thread_pool::enqueue(F&& f, Args&&... args)
    -> std::future<typename std::result_of<F(Args...)>::type> {
    using return_type = typename std::result_of<F(Args...)>::type;
    auto task = std::make_shared<std::packaged_task<return_type()>>(
        std::bind(std::forward<F>(f), std::forward<Args>(args)...));
    std::future<return_type> res = task->get_future();
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        // don't allow enqueueing after stopping the pool
        if (_stop) {
            throw std::runtime_error("enqueue on stopped thread_pool");
        }
        _tasks.emplace([task]() { (*task)(); });
    }
    _condition.notify_one();
    return res;
}

auto thread_pool::wait_for_all() -> void {
    if (_running_tasks_num.load() > 0) {
        _task_completed.wait(false);
    }
}

// the destructor joins all threads
thread_pool::~thread_pool() {
    {
        std::unique_lock<std::mutex> lock(queue_mutex);
        _stop = true;
    }
    _condition.notify_all();
    for (std::thread& worker : workers) {
        worker.join();
    }
}

}  // namespace simple_thread_pool
#endif