
#include <iostream>
#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include <folly/Function.h>
#include <folly/executors/CPUThreadPoolExecutor.h>
#include <folly/executors/IOThreadPoolExecutor.h>
#include <folly/futures/Future.h>

#include "dp_thread_pool/thread_pool.h"
#include "task_thread_pool.hpp"
#include "BS_thread_pool.hpp"

#include "simple_thread_pool/simple_thread_pool.h"

void simple_task() {
    auto str = std::string("LazyOrEager") + "LazyOrEager";
}

simple_thread_pool::thread_pool sp_pool(16);

static void BM_StdFutureSPThreadPool(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < 25; i += 1) {
            sp_pool.enqueue(simple_task);
        }
        sp_pool.wait_for_all();
    }
}


dp::thread_pool dp_pool(16);

static void BM_StdFutureDPThreadPool(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < 25; i += 1) {
            dp_pool.enqueue_detach(simple_task);
        }
        dp_pool.wait_for_tasks();
    }
}

BS::thread_pool bs_pool(16);

static void BM_StdFutureBSThreadPool(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < 25; i += 1) {
            bs_pool.detach_task(simple_task);
        }
        bs_pool.wait();
    }
}

task_thread_pool::task_thread_pool task_pool(16);

static void BM_StdFutureTaskThreadPool(benchmark::State& state) {
    for (auto _ : state) {
        for (int i = 0; i < 25; i += 1) {
            task_pool.submit_detach(simple_task);
        }
        task_pool.wait_for_tasks();
    }
}

folly::CPUThreadPoolExecutor executor(16);

static void BM_FollyFutureThreadPool(benchmark::State& state) {
    for (auto _ : state) {
        std::vector<folly::Future<folly::Unit>> futures;
        for (int i = 0; i < 25; i += 1) {
            futures.push_back(
                folly::makeFuture().via(&executor).thenValue([](folly::Unit x) {
                    simple_task();
                    return folly::Unit{};
                }));
        }
        folly::collectAll(futures.begin(), futures.end()).get();
    }
}

// Register the function as a benchmark
BENCHMARK(BM_StdFutureSPThreadPool);
BENCHMARK(BM_StdFutureDPThreadPool);
BENCHMARK(BM_StdFutureBSThreadPool);
BENCHMARK(BM_StdFutureTaskThreadPool);
BENCHMARK(BM_FollyFutureThreadPool);

BENCHMARK_MAIN();