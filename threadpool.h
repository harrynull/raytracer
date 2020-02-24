#pragma once
#include <queue>
#include <functional>
#include <mutex>
#include <thread>

class ThreadPool
{
public:
    ThreadPool()
    {
    }
    ThreadPool(const ThreadPool&) = delete;
    ~ThreadPool()
    {
        for (auto& t : threads) t.join();
    }
    void stop()
    {
        {
            std::lock_guard<std::mutex> lock(mutex);
            while (!tasks.empty()) tasks.pop();
        }
        for (auto& t : threads)t.join();
        threads.clear();
        counter = 0;
    }

    void start(size_t thread_num)
    {
        while (thread_num--) threads.emplace_back([this]() {worker(); });
    }

    void addTask(std::function<void()> tsk)
    {
        std::lock_guard<std::mutex> lock(mutex);
        tasks.push(tsk);
    }

    size_t getCounter() const noexcept { return counter; }

    bool finished() const noexcept
    {
        return tasks.empty();
    }

    void wait() const noexcept
    {
        while (!finished()) std::this_thread::yield();
    }

private:
    void worker()
    {
        for (;;) {
            std::function<void()> tsk;
            {
                std::lock_guard<std::mutex> lock(mutex);
                if (tasks.empty()) return;
                tsk = tasks.front();
                tasks.pop();
                counter++;
            }
            tsk();
        }
    }
    std::queue<std::function<void()>> tasks;
    std::vector<std::thread> threads;
    std::mutex mutex;
    std::size_t counter = 0;
};
